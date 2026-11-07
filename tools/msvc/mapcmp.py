#!/usr/bin/env python3
"""Compare a linked module's symbol addresses (from its .map) with the
original's (tools/names/<MODULE>.txt):

  mapcmp.py MODULE [--sec .data|.bss|.text|all] [--all]

Prints, per object, its symbols in the chosen section with our address and
the original's, marking the ones that differ (--all: every symbol, not just
the objects with differences).
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXE = {'LEMMINGS': 'LEMMINGS.EXE'}


def undecorate(n):
    for pat in (r'^\?([A-Za-z_0-9]+)@@[0-9]', r'^\?([A-Za-z_0-9]+)@@', r'^\?([A-Za-z_0-9]+)@([A-Za-z_0-9]+)@@',
                r'^_([A-Za-z_0-9]+)@[0-9]+$', r'^_([A-Za-z_0-9]+)$'):
        m = re.match(pat, n)
        if m:
            if m.lastindex == 2:
                return f'{m.group(2)}::{m.group(1)}'
            return m.group(1)
    return n


def main():
    mod = sys.argv[1].upper()
    sec = '.data'
    if '--sec' in sys.argv:
        sec = sys.argv[sys.argv.index('--sec') + 1]
    show_all = '--all' in sys.argv
    out = os.path.join(ROOT, 'build', 'msvc', mod, f'{mod}.map')
    text = open(out).read()
    secs = {}
    for m in re.finditer(r'^ (\d{4}):([0-9a-f]{8}) ([0-9a-f]{8})H (\S+)', text, re.M):
        secs.setdefault(m.group(1), m.group(4).split('$')[0])
    orig = {}
    for l in open(os.path.join(ROOT, 'tools', 'names', f'{EXE.get(mod, mod + ".DLL")}.txt')):
        f = l.split()
        if len(f) >= 2 and re.match(r'^[0-9a-f]+$', f[0]):
            orig.setdefault(f[1], int(f[0], 16))
    rows = {}
    for m in re.finditer(r'^ (\d{4}):[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:[^\s:]+:)?(\S+\.obj)\s*$', text, re.M):
        s = secs.get(m.group(1), '?')
        if sec != 'all' and s != sec:
            continue
        name = undecorate(m.group(2))
        rows.setdefault(m.group(4), []).append((int(m.group(3), 16), name, orig.get(name, orig.get(m.group(2)))))
    nbad = 0
    for obj, syms in rows.items():
        bad = [r for r in syms if r[2] is not None and r[2] != r[0]]
        nbad += len(bad)
        if not bad and not show_all:
            continue
        print(f'== {obj}')
        for ours, name, theirs in sorted(syms):
            mark = '' if theirs is None else ('   ok' if theirs == ours else f'   original {theirs:#x} ({theirs - ours:+#x})')
            print(f'  {ours:#x} {name}{mark}')
    print(f'{nbad} symbols at the wrong address')


if __name__ == '__main__':
    main()
