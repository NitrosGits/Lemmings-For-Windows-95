#!/usr/bin/env python3
"""LIBC.LIB as the game linked it: Visual C++ 2.0's, with the differences we
have found between it and the later 2.x runtime the original used.

  mklibc.py OUTDIR
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
VCLIB = os.path.join(os.environ.get('VCDIR', os.path.join(ROOT, 'build', 'vc22', 'MSVC20')), 'LIB')

# (what, regex over the library's bytes that must match exactly once, replacement for group 1)
PATCHES = [
    # heapinit.obj, __heap_init: on old Windows versions the 2.x runtime lets heap
    # regions grow to 16MB (_heap_maxregsize), 2.0's to 256KB.
    ('_heap_maxregsize', rb'\xc7\x05....\x00\x40\x00\x00\xc7\x05....(\x00\x00\x04\x00)\xc3', b'\x00\x00\x00\x01'),
]


def main(outdir):
    d = bytearray(open(os.path.join(VCLIB, 'LIBC.LIB'), 'rb').read())
    for what, pat, rep in PATCHES:
        hits = list(re.finditer(pat, bytes(d), re.S))
        if len(hits) != 1:
            print(f'LIBC patch {what}: not needed ({len(hits)} matches)')
            continue
        s, e = hits[0].span(1)
        d[s:e] = rep
    os.makedirs(outdir, exist_ok=True)
    open(os.path.join(outdir, 'LIBC.LIB'), 'wb').write(d)
    print(f'LIBC.LIB: {len(PATCHES)} patches')


if __name__ == '__main__':
    main(sys.argv[1])
