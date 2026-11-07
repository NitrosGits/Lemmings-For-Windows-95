#!/usr/bin/env python3
"""Find differences that may change behaviour, not just register allocation:

  semdiff.py MODULE SOURCE...

For every unmatched function, compares the aligned instruction pairs that have
the same mnemonic with register names blanked out. What still differs is a
struct field offset, an immediate or a different global: possibly a real bug
in the decompiled source (IconWnd::OnCreate advanced the wrong RECT field and
was reported as a 3-byte "same shape" near miss). Differences only in
esp/ebp-relative offsets are listed separately (locals laid out differently
is usually register allocation too, but not always).
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
REGS = r'\b(e?[abcd]x|[abcd][lh]|e?[sd]i|e?[sb]p)\b'


def norm(ops):
    return re.sub(REGS, 'R', ops)


def parse(stdout):
    """{function: (original instructions, ours)} for unmatched functions, from match.py -v."""
    out, func = {}, None
    for line in stdout.splitlines():
        m = re.match(r'^0x[0-9a-f]+ (\S+): (DIFF|MATCH)', line)
        if m:
            func = m.group(1) if m.group(2) == 'DIFF' else None
            if func:
                out[func] = ([], [])
            continue
        if not func or not re.match(r'^   [ *] [0-9a-f]{8}', line):
            continue
        body = line[5:]
        if '|' not in body:
            continue
        left, right = body.split('|', 1)
        left = re.sub(r'^[0-9a-f]{8}\s*', '', left.strip())
        for side, ins in ((0, left), (1, right.strip())):
            if ins and not ins.startswith('.byte'):
                out[func][side].append(ins)
    return out


def key(ins):
    m, _, ops = ins.partition(' ')
    ops = ops.strip()
    if m.startswith('j') or m == 'call' and not 'ds:' in ops:
        ops = ''                                    # branch targets move with the code
    ops = norm(ops)
    stack = bool(re.search(r'\[(esp|ebp)', ins))
    if stack:
        ops = re.sub(r'\[R\+0x[0-9a-f]+\]|\[R\]', '[stack]', ops)
    return m + ' ' + ops


def main():
    mod = sys.argv[1]
    total = 0
    for src in sys.argv[2:]:
        r = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, src, '-v'], capture_output=True, text=True)
        for f, (orig, ours) in parse(r.stdout).items():
            a, b = {}, {}
            for i in orig:
                a.setdefault(key(i), []).append(i)
            for i in ours:
                b.setdefault(key(i), []).append(i)
            only_a = [x for k, v in a.items() for x in v[len(b.get(k, [])):]]
            only_b = [x for k, v in b.items() for x in v[len(a.get(k, [])):]]
            # pure register-to-register moves and push/pop of registers come and go with allocation
            boring = lambda i: re.fullmatch(r'(mov|push|pop|xchg|xor|or|test)\s+' + REGS + r'(,' + REGS + r')?', i.replace(' ', '').replace('\t', '')[:0] + i.split()[0] + ' ' + ''.join(i.split()[1:])) is not None
            only_a = [i for i in only_a if not boring(i)]
            only_b = [i for i in only_b if not boring(i)]
            if not only_a and not only_b:
                continue
            total += 1
            print(f'== {os.path.basename(src)} {f}: original-only {len(only_a)}, ours-only {len(only_b)}')
            for i in only_a[:6]:
                print(f'   original: {i}')
            for i in only_b[:6]:
                print(f'   ours:     {i}')
    print(f'{total} functions whose instructions differ beyond register allocation')


if __name__ == '__main__':
    main()
