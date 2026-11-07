#!/usr/bin/env python3
"""Sweep symbol padding inside a shared header: for each count N, insert N
padding declarations at a marker line of the header, compile every source of
the module against it and count the functions that match.

  hdrsweep.py MODULE HEADER 'marker line' A-B[/STEP] [--jobs N] [--write N]

The marker is an exact line of HEADER; the padding goes just before it. The
sources (the module's objs.txt) are compiled from scratch copies next to the
modified header, so the real files are untouched. --write N puts N lines of
padding into HEADER.
"""
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def padding(tag, n):
    if not n:
        return ''
    return ('/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */\n'
            'extern int ' + ', '.join(f'pad_{tag}{i}' for i in range(n)) + ';\n\n')


def main():
    args = sys.argv[1:]
    jobs = 8
    if '--jobs' in args:
        i = args.index('--jobs')
        jobs = int(args[i + 1])
        del args[i:i + 2]
    write = None
    if '--write' in args:
        i = args.index('--write')
        write = int(args[i + 1])
        del args[i:i + 2]
    mod, header, marker, rng = args[:4]
    text = open(header).read()
    if marker + '\n' not in text:
        sys.exit('marker not found')
    tag = re.sub(r'\W', '', marker.split('(')[0].split()[-1] if marker.split() else 'x').lower()[:12]
    if write is not None:
        open(header, 'w').write(text.replace(marker + '\n', padding(tag, write) + marker + '\n', 1))
        print(f'{write} lines of padding written before {marker!r}')
        return
    lo, _, rest = rng.partition('-')
    hi, _, step = rest.partition('/')
    counts = range(int(lo), int(hi) + 1, int(step or 1))
    srcdir = os.path.join(ROOT, 'src', mod.split('.')[0].lower())
    srcs = [l.strip() for l in open(os.path.join(srcdir, 'objs.txt')) if l.strip() and not l.startswith('#')]

    def prepare(n):
        d = os.path.join(ROOT, 'build', 'scratch', 'hdrsweep', str(n))
        os.makedirs(d, exist_ok=True)
        open(os.path.join(d, os.path.basename(header)), 'w').write(
            text.replace(marker + '\n', padding(tag, n) + marker + '\n', 1))
        for s in srcs:
            shutil.copy(os.path.join(srcdir, s), d)
        return d

    def compile_one(job):
        n, s = job
        d = os.path.join(ROOT, 'build', 'scratch', 'hdrsweep', str(n))
        r = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, os.path.join(d, s)],
                           capture_output=True, text=True).stdout
        m = re.search(r'(\d+)/(\d+) functions match', r)
        matched = [x.group(1) for x in re.finditer(r'^0x\w+ ([\w:~]+): MATCH', r, re.M)]
        return n, s, int(m.group(1)) if m else 0, matched

    for n in counts:
        prepare(n)
    work = [(n, s) for n in counts for s in srcs]
    totals = {n: 0 for n in counts}
    names = {n: set() for n in counts}
    with ThreadPoolExecutor(jobs) as ex:
        for n, s, k, matched in ex.map(compile_one, work):
            totals[n] += k
            names[n].update(f'{s}:{x}' for x in matched)
    base = names[counts[0]]
    for n in counts:
        gained = sorted(names[n] - base)
        lost = sorted(base - names[n])
        print(f'{n:4d}: {totals[n]} match' + (f'  +{len(gained)} -{len(lost)}' if n != counts[0] else ''), flush=True)


if __name__ == '__main__':
    main()
