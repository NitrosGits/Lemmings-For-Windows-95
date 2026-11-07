#!/usr/bin/env python3
"""Joint sweep of two symbol-padding blocks in one source file.

  pad2d.py MODULE SOURCE PREFIX_A RANGE_A PREFIX_B RANGE_B FUNC... [--jobs N]

PREFIX_A/B name existing `extern int PREFIXn, ...;` padding lines (e.g. pad_savelayout); each is
rewritten with every count in RANGE (lo-hi). Every combination is compiled with try.sh and scored
by the summed differing instructions of the listed FUNCs (plus the file's match count). Needed when
padding after a function changes the functions before it (mainwnd.cpp: SaveLayout/LayoutWindows).
Prints the best combinations; never edits SOURCE.
"""
import itertools, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))


def setpad(s, prefix, n):
    pat = re.compile(r'extern int %s0[^;]*;\n' % re.escape(prefix))
    m = pat.search(s)
    if not m:
        raise SystemExit('no padding line for ' + prefix)
    line = ('extern int ' + ', '.join('%s%d' % (prefix, k) for k in range(n)) + ';\n') if n else ''
    return s[:m.start()] + line + s[m.end():]


def main():
    args = sys.argv[1:]
    jobs = 8
    if '--jobs' in args:
        k = args.index('--jobs'); jobs = int(args[k + 1]); del args[k:k + 2]
    mod, src, pa, ra, pb, rb = args[:6]
    funcs = args[6:]
    s = open(src, encoding='latin1').read()
    rng = lambda r: range(int(r.split('-')[0]), int(r.split('-')[1]) + 1)
    combos = list(itertools.product(rng(ra), rng(rb)))
    ext = os.path.splitext(src)[1]

    def run(c):
        a, b = c
        t = setpad(setpad(s, pa, max(a, 1)), pb, max(b, 1))
        if a == 0: t = re.sub(r'extern int %s0;\n' % re.escape(pa), '', t)
        if b == 0: t = re.sub(r'extern int %s0;\n' % re.escape(pb), '', t)
        p = os.path.join(os.path.dirname(os.path.abspath(src)), '.pad2d_%d_%d%s' % (a, b, ext))
        open(p, 'w', encoding='latin1').write(t)
        try:
            o = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, p], capture_output=True, text=True).stdout
        finally:
            os.remove(p)
        tot = 0
        for f in funcs:
            m = re.search(r'^0x\w+ %s: (MATCH|DIFF.*?(\d+) insns differ)' % re.escape(f), o, re.M)
            tot += 999 if not m else (0 if m.group(1) == 'MATCH' else int(m.group(2)))
        m = re.search(r'(\d+)/(\d+) functions match', o)
        return tot, -(int(m.group(1)) if m else 0), a, b

    with ThreadPoolExecutor(jobs) as ex:
        res = sorted(ex.map(run, combos))
    for r in res[:15]:
        print('pads %s=%d %s=%d: %d insns over %s, %d functions match' % (pa, r[2], pb, r[3], r[0], ','.join(funcs), -r[1]))


if __name__ == '__main__':
    main()
