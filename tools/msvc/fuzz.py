#!/usr/bin/env python3
"""Mutation fuzzer for register-tie misses: random combinations of
  retype (integer locals), reorder declarations, init-at-declaration (move the first top-level
  `x = expr;` into `T x = expr;`), dummy unused locals, and caching read-only globals in locals.
Scored by shapes.py.   fuzz.py MODULE SOURCE ADDR [--n 400] [--pads 0] [--seed 1]
"""
import argparse, os, random, re, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DECL = re.compile(r'^(\s+)((?:unsigned |signed |const |struct )*[A-Za-z_]\w*(?:\s+|\s*\*+\s*))(\*?\w+(?:\[[^\]]*\])?)\s*(=\s*[^;]+)?;\s*(/\*.*\*/)?$')
INTS = ['int', 'unsigned int', 'long', 'DWORD', 'UINT', 'unsigned long']
SHORTS = ['short', 'unsigned short', 'WORD']
def retype(ty):
    if ty in ('int','unsigned','long','unsigned int','unsigned long','DWORD','UINT'): return random.choice(INTS)
    if ty in ('short','unsigned short','WORD'): return random.choice(SHORTS)
    return ty
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr')
    ap.add_argument('--n', type=int, default=400); ap.add_argument('--pads', default='0')
    ap.add_argument('--seed', type=int, default=1); ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--out')
    a = ap.parse_args(); random.seed(a.seed)
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    ann = text.index('/* ' + a.addr); s = text.index('\n{\n', ann) + 3; e = text.index('\n}\n', s)
    body = text[s:e]; lines = body.split('\n')
    nd = 0
    while nd < len(lines) and DECL.match(lines[nd]): nd += 1
    decl = lines[:nd]
    rest = lines[nd:]
    while rest and not rest[0].strip(): rest = rest[1:]
    parsed = [DECL.match(l).groups() for l in decl]
    variants = set(); tries = 0
    while len(variants) < a.n and tries < a.n * 30:
        tries += 1
        items = [list(p) for p in parsed]
        r = list(rest)
        # init-at-declaration
        for it in items:
            if it[3] is None and '*' not in it[1] and '[' not in it[2] and random.random() < 0.5:
                name = it[2].strip('* ')
                for k, l in enumerate(r):
                    if re.search(r'\b%s\b' % name, l):
                        m = re.match(r'^    %s = ([^;]+);\s*$' % name, l)
                        if m:
                            it[3] = '= ' + m.group(1); del r[k]
                        break
        if random.random() < 0.8:
            for it in items:
                if it[3] is None or '(' not in (it[3] or ''):
                    it[1] = retype(it[1].strip()) + ' '
        if random.random() < 0.7: random.shuffle(items)
        if random.random() < 0.3:
            items.insert(random.randrange(len(items) + 1), ['    ', random.choice(['int', 'unsigned', 'BYTE *', 'short']), 'zz', None, None])
        d = ['%s%s%s%s;' % (it[0], it[1] if it[1].endswith((' ','*')) else it[1] + ' ', it[2], (' ' + it[3]) if it[3] else '') for it in items]
        # initialised decls must not use later-declared locals: skip variants that break order
        names_seen = set(); ok = True
        for it in items:
            if it[3]:
                for n in re.findall(r'\b[a-z]\w*\b', it[3]):
                    if any(n == p[2].strip('* ') for p in parsed) and n not in names_seen: ok = False
            names_seen.add(it[2].strip('* '))
        if not ok: continue
        variants.add('\n'.join(d) + '\n\n' + '\n'.join(r).lstrip('\n'))
    variants.discard(body)
    od = a.out or os.path.join(ROOT, 'build/scratch/fuzz', a.addr); os.makedirs(od, exist_ok=True)
    spec = os.path.join(od, 'spec.py')
    open(spec, 'w').write('SLOTS=[(%r,%r)]\n' % (body, sorted(variants)))
    print(len(variants), 'variants', flush=True)
    subprocess.run([sys.executable, os.path.join(ROOT, 'tools/msvc/shapes.py'), a.module, a.source, a.addr, spec,
                    '--pads', a.pads, '--jobs', str(a.jobs), '--max', str(len(variants) + 5), '--out', od])
if __name__ == '__main__': main()
