#!/usr/bin/env python3
"""Sweep dummy locals in a shared inline body x padding before a function, for one file.

  inlinepad.py MODULE SOURCE MACRO NLOCALS(lo-hi) PADPREFIX NPADS(lo-hi) FUNC [--jobs N]

The shared header must contain `#ifdef MACRO / MACRO / #endif` inside the inline body (see
window.h's LAYOUT_PAD in Dock::Layout). Each variant defines MACRO as N `int __lpK;` declarations at
the top of a copy of SOURCE and rewrites the `extern int PADPREFIX0, ...;` line to M names. Inline
members are expanded at the end of the file, so their registers depend on these counts (session 34).
Reports FUNC's differing instructions and the file's match count, best first. Never edits SOURCE.
"""
import itertools, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

def main():
    a = sys.argv[1:]; jobs = 6
    if '--jobs' in a:
        k = a.index('--jobs'); jobs = int(a[k + 1]); del a[k:k + 2]
    mod, src, macro, rl, pfx, rp, func = a
    s = open(src, encoding='latin1').read()
    r = lambda t: range(int(t.split('-')[0]), int(t.split('-')[1]) + 1)
    pat = re.compile(r'extern int %s0[^;]*;\n' % re.escape(pfx))
    m = pat.search(s)
    def run(c):
        nl, np_ = c
        t = s[:m.start()] + (('extern int ' + ', '.join('%s%d' % (pfx, k) for k in range(np_)) + ';\n') if np_ else '') + s[m.end():]
        if nl:
            t = '#define %s %s\n' % (macro, ' '.join('int __lp%d;' % k for k in range(nl))) + t
        p = os.path.join(os.path.dirname(os.path.abspath(src)), '.inlinepad_%d_%d%s' % (nl, np_, os.path.splitext(src)[1]))
        open(p, 'w', encoding='latin1').write(t)
        try:
            o = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, p], capture_output=True, text=True).stdout
        finally:
            os.remove(p)
        f = re.search(r'^0x\w+ %s: (MATCH|DIFF.*?(\d+) insns differ)' % re.escape(func), o, re.M)
        sc = 999 if not f else (0 if f.group(1) == 'MATCH' else int(f.group(2)))
        mm = re.search(r'(\d+)/(\d+) functions match', o)
        diffs = ','.join(re.findall(r'^0x\w+ ([^:]+): DIFF', o, re.M))
        return -(int(mm.group(1)) if mm else 0), sc, nl, np_, diffs
    with ThreadPoolExecutor(jobs) as ex:
        res = sorted(ex.map(run, itertools.product(r(rl), r(rp))), key=lambda x: (x[1], x[0]))
    for x in res[:20]:
        print('locals=%d pads=%d: %s %d insns, %d functions match, diffs: %s' % (x[2], x[3], func, x[1], -x[0], x[4]))

if __name__ == '__main__':
    main()
