#!/usr/bin/env python3
"""The order VC 2.2 gives a file's uninitialised globals (.bss), and checks.

C1XX hashes identifiers with h = 4*h + c + (h >> 7) (32 bits, from 0; the
tokenizer at C1XX 0x40b5e2). Within one object file, .bss symbols are emitted
in ascending bucket H(name) & 0x3ff; names in one bucket come later-defined
first. (Alignment: arrays/structs of 8+ bytes are 8-aligned.)

  bsshash.py name...           print each name's bucket
  bsshash.py --check OBJ...    compare an object's .bss order with the prediction
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))


def H(s):
    h = 0
    for ch in s:
        h = (4 * h + ord(ch) + (h >> 7)) & 0xffffffff
    return h


def bucket(name):
    return H(name) & 0x3ff


def undecorate(n):
    if n.startswith('?') and '@@' in n:
        return n[1:n.index('@@')]
    if n.startswith('_'):
        return n[1:]
    return n


def check(path):
    import coff
    o = coff.Obj(path)
    b = [s for s in o.sections if s.name == '.bss']
    if not b:
        return True
    syms = sorted((y.value, y.name) for y in o.symbols.values() if y.secnum == b[0].idx and not y.name.startswith('.'))
    keys = [bucket(undecorate(n)) for v, n in syms]
    ok = keys == sorted(keys)
    print(f'{os.path.basename(path)}: {len(syms)} .bss symbols, order {"matches" if ok else "DIFFERS from"} the hash')
    return ok


if __name__ == '__main__':
    if sys.argv[1] == '--check':
        sys.exit(0 if all([check(p) for p in sys.argv[2:]]) else 1)
    for n in sys.argv[1:]:
        print(n, bucket(n), hex(H(n)))
