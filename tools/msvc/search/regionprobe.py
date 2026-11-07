#!/usr/bin/env python3
"""Compile body variants of one function in its real file and print part of OUR code for each.

  regionprobe.py MODULE SOURCE SIGFILE BODIES.txt FUNC [--from REGEX] [--to REGEX] [--nth N] [--jobs N]

BODIES.txt: bodies separated by lines of just '%%' (as variants.py takes them, no [[ ]] choices).
For each body prints its score (differing instructions; 0 = MATCH) and our instructions from the
N-th line matching --from (default: the start) up to the first line matching --to after it
(default: the first conditional jump). Used to see how a source change moves one register choice
or scheduling decision (e.g. StretchChanged's `cy0 * across` product) without reading full diffs.
"""
import argparse, os, re, subprocess
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('sigfile'); ap.add_argument('bodies')
    ap.add_argument('func'); ap.add_argument('--from', dest='frm', default=None); ap.add_argument('--nth', type=int, default=1)
    ap.add_argument('--to', default=r'\bj(?!mp)[a-z]+\s'); ap.add_argument('--jobs', type=int, default=6)
    a = ap.parse_args()
    sig = open(a.sigfile).read().rstrip('\n')
    s = open(a.source, encoding='latin1').read()
    i = s.index(sig + '\n{') + len(sig) + 2
    j = s.index('\n}\n', i)
    bodies = open(a.bodies, encoding='latin1').read().split('\n%%\n')
    ext = os.path.splitext(a.source)[1]

    def run(k):
        p = os.path.join(os.path.dirname(os.path.abspath(a.source)), '.regionprobe_%s_%d%s' % (a.func, k, ext))
        open(p, 'w', encoding='latin1').write(s[:i] + '\n' + bodies[k].strip('\n') + s[j:])
        try:
            o = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), a.module, p, '-f', a.func, '-v'],
                               capture_output=True, text=True).stdout
        finally:
            os.remove(p)
        lines = o.split('\n')
        m = re.search(r'(\d+) insns', lines[0])
        score = m.group(1) if m else ('0' if 'MATCH' in lines[0] else '?')
        ours = [l.split('|')[0].rstrip() for l in lines[1:] if '|' in l]
        start = 0
        if a.frm:
            hits = [n for n, l in enumerate(ours) if re.search(a.frm, l)]
            start = hits[a.nth - 1] if len(hits) >= a.nth else 0
        end = len(ours)
        for n in range(start + 1, len(ours)):
            if re.search(a.to, ours[n]):
                end = n + 1
                break
        return k, score, ours[start:end]

    with ThreadPoolExecutor(a.jobs) as ex:
        for k, score, region in ex.map(run, range(len(bodies))):
            print('== body %d: %s insns' % (k, score))
            for l in region:
                print('   ' + re.sub(r'^\s*[*]?\s*[0-9a-f]+\s+', '', l))


if __name__ == '__main__':
    main()
