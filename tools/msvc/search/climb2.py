#!/usr/bin/env python3
"""Plateau-tolerant hill climb over (a) the order of a function's leading local declarations and
(b) the order of its first run of one-line assignment statements (swaps that would break a
data dependency are skipped). Accepts equal-or-better scores, so it walks plateaus.

  climb2.py MODULE SOURCE ADDR [--rounds 40] [--per 6] [--jobs 3] [--seed 1]

Score = instructions differing (match.py via try.sh); stops at 0 (a match). Prints the best body.
Compiles under the real file name in build/scratch/climb2/<n>/ (see tools/msvc/realname.py).
"""
import argparse, concurrent.futures, os, random, re, shutil, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
DECL = re.compile(r'^    [A-Za-z_][\w ]*[ *]+\*?\w+(\[[^\]]*\])?( = [^;]+)?;\s*$')
ASSIGN = re.compile(r'^    (\w+)(\.\w+|->\w+|\[[^\]]*\])* = [^;{}]+;\s*$')


def words(s):
    return set(re.findall(r'[A-Za-z_]\w*', s))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr')
    ap.add_argument('--rounds', type=int, default=40); ap.add_argument('--per', type=int, default=6)
    ap.add_argument('--jobs', type=int, default=3); ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--temp', type=float, default=0.0, help='annealing temperature: accept a worse neighbour with prob exp(-delta/temp)')
    ap.add_argument('--pad', type=int, default=0, help='extern int pad_c2_N symbols inserted before the function')
    a = ap.parse_args()
    random.seed(a.seed)
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    st = text.index('/* ' + a.addr); s = text.index('\n{\n', st) + 3; e = text.index('\n}\n', s)
    lines = text[s:e].split('\n')
    i = 0
    while i < len(lines) and (DECL.match(lines[i]) or not lines[i].strip() and i == 0):
        i += 1
    nd = i
    while i < len(lines) and not lines[i].strip():
        i += 1
    while i < len(lines) and not ASSIGN.match(lines[i]):
        i += 1
    j = i
    while j < len(lines) and ASSIGN.match(lines[j]):
        j += 1
    decl, mid, rest = lines[:nd], lines[i:j], lines[j:]
    gap = lines[nd:i]
    base = os.path.basename(src)
    print(f'{nd} decls, {len(mid)} leading assignments')

    def build(d, m):
        return '\n'.join(d + gap + m + rest)

    cache = {}

    def score(key):
        d, m = key
        if key in cache:
            return cache[key]
        n = len(cache)
        dd = os.path.join(ROOT, 'build/scratch/climb2', str(os.getpid()) + '_' + str(n))
        os.makedirs(dd, exist_ok=True)
        p = os.path.join(dd, base)
        padtxt = ('extern int ' + ', '.join('pad_c2_%d' % n for n in range(a.pad)) + ';\n') if a.pad else ''
        open(p, 'w', encoding='latin1').write(text[:st] + padtxt + text[st:s] + build(list(d), list(m)) + text[e:])
        o = subprocess.run(['zsh', os.path.join(ROOT, 'tools/msvc/try.sh'), a.module, p], cwd=ROOT,
                           capture_output=True, text=True).stdout
        shutil.rmtree(dd, ignore_errors=True)
        line = next((l for l in o.splitlines() if re.match(re.escape(a.addr) + r'\s', l)), '')
        mm = re.search(r'(\d+) insns differ', line)
        v = 0 if ': MATCH' in line else int(mm.group(1)) if mm else 9999
        cache[key] = v
        return v

    def neighbours(d, m):
        out = []
        for _ in range(a.per * 3):
            if random.random() < 0.6 and len(d) > 1:
                x, y = random.sample(range(len(d)), 2)
                nd_ = list(d); nd_[x], nd_[y] = nd_[y], nd_[x]
                out.append((tuple(nd_), m))
            elif len(m) > 1:
                x = random.randrange(len(m) - 1)
                wa, wb = m[x], m[x + 1]
                av = ASSIGN.match(wa).group(1); bv = ASSIGN.match(wb).group(1)
                if av in words(wb) or bv in words(wa) or av == bv:
                    continue
                nm = list(m); nm[x], nm[x + 1] = nm[x + 1], nm[x]
                out.append((d, tuple(nm)))
        seen, res = set(), []
        for k in out:
            if k not in seen and k not in cache:
                seen.add(k); res.append(k)
        return res[:a.per]

    cur = (tuple(decl), tuple(mid))
    cs = score(cur)
    print('start', cs)
    best = (cs, cur)
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
        for r in range(a.rounds):
            nb = neighbours(*cur)
            if not nb:
                continue
            sc = list(ex.map(score, nb))
            cand = sorted(zip(sc, range(len(nb))))
            import math
            T = a.temp * (1.0 - r / max(1, a.rounds)) if a.temp else 0.0
            if cand and (cand[0][0] <= cs or (T > 0 and random.random() < math.exp(-(cand[0][0] - cs) / T))):
                cs = cand[0][0]; cur = nb[cand[0][1]]
                if cs < best[0]:
                    best = (cs, cur)
                    print('round', r, 'new best', cs, flush=True)
            if cs == 0:
                break
    print('BEST', best[0])
    print(build(list(best[1][0]), list(best[1][1])))


if __name__ == '__main__':
    main()
