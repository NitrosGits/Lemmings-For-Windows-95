#!/usr/bin/env python3
"""Print strings of an original module as C literals: cstr.py MODULE ADDR [ADDR...]"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from pe import PE  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def lit(b):
    out = []
    for c in b:
        ch = chr(c)
        if ch == '\n':
            out.append('\\n')
        elif ch == '\r':
            out.append('\\r')
        elif ch == '\t':
            out.append('\\t')
        elif ch in '"\\':
            out.append('\\' + ch)
        elif 32 <= c < 127:
            out.append(ch)
        else:
            out.append('\\%03o' % c)
    return '"' + ''.join(out) + '"'


if __name__ == '__main__':
    pe = PE(os.path.join(ROOT, 'orig', 'WINLEMM', sys.argv[1]))
    for a in sys.argv[2:]:
        v = int(a, 16)
        print(f'{v:#x}: {lit(pe.cstr(v))}')
