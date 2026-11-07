#!/usr/bin/env python3
"""Try every order of each run of consecutive simple statements in a function:

  runperm.py MODULE SOURCE 'signature line' [--max-run N] [--pads A-B] [--jobs N] [--apply]

A run is consecutive lines at the same indentation that are each one simple
statement (an assignment, a call, ++/--, ending in ';'; no declarations,
control flow or braces). Runs of 2..N statements (default 5) are permuted one
run at a time, each order compiled with every padding (variants.py) and scored.
A byte match is the original's code, so a matching order is safe however the
statements depend on each other. --apply writes the best matching order.
"""
import itertools
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SIMPLE = re.compile(r'^(\s+)(?!return\b|if\b|for\b|while\b|do\b|switch\b|case\b|break\b|continue\b|goto\b|else\b)'
                    r'[A-Za-z_*(][^;{}]*;\s*$')
DECL = re.compile(r'^\s+(const\s+|static\s+|unsigned\s+|struct\s+)*[A-Z_a-z][\w]*(\s*\*)*\s+\**[A-Za-z_]\w*'
                  r'(\[[^]]*\])*\s*(=.*)?;\s*$')


def runs(lines, maxrun):
    out, i = [], 0
    while i < len(lines):
        m = SIMPLE.match(lines[i])
        if m and not DECL.match(lines[i]):
            ind, j = m.group(1), i + 1
            while j < len(lines):
                n = SIMPLE.match(lines[j])
                if not n or n.group(1) != ind or DECL.match(lines[j]):
                    break
                j += 1
            if 2 <= j - i <= maxrun:
                out.append((i, j))
            i = j
        else:
            i += 1
    return out


def main():
    args = sys.argv[1:]
    def opt(name, default):
        if name in args:
            k = args.index(name)
            v = args[k + 1]
            del args[k:k + 2]
            return v
        return default
    maxrun = int(opt('--max-run', 5))
    pads = opt('--pads', '0-15')
    jobs = opt('--jobs', '4')
    apply = '--apply' in args
    if apply:
        args.remove('--apply')
    mod, src, sig = args[:3]
    s = open(src).read()
    start = s.index(sig + '\n{\n') + len(sig) + 3
    end = s.index('\n}\n', start)
    lines = s[start:end].split('\n')
    best = None
    for a, b in runs(lines, maxrun):
        bodies = []
        for p in itertools.permutations(lines[a:b]):
            if list(p) != lines[a:b]:
                bodies.append(lines[:a] + list(p) + lines[b:])
        bodies.insert(0, lines)
        tmp = os.path.join(ROOT, 'build', 'scratch', 'runperm_bodies.txt')
        open(tmp, 'w').write('\n%%\n'.join('\n'.join(x) for x in bodies) + '\n')
        copy = os.path.join(ROOT, 'build', 'scratch', 'runperm_' + os.path.basename(src))
        open(copy, 'w').write(s)
        out = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'msvc', 'variants.py'), mod, copy, sig, tmp,
                              '--pads', pads, '--jobs', jobs], capture_output=True, text=True).stdout
        m = re.search(r'^\s+(\d+)\s+(MATCH|(\d+)/\d+)\s+pad\s+(\d+)', out, re.M)
        if not m:
            continue
        lo, hi = map(int, pads.split('-'))
        k = int(m.group(1))
        score = 0 if m.group(2) == 'MATCH' else int(m.group(3))
        body = bodies[k // (hi - lo + 1)]
        print(f'run lines {a}-{b - 1}: best {m.group(2)} at pad {m.group(4)}' +
              ('' if body is lines else ':\n' + '\n'.join(body[a:b])), flush=True)
        if best is None or score < best[0]:
            best = (score, body, int(m.group(4)))
    if best and best[0] == 0 and apply:
        s = s[:start] + '\n'.join(best[1]) + s[end:]
        open(src, 'w').write(s)
        print(f'written (needs {best[2]} lines of padding before the function: re-run padplan)')


if __name__ == '__main__':
    main()
