#!/usr/bin/env python3
"""Try a function's statements in other orders: each statement given with
--move (a substring of its line) is tried at every position among the
function body's other top-level statement lines, every combination, and each
order is scored with variants.py.

  stmtmove.py MODULE SOURCE 'signature line' --move TEXT [--move TEXT ...]
              [--first N] [variants.py options: --jobs, --pads, --show, --apply]

Only the body's first N lines (default: up to its first blank line after the
declarations, i.e. a run of simple statements) take part.
"""
import itertools
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    args = sys.argv[1:]
    moves = []
    while '--move' in args:
        i = args.index('--move')
        moves.append(args[i + 1])
        del args[i:i + 2]
    first = None
    if '--first' in args:
        i = args.index('--first')
        first = int(args[i + 1])
        del args[i:i + 2]
    mod, src, sig = args[:3]
    rest = args[3:]
    s = open(src).read()
    start = s.index(sig + '\n{\n') + len(sig) + 3
    end = s.index('\n}\n', start)
    body = s[start:end].split('\n')
    # the declarations stay first
    decl = 0
    while decl < len(body) and body[decl].strip() and body[decl].startswith('    ') and \
            not body[decl].startswith('     ') and '=' not in body[decl].split('(')[0] and \
            body[decl].rstrip().endswith(';') and ' ' in body[decl].strip() and \
            not body[decl].strip().startswith(('return', 'if', 'for', 'while')) and \
            '->' not in body[decl] and '(' not in body[decl]:
        decl += 1
    head = body[:decl]
    while decl < len(body) and not body[decl].strip():
        head.append(body[decl])
        decl += 1
    stmts = body[decl:first and decl + first or None]
    tail = body[decl + len(stmts):] if first else []
    moving = []
    for m in moves:
        hits = [l for l in stmts if m in l]
        if len(hits) != 1:
            sys.exit(f'--move {m!r}: {len(hits)} lines match')
        moving.append(hits[0])
    fixed = [l for l in stmts if l not in moving]
    bodies = []
    for positions in itertools.product(range(len(fixed) + 1), repeat=len(moving)):
        order = list(fixed)
        # insert from the last position down so earlier indices stay valid
        for pos, line in sorted(zip(positions, moving), key=lambda t: -t[0]):
            order.insert(pos, line)
        text = '\n'.join(head + order + tail)
        if text not in bodies:
            bodies.append(text)
    path = os.path.join(ROOT, 'build', 'scratch', 'stmtmove.txt')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'w').write('\n%%\n'.join(bodies) + '\n')
    print(f'{len(bodies)} orders', flush=True)
    sys.exit(subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'msvc', 'variants.py'), mod, src, sig, path]
                            + rest).returncode)


if __name__ == '__main__':
    main()
