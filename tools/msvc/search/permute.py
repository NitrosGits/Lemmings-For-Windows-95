#!/usr/bin/env python3
"""Stochastic source-level permuter for register/scheduling near-misses (a poor man's decomp-permuter
for VC 2.x).  Hill-climbs (equal-or-better accepted, restarts on stall) over random semantic-preserving
mutations of ONE function body:
  decl   swap two local declarations
  stmt   swap two adjacent call-free single-line statements that do not touch the same names
  cmp    flip a simple comparison  A < B  ->  B > A
  comm   swap the operands of a simple  A + B  /  A * B
  assign x += k  <->  x = x + k ;  x++ <-> x += 1
Scoring is tools/msvc/try.sh (instructions differing), so pass a SMALL translation unit
(e.g. the file truncated after the function; see HANDOFF session 36) to make it fast.

  permute.py MODULE SOURCE ADDR [--iters 200] [--per 6] [--jobs 3] [--seed 1] [--muts 2]
Prints every new best and the final body (which then has to be applied and verified in the real file).
"""
import argparse, concurrent.futures, os, random, re, shutil, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
TOK = r'(?:[A-Za-z_]\w*(?:\[[A-Za-z_0-9]+\])?|\d+)'
DECL = re.compile(r'^    (?:volatile |register )?[A-Za-z_][\w ]*[ *]+\*?\w+(\[[^\]]*\])?( = [^;]+)?;\s*$')
SIMPLE = re.compile(r'^(\s+)([A-Za-z_]\w*(?:\[[^\]]*\])?(?:(?:\.|->)\w+)*) (=|\+=|-=|<<=|>>=) ([^;(){}]+);\s*$')


def names(s):
    return set(re.findall(r'[A-Za-z_]\w*', s))


def mutate(lines, rnd):
    L = list(lines)
    kind = rnd.choice(['decl', 'stmt', 'stmt', 'cmp', 'comm', 'assign'])
    if kind == 'decl':
        idx = [i for i, l in enumerate(L) if DECL.match(l)]
        if len(idx) < 2:
            return None
        a, b = rnd.sample(idx, 2)
        if abs(a - b) > 20:
            return None
        L[a], L[b] = L[b], L[a]
        return L
    if kind == 'stmt':
        idx = [i for i in range(len(L) - 1) if SIMPLE.match(L[i]) and SIMPLE.match(L[i + 1])
               and SIMPLE.match(L[i]).group(1) == SIMPLE.match(L[i + 1]).group(1)]
        if not idx:
            return None
        i = rnd.choice(idx)
        wa, wb = names(L[i]), names(L[i + 1])
        va, vb = SIMPLE.match(L[i]).group(2), SIMPLE.match(L[i + 1]).group(2)
        if names(va) & wb or names(vb) & wa or (wa & wb):
            if not ((wa & wb) <= set()):
                return None
        L[i], L[i + 1] = L[i + 1], L[i]
        return L
    if kind == 'cmp':
        pat = re.compile(r'(' + TOK + r') (<=|>=|<|>) (' + TOK + r')')
        cands = [(i, m) for i, l in enumerate(L) for m in pat.finditer(l)]
        if not cands:
            return None
        i, m = rnd.choice(cands)
        flip = {'<': '>', '>': '<', '<=': '>=', '>=': '<='}[m.group(2)]
        L[i] = L[i][:m.start()] + m.group(3) + ' ' + flip + ' ' + m.group(1) + L[i][m.end():]
        return L
    if kind == 'comm':
        pat = re.compile(r'(?<![\w*])(' + TOK + r') ([+*]) (' + TOK + r')(?![\w\[(])')
        cands = [(i, m) for i, l in enumerate(L) if not l.lstrip().startswith(('extern', '/*')) for m in pat.finditer(l)]
        if not cands:
            return None
        i, m = rnd.choice(cands)
        L[i] = L[i][:m.start()] + m.group(3) + ' ' + m.group(2) + ' ' + m.group(1) + L[i][m.end():]
        return L
    if kind == 'assign':
        pat1 = re.compile(r'^(\s+)(\w+) \+= ([^;]+);')
        pat2 = re.compile(r'^(\s+)(\w+) = \2 \+ ([^;]+);')
        c1 = [i for i, l in enumerate(L) if pat1.match(l)]
        c2 = [i for i, l in enumerate(L) if pat2.match(l)]
        if c1 and (not c2 or rnd.random() < 0.5):
            i = rnd.choice(c1); m = pat1.match(L[i])
            L[i] = f'{m.group(1)}{m.group(2)} = {m.group(2)} + {m.group(3)};'
            return L
        if c2:
            i = rnd.choice(c2); m = pat2.match(L[i])
            L[i] = f'{m.group(1)}{m.group(2)} += {m.group(3)};'
            return L
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr')
    ap.add_argument('--iters', type=int, default=200); ap.add_argument('--per', type=int, default=6)
    ap.add_argument('--jobs', type=int, default=3); ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--muts', type=int, default=2)
    a = ap.parse_args()
    rnd = random.Random(a.seed)
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    st = text.index('/* ' + a.addr); s = text.index('\n{\n', st) + 3; e = text.index('\n}\n', s)
    base = os.path.basename(src)
    orig = text[s:e].split('\n')
    cache = {}

    def score(body):
        key = '\n'.join(body)
        if key in cache:
            return cache[key]
        dd = os.path.join(ROOT, 'build/scratch/permute', '%d_%d' % (os.getpid(), len(cache)))
        os.makedirs(dd, exist_ok=True)
        p = os.path.join(dd, base)
        open(p, 'w', encoding='latin1').write(text[:s] + key + text[e:])
        o = subprocess.run(['zsh', os.path.join(ROOT, 'tools/msvc/try.sh'), a.module, p], cwd=ROOT,
                           capture_output=True, text=True).stdout
        shutil.rmtree(dd, ignore_errors=True)
        line = next((l for l in o.splitlines() if re.match(re.escape(a.addr) + r'\s', l)), '')
        m = re.search(r'(\d+) insns differ', line)
        v = 0 if ': MATCH' in line else int(m.group(1)) if m else 9999
        cache[key] = v
        return v

    cur, cs = orig, score(orig)
    best = (cs, cur)
    print('start', cs, flush=True)
    stall = 0
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
        for it in range(a.iters):
            nb = []
            for _ in range(a.per * 4):
                x = cur
                for _ in range(rnd.randint(1, a.muts)):
                    y = mutate(x, rnd)
                    if y:
                        x = y
                if x is not cur and '\n'.join(x) not in cache:
                    nb.append(x)
                if len(nb) >= a.per:
                    break
            if not nb:
                continue
            sc = list(ex.map(score, nb))
            k = min(range(len(nb)), key=lambda i: sc[i])
            if sc[k] <= cs:
                stall = 0 if sc[k] < cs else stall + 1
                cur, cs = nb[k], sc[k]
                if cs < best[0]:
                    best = (cs, cur)
                    print('iter', it, 'new best', cs, flush=True)
                if cs == 0:
                    break
            else:
                stall += 1
            if stall > 40:
                cur, cs = best[1], best[0]; stall = 0
    print('BEST', best[0])
    print('\n'.join(best[1]))


if __name__ == '__main__':
    main()
