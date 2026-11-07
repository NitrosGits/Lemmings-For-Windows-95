#!/usr/bin/env python3
"""infer.py PROJECT MODULE: map our data symbols to the original's D_ADDR by pairing relocs of built units with the target's."""
import json, os, re, sys, collections
sys.path.insert(0, '/Users/daniel/Lemmings-For-Windows-95/tools')
from coff import Obj
proj, mod = sys.argv[1:3]
os.chdir(proj)
j = json.load(open('config/lemmings/functions.json'))
votes = collections.defaultdict(collections.Counter)
for u, fs in j['units'].items():
    if not u.startswith(mod + '/'): continue
    p = f'build/lemmings/src/auto/{u}.obj'
    if not os.path.exists(p): continue
    o = Obj(p)
    sec = next((s for s in o.sections if s.name.startswith('.text')), None)
    syms = o.symbols  # dict? keyed by index or name
    items = list(syms.values()) if isinstance(syms, dict) else syms
    byidx = {s.idx: s for s in items}
    # function start offsets in .text, by symbol name
    funcs = {}
    for s in items:
        if s.is_function and s.secnum == sec.idx if sec else False:
            funcs[s.name] = s.value
    for f in fs:
        start = None
        for n, v in funcs.items():
            if re.search(r'func_%08X' % f['addr'], n, re.I): start = v
        if start is None: continue
        ours = {}
        for off, si, typ in sec.relocs:
            if start <= off < start + f['size']:
                add = int.from_bytes(sec.data[off:off+4], 'little')
                ours[off - start] = (byidx[si].name, add)
        for off, _, name, add in f['relocs']:
            if name and name.startswith('_D_') and off in ours:
                n, oadd = ours[off]
                if oadd == add and not n.startswith('_D_'):
                    votes[n][name] += 1
names = j['names']; added = 0; conflicts = []
for n, c in votes.items():
    best, cnt = c.most_common(1)[0]
    if len(c) > 1: conflicts.append((n, dict(c)))
    key = best[1:]  # D_ADDR
    real = n[1:] if n.startswith('_') else n
    real = real.split('@')[0]
    if key not in names and real not in names.values():
        names[key] = real; added += 1
json.dump(j, open('config/lemmings/functions.json', 'w'), indent=1)
print('data names added', added, 'conflicts', conflicts[:5])
