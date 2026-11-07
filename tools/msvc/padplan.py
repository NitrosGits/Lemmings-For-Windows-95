#!/usr/bin/env python3
"""Plan the symbol padding for a whole file, top-down.

Visual C++ 2.x's code for a function depends on how many symbols were declared
before it (see docs/DECOMP.md). The effect repeats, but the period differs from
place to place (17 and 32 have both been seen), so no shortcut is safe. Instead
this goes through the annotated functions in order, keeping everything above
the current one fixed: if it doesn't match, it tries 1..MAX lines of padding
just before it and keeps the first count that makes it match. Every choice is
checked with a real compile of the file as it will be written.

  padplan.py MODULE SOURCE [--max N] [--jobs N] [--start ADDRESS] [--write]

The file's existing padding lines are replaced, except the one after the
#includes (pad_top...), which is kept. With --start, the source prefix before
that annotated function is preserved and planning begins there. Without
--write it only reports.
"""
import os
import re
import subprocess
import sys
import threading
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PAD_RE = re.compile(r'^(?:/\* Symbol padding[^\n]*\*/\n|/\*\n \* Symbol padding.*?\*/\n)'
                    r'extern int pad_(?!top)\w+(?:, pad_\w+)*;\n\n?', re.M | re.S)


def strip_padding(text):
    """TEXT without its padding lines (the top one stays)."""
    return PAD_RE.sub('', text)


def padding(tag, n):
    return ('/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */\n'
            'extern int ' + ', '.join(f'pad_{tag}{i}' for i in range(n)) + ';\n\n')


def annotated(text):
    """(name, address, offset of its address comment) for each function, in order."""
    out = []
    for m in re.finditer(r'^/\* (0x[0-9a-f]+)\b[^\n]*\*/\n(?:[^\n]*\n)*?[^\n;{}]*?\b([\w:~]+)\(', text, re.M):
        out.append((m.group(2), int(m.group(1), 16), m.start()))
    return out


def main():
    args = sys.argv[1:]
    opt = {'--max': 40, '--jobs': 9}
    start_address = None
    if '--start' in args:
        i = args.index('--start')
        start_address = int(args[i + 1], 0)
        del args[i:i + 2]
    for k in list(opt):
        if k in args:
            i = args.index(k)
            opt[k] = int(args[i + 1])
            del args[i:i + 2]
    write = '--write' in args
    if write:
        args.remove('--write')
    mod, src = args
    source = open(src).read()
    if start_address is None:
        text = strip_padding(source)
        start_index = 0
    else:
        funcs = annotated(source)
        starts = [(i, at) for i, (_, address, at) in enumerate(funcs) if address == start_address]
        if not starts:
            raise SystemExit(f'function address {start_address:#x} not found')
        start_index, start_at = starts[0]
        text = source[:start_at] + strip_padding(source[start_at:])
    name = os.path.splitext(os.path.basename(src))[0]
    outdir = os.path.join(ROOT, 'build', 'scratch', 'padplan')
    os.makedirs(outdir, exist_ok=True)
    local = threading.local()
    slots = iter(range(1000))

    def matches(t, fn, address):
        if not hasattr(local, 'slot'):
            local.slot = next(slots)            # one scratch file per worker thread
        out = os.path.join(outdir, f'{name}_{local.slot}' + os.path.splitext(src)[1])
        open(out, 'w').write(t)
        r = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, out],
                           capture_output=True, text=True).stdout
        target = next((line for line in r.splitlines() if line.startswith(f'{address:#x} ')), '')
        return ': MATCH' in target

    lines = 0
    unmatched = []
    with ThreadPoolExecutor(opt['--jobs']) as ex:
        i = start_index
        while i < len(annotated(text)):
            fn, address, at = annotated(text)[i]
            i += 1
            if matches(text, fn, address):
                print(f'{fn:28} ok', flush=True)
                continue
            tag = re.sub(r'\W', '', fn).lower()
            found = None
            for first in range(1, opt['--max'] + 1, opt['--jobs']):
                ns = list(range(first, min(first + opt['--jobs'], opt['--max'] + 1)))
                cands = [text[:at] + padding(tag, n) + text[at:] for n in ns]
                for n, ok in zip(ns, ex.map(lambda t: matches(t, fn, address), cands)):
                    if ok:
                        found = n
                        break
                if found:
                    break
            if found:
                text = text[:at] + padding(tag, found) + text[at:]
                lines += 1
                print(f'{fn:28} pad {found}', flush=True)
            else:
                unmatched.append(fn)
                print(f'{fn:28} -', flush=True)
    total = len(annotated(text))
    planned = total - start_index
    prefix = f'from {start_address:#x}: ' if start_address is not None else ''
    print(f'{prefix}{planned - len(unmatched)}/{planned} functions match; {lines} padding lines; '
          f'unmatched: {" ".join(unmatched) or "none"}')
    if write:
        open(src, 'w').write(text)
        print('written to', src)


if __name__ == '__main__':
    main()
