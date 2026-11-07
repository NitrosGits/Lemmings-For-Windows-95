#!/usr/bin/env python3
"""hswap.py FILE FUNC : try swapping pairs of header extern lines for the globals FUNC uses; report gains.
Works on a private copy of src/include (scratch include dir first on the path)."""
import itertools, os, re, shutil, subprocess, sys
ROOT = '/Users/daniel/Lemmings-For-Windows-95'
src, func = sys.argv[1], sys.argv[2]
work = f'{ROOT}/build/scratch/s30_hswap/w_{re.sub(r"\W","_",func)}'
os.makedirs(work, exist_ok=True)
text = open(f'{ROOT}/{src}', encoding='latin1').read()
# function body
m = re.search(r'^[^\n]*\b%s\s*\([^)]*\)\s*\n\{' % re.escape(func), text, re.M)
body = text[m.start(): text.index('\n}\n', m.start())]
gl = sorted(set(re.findall(r'\bg_\w+', body)))
hdrs = ['winlemm.h', 'window.h']
H = {}
for h in hdrs:
    out = []
    for l in open(f'{ROOT}/src/include/{h}', encoding='latin1').read().split('\n'):
        mm = re.match(r'(\s*extern\s+[A-Za-z_]\w*\s+)((?:g_\w+\s*,\s*)+g_\w+)\s*;(.*)$', l)
        if mm and '*' not in l and '[' not in l:
            for nm in [x.strip() for x in mm.group(2).split(',')]:
                out.append(mm.group(1) + nm + ';')
        else:
            out.append(l)
    H[h] = out
# split multi-name extern lines? only single-name lines are swappable
loc = {}
for h, L in H.items():
    for i, l in enumerate(L):
        mm = re.match(r'\s*extern\s+[^;(]*?\b(g_\w+)\s*(\[[^\]]*\])?\s*;', l)
        if mm and mm.group(1) in gl and l.count(',') == 0:
            loc[mm.group(1)] = (h, i)
names = [g for g in gl if g in loc]
def score(Hmod):
    inc = f'{work}/inc'
    if os.path.exists(inc): shutil.rmtree(inc)
    shutil.copytree(f'{ROOT}/src/include', inc)
    for h, L in Hmod.items(): open(f'{inc}/{h}', 'w', encoding='latin1').write('\n'.join(L))
    s = text
    for h in hdrs: s = s.replace('#include "%s"' % h, '#include "%s/%s"' % (inc, h))
    # nested includes inside headers
    for h in hdrs:
        p = f'{inc}/{h}'; t = open(p, encoding='latin1').read()
        for h2 in hdrs: t = t.replace('#include "%s"' % h2, '#include "%s/%s"' % (inc, h2))
        open(p, 'w', encoding='latin1').write(t)
    sp = f'{work}/t' + os.path.splitext(src)[1]
    open(sp, 'w', encoding='latin1').write(s)
    o = subprocess.run([f'{ROOT}/tools/msvc/try.sh', 'LEMMINGS.EXE', sp], capture_output=True, text=True).stdout
    f = re.search(r'%s: (MATCH|DIFF.*?(\d+) insns)' % re.escape(func), o)
    t = re.search(r'(\d+)/\d+ functions match', o)
    return (0 if f and f.group(1) == 'MATCH' else int(f.group(2)) if f else 999), (int(t.group(1)) if t else -1)
base = score(H)
print(func, 'globals', names, 'base', base, flush=True)
best = base
pairs = [(a, b) for a, b in itertools.combinations(names, 2) if loc[a][0] == loc[b][0]]
DOUBLE = os.environ.get('DOUBLE') == '1'
combos = [[p] for p in pairs] + ([[p, q] for p, q in itertools.combinations(pairs, 2) if not set(p) & set(q)] if DOUBLE else [])
for combo in combos:
  if True:
    Hm = {h: list(L) for h, L in H.items()}
    for a, b in combo:
        (ha, ia), (hb, ib) = loc[a], loc[b]
        Hm[ha][ia], Hm[ha][ib] = Hm[ha][ib], Hm[ha][ia]
    r = score(Hm)
    if r[0] < base[0]:
        print('  GAIN', combo, r, flush=True)
    if r[0] < best[0]: best = r
if False:
  for a, b in []:
    (ha, ia), (hb, ib) = loc[a], loc[b]
    Hm = {h: list(L) for h, L in H.items()}
    if ha == hb:
        Hm[ha][ia], Hm[ha][ib] = Hm[ha][ib], Hm[ha][ia]
    else:
        continue
    r = score(Hm)
    if r[0] < base[0]:
        print('  GAIN', a, b, r, flush=True)
    if r < best or (r[0] < best[0]): best = r
print('done', func, 'best', best, flush=True)
