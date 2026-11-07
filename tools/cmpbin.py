#!/usr/bin/env python3
"""Compare a rebuilt module with the original, byte for byte.

  cmpbin.py REBUILT ORIGINAL

Differences are grouped: the link timestamps and linker version (our Visual C++
2.0 linker is 2.50, the original's was 2.55), the import tables and the import
slot addresses in code (2.55 orders imports differently), and everything else,
which is a real mismatch and is listed with the function it falls in.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe import PE  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def functions(mod):
    p = os.path.join(ROOT, 'analysis', mod, 'functions.tsv')
    if not os.path.exists(p):
        return []
    out = []
    for line in open(p).readlines()[1:]:
        f = line.split('\t')
        out.append((int(f[0], 16), int(f[1], 16), f[3]))
    return out


def main(new, old):
    a, b = PE(new), PE(old)
    da, db = a.d, b.d
    if len(da) != len(db):
        print(f'size {len(da)} vs original {len(db)}')
    pe = struct.unpack_from('<I', db, 0x3c)[0]
    known = {pe + 8 + i: 'timestamp' for i in range(4)}
    known[pe + 24 + 3] = 'linker version'
    erva = b.dirs[0][0]
    if erva:
        eoff = b.off(b.base + erva)
        for i in range(4):
            known[eoff + 4 + i] = 'timestamp'
    rrva = b.dirs[2][0] if len(b.dirs) > 2 else 0
    if rrva:
        # every resource directory carries a TimeDateStamp at +4
        roff = b.off(b.base + rrva)
        todo = [0]
        while todo:
            o = todo.pop()
            for k in range(4):
                known[roff + o + 4 + k] = 'timestamp'
            nnamed, nid = struct.unpack_from('<HH', db, roff + o + 12)
            for e in range(nnamed + nid):
                ent = struct.unpack_from('<I', db, roff + o + 16 + 8 * e + 4)[0]
                if ent & 0x80000000:
                    todo.append(ent & 0x7fffffff)
    irva, isz = b.dirs[1]
    iat_lo = min(b.imports) if b.imports else 0
    iat_hi = max(b.imports) + 4 if b.imports else 0
    idata = b.sec('.idata') if any(s['name'] == '.idata' for s in b.sections) else None
    funcs = functions(os.path.basename(old).upper())

    groups = {}
    real = []
    for i in range(min(len(da), len(db))):
        if da[i] == db[i]:
            continue
        if i in known:
            groups[known[i]] = groups.get(known[i], 0) + 1
            continue
        if idata and idata['raw'] <= i < idata['raw'] + idata['rsize']:
            groups['import tables'] = groups.get('import tables', 0) + 1
            continue
        # inside a relocated dword that points at import slots in both?
        sec = next((s for s in b.sections if s['raw'] <= i < s['raw'] + s['rsize']), None)
        if sec:
            va = sec['va'] + i - sec['raw']
            hit = False
            for r in range(va - 3, va + 1):
                if r in b.relocs and r in a.relocs:
                    vb, vn = b.u32(r), a.u32(r)
                    if iat_lo <= vb < iat_hi and iat_lo <= vn < iat_hi:
                        hit = True
            if hit:
                groups['import slot addresses'] = groups.get('import slot addresses', 0) + 1
                continue
            fn = next((f[2] for f in funcs if f[0] <= va <= f[1]), '')
            real.append((i, va, sec['name'], fn))
        else:
            real.append((i, None, 'header', ''))
    name = os.path.basename(new)
    if not groups and not real and len(da) == len(db):
        print(f'{name}: IDENTICAL to the original')
        return 0
    if not real and len(da) == len(db):
        print(f'{name}: matches the original except ' + ', '.join(f'{k} ({v} bytes)' for k, v in groups.items()))
        return 0
    print(f'{name}: {len(real)} bytes differ' + (' (plus ' + ', '.join(f'{k} {v}' for k, v in groups.items()) + ')' if groups else ''))
    shown = set()
    for i, va, sec, fn in real:
        key = (sec, fn)
        if key in shown:
            continue
        shown.add(key)
        print(f'  {sec} {va:#x} {fn}' if va is not None else f'  header offset {i:#x}')
        if len(shown) > 30:
            print('  ...')
            break
    return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1], sys.argv[2]))
