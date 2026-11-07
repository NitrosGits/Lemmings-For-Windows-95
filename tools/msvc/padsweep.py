#!/usr/bin/env python3
"""How the number of declarations before a file's code changes what matches.

Visual C++ 2.x breaks ties in register allocation by symbol-table position, so
code compiles differently depending on how many symbols were declared before it
(headers included). This inserts N dummy declarations after the last #include
and reports, for each N, which annotated functions match.

  padsweep.py MODULE SOURCE N0 N1 [STEP] [--before 0xADDR] [extra CL flags...]

--before puts the declarations just before that function's address comment
instead of after the includes.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    args = sys.argv[1:]
    before = None
    if '--before' in args:
        i = args.index('--before')
        before = args[i + 1]
        del args[i:i + 2]
    mod, src = args[0], args[1]
    n0, n1 = int(args[2]), int(args[3])
    step = int(args[4]) if len(args) > 4 else 1
    flags = ' '.join(args[5:]) or '/Ox'
    text = open(src).read()
    if before:
        last = re.search(r'^/\* ' + before + r'\b', text, re.M | re.I).start()
    else:
        last = max(m.end() for m in re.finditer(r'^#include .*\n', text, re.M))
    out = os.path.join(ROOT, 'build', 'scratch', 'pad', os.path.basename(src))
    os.makedirs(os.path.dirname(out), exist_ok=True)
    rows = []
    for n in range(n0, n1 + 1, step):
        pad = ''.join(f'extern int pad_{i};\n' for i in range(n))
        open(out, 'w').write(text[:last] + pad + text[last:])
        r = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, out],
                           capture_output=True, text=True, env=dict(os.environ, MSVCFLAGS=flags))
        bad = [m.group(1) for m in re.finditer(r'^0x\w+ (\w+): (?:DIFF|not in)', r.stdout, re.M)]
        total = re.search(r'(\d+)/(\d+) functions match', r.stdout)
        rows.append((n, total.group(0) if total else 'compile failed', bad))
        print(f'{n:4d}  {rows[-1][1]:28s}  {" ".join(bad)}', flush=True)


if __name__ == '__main__':
    main()
