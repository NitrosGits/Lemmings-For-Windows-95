#!/usr/bin/env python3
"""Search the order of a function's local declarations for a byte match.

Visual C++ 2.x picks registers partly by symbol order, so the order locals are
declared in (and how many symbols precede the function) changes the code. This
splits the declarations at the top of one function into single declarators,
tries random reorderings (and, with --pad, a number of dummy declarations
before the function), compiles each and keeps the best. The best version is
written to build/scratch/permute/; with --write it replaces the source.

  permute.py MODULE SOURCE FUNCNAME [--iters N] [--pad MAX] [--write] [--seed S] [--whole]

The function's first block of lines after '{', up to the first blank line, must
be its declarations (initialisers are kept with their variable).
"""
import os
import random
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FLAGS = os.environ.get('MSVCFLAGS', '/Ox')


def split_decls(block):
    """'int a, *b = 0;' -> ['int a;', 'int *b = 0;'] (comments dropped)."""
    out = []
    block = re.sub(r'/\*.*?\*/', '', block, flags=re.S)
    for stmt in block.split(';'):
        stmt = stmt.strip()
        if not stmt:
            continue
        m = re.match(r'((?:(?:const|unsigned|signed|static|register|struct|volatile)\s+)*\w+)\s+(.*)', stmt, re.S)
        if not m:
            raise SystemExit(f'cannot split declaration: {stmt}')
        base, rest = m.group(1), m.group(2)
        depth = 0
        cur = ''
        parts = []
        for ch in rest:
            if ch in '([{':
                depth += 1
            elif ch in ')]}':
                depth -= 1
            if ch == ',' and depth == 0:
                parts.append(cur)
                cur = ''
            else:
                cur += ch
        parts.append(cur)
        for p in parts:
            out.append(f'{base} {p.strip()};')
    return out


WHOLE = False


def score(mod, path, name):
    """Differing bytes in the target function and every function before it in
    the file (so fixing one can't break another above it; padding further down
    can compensate for the ones below), the target's own count weighted more.
    With --whole, over every function in the file."""
    r = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, path],
                       capture_output=True, text=True, env=dict(os.environ, MSVCFLAGS=FLAGS))
    total = 0
    seen = False
    for m in re.finditer(r'^0x\w+ ([\w:~]+): (MATCH|not in object|DIFF \((\d+) bytes\)(?: (\d+) bytes differ)?)',
                         r.stdout, re.M):
        if m.group(2) == 'MATCH':
            n = 0
        elif m.group(2) == 'not in object':
            n = 1000
        else:
            n = int(m.group(4)) if m.group(4) else 1
        if m.group(1) == name:
            seen = True
            n = n * 2
        total += n
        if seen and not WHOLE:
            break
    return total if seen else 10 ** 6


def main():
    args = sys.argv[1:]
    opt = {'--iters': 60, '--pad': 0, '--seed': 1}
    flags = set()
    for k in list(opt):
        if k in args:
            i = args.index(k)
            opt[k] = int(args[i + 1])
            del args[i:i + 2]
    global WHOLE
    if '--whole' in args:
        WHOLE = True
        args.remove('--whole')
    for k in ['--write']:
        if k in args:
            flags.add(k)
            args.remove(k)
    mod, src, name = args
    random.seed(opt['--seed'])
    text = open(src).read()
    m = re.search(rf'^[^\n;{{}}]*\b{name}\s*\([^)]*\)\s*\n\{{\n', text, re.M)
    if not m:
        raise SystemExit(f'{name}: definition not found')
    body = m.end()
    blank = text.index('\n\n', body)
    decls = split_decls(text[body:blank])
    comment_start = text.rfind('\n/*', 0, m.start()) + 1
    out = os.path.join(ROOT, 'build', 'scratch', 'permute', os.path.basename(src))
    os.makedirs(os.path.dirname(out), exist_ok=True)

    tag = re.sub(r'\W', '', name).lower()

    def build(order, pad):
        padtext = ''
        if pad:
            padtext = ('/*\n * Symbol padding (see docs/DECOMP.md): the original had more declarations\n'
                       ' * before this point, and Visual C++ 2.x picks registers by symbol position.\n */\n'
                       'extern int ' + ', '.join(f'pad_{tag}{i}' for i in range(pad)) + ';\n\n')
        return (text[:comment_start] + padtext + text[comment_start:body] +
                ''.join(f'    {d}\n' for d in order) + text[blank + 1:])

    best = list(decls)
    best_pad = 0
    open(out, 'w').write(build(best, 0))
    best_score = start_score = score(mod, out, name)
    print(f'start: {best_score}', flush=True)
    for it in range(opt['--iters']):
        if best_score == 0:
            break
        cand = list(best)
        for _ in range(random.randint(1, 3)):
            i, j = random.randrange(len(cand)), random.randrange(len(cand))
            cand[i], cand[j] = cand[j], cand[i]
        # declarations with initialisers run in order: keep theirs as it was
        inits = [d for d in decls if '=' in d]
        it_inits = iter(inits)
        cand = [next(it_inits) if '=' in d else d for d in cand]
        pad = best_pad
        if opt['--pad'] and random.random() < 0.3:
            pad = random.randint(0, opt['--pad'])
        open(out, 'w').write(build(cand, pad))
        s = score(mod, out, name)
        if s <= best_score:
            if s < best_score:
                print(f'{it}: {s} (pad {pad})', flush=True)
            best, best_pad, best_score = cand, pad, s
    final = build(best, best_pad)
    open(out, 'w').write(final)
    print(f'best: score {best_score} (file total), pad {best_pad}; {out}')
    print(''.join(f'    {d}\n' for d in best))
    if '--write' in flags and best_score < start_score:
        open(src, 'w').write(final)
        print('written to', src)


if __name__ == '__main__':
    main()
