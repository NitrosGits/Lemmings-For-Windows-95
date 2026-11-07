#!/usr/bin/env python3
"""Sweep symbol padding at the top of a source file, before its first
#include, so that everything the headers declare shifts: for each count,
compile and list the functions that match.

  topsweep.py MODULE SOURCE A-B[/STEP] [--jobs N] [--write N]

--write N puts N padding lines (pad_top...) at the top of SOURCE, replacing
any there already.
"""
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOP_RE = re.compile(r'/\* Symbol padding[^\n]*\*/\nextern int pad_top\w*(?:, pad_top\w*)*;\n\n?')


def with_top(text, n):
    text = TOP_RE.sub('', text, count=1)
    if not n:
        return text
    pad = ('/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */\n'
           'extern int ' + ', '.join(f'pad_top{i}' for i in range(n)) + ';\n\n')
    i = text.index('#include')
    return text[:i] + pad + text[i:]


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
    mod, src = args[:2]
    text = open(src).read()
    if write is not None:
        open(src, 'w').write(with_top(text, write))
        print(f'{write} lines of top padding written')
        return
    lo, _, rest = args[2].partition('-')
    hi, _, step = rest.partition('/')
    counts = range(int(lo), int(hi) + 1, int(step or 1))
    name = os.path.splitext(os.path.basename(src))[0]
    outdir = os.path.join(ROOT, 'build', 'scratch', 'topsweep')
    os.makedirs(outdir, exist_ok=True)

    def run(n):
        d = os.path.join(outdir, f'{name}{n}')
        os.makedirs(d, exist_ok=True)
        out = os.path.join(d, os.path.basename(src))
        open(out, 'w').write(with_top(text, n))
        r = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, out],
                           capture_output=True, text=True).stdout
        return n, [m.group(1) for m in re.finditer(r'^0x\w+ ([\w:~]+): MATCH', r, re.M)], \
            len(re.findall(r'^0x\w+ ', r, re.M))

    results = []
    with ThreadPoolExecutor(jobs) as ex:
        for n, matched, total in ex.map(run, counts):
            results.append((n, matched, total))
            print(f'{n:4d}: {len(matched)}/{total}', flush=True)
    best = max(results, key=lambda r: len(r[1]))
    print(f'best {best[0]}: {len(best[1])}/{best[2]}: {" ".join(best[1])}')


if __name__ == '__main__':
    main()
