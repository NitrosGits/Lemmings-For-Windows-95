#!/usr/bin/env python3
"""Try several bodies for one function, in parallel, and score each:

  variants.py MODULE SOURCE 'signature line' BODIESFILE [--pads A-B[/STEP]] [--jobs N] [--show K]

BODIESFILE holds the candidate bodies (the text between the function's braces),
separated by lines of just '%%'. A body may hold choices, [[ a || b || c ]]
(they can span lines): it stands for every combination of them. Each is
compiled with the rest of SOURCE and
scored by the number of instructions, then bytes, that differ from the
original (0: a match).
--pads A-B[/STEP] also tries each with A..B (every STEP) lines of symbol padding before the function
(see declsearch.py). --show K prints the diff of variant K instead; --apply K
writes variant K's body (without its padding) into SOURCE, one statement a line.
"""
import itertools
import os
import re
import subprocess
import sys
import threading
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def expand(body):
    """Every combination of the body's [[ a || b ]] choices."""
    parts = re.split(r'\[\[(.*?)\]\]', body, flags=re.S)
    if any('[[' in p for p in parts[1::2]):
        raise SystemExit('choices [[ ]] cannot be nested')
    fixed, choices = parts[0::2], [[c.strip(' ') for c in p.split('||')] for p in parts[1::2]]
    out = []
    for combo in itertools.product(*choices):
        out.append(''.join(f + c for f, c in zip(fixed, combo + ('',))))
    return out


def main():
    args = sys.argv[1:]
    jobs, show, pads = 6, None, range(0, 1)
    if '--pads' in args:
        i = args.index('--pads')
        lo, _, rest = args[i + 1].partition('-')       # A-B or A-B/STEP
        hi, _, step = rest.partition('/')
        pads = range(int(lo), int(hi) + 1, int(step or 1))
        del args[i:i + 2]
    if '--jobs' in args:
        i = args.index('--jobs')
        jobs = int(args[i + 1])
        del args[i:i + 2]
    apply = None
    if '--apply' in args:
        i = args.index('--apply')
        apply = int(args[i + 1])
        del args[i:i + 2]
    if '--show' in args:
        i = args.index('--show')
        show = int(args[i + 1])
        del args[i:i + 2]
    mod, src, sig, bodyfile = args[:4]
    s = open(src).read()
    start = s.index(sig + '\n{')
    end = s.index('\n}\n', start)
    bodies = [b.strip('\n') for b in re.split(r'^%%\s*$', open(bodyfile).read(), flags=re.M)]
    bodies = [x for b in bodies if b.strip() for x in expand(b)]
    work = [(b, p) for b in bodies for p in pads]
    comment_start = s.rfind('\n/*', 0, start) + 1
    name = sig.split('(')[0].split()[-1].lstrip('*')
    fname = name.replace('::', '_').replace('~', '')
    outdir = os.path.join(ROOT, 'build', 'scratch', 'variants')
    os.makedirs(outdir, exist_ok=True)

    local = threading.local()
    slots = iter(range(1000))

    def run(k):
        body, pad = work[k]
        if not hasattr(local, 'slot'):
            local.slot = next(slots)            # one scratch file per worker thread
        out = os.path.join(outdir, '%s_%d%s' % (fname, local.slot, os.path.splitext(src)[1]))
        padtext = ''
        if pad:
            padtext = ('/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */\n'
                       'extern int ' + ', '.join('pad_%s%d' % (fname.lower(), i) for i in range(pad)) + ';\n\n')
        open(out, 'w').write(s[:comment_start] + padtext + s[comment_start:start] + sig + '\n{\n' + body + s[end:])
        extra = ['-v'] if show is not None else []
        r = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, out, '-f', name] + extra,
                           capture_output=True, text=True).stdout
        first = r.split('\n')[0]
        if ': MATCH' in first:
            return k, (0, 0), r
        m = re.search(r'(\d+) bytes differ.*?(\d+) insns differ', first)
        return k, (int(m.group(2)), int(m.group(1))) if m else (99999, 99999), r

    if show is not None:
        print(run(show)[2].rstrip())
        return
    if apply is not None:
        lines = []
        for line in work[apply][0].split('\n'):
            ind = line[:len(line) - len(line.lstrip())]
            parts, cur, depth = [], '', 0       # split at '; ' outside parentheses
            text = line.strip()
            for i, ch in enumerate(text):
                depth += (ch == '(') - (ch == ')')
                cur += ch
                if ch == ';' and depth == 0 and text[i + 1:i + 2] == ' ':
                    parts.append(cur)
                    cur = ''
            parts.append(cur.lstrip())
            lines += [ind + x.strip() if x.strip() else '' for x in parts]
        open(src, 'w').write(s[:start] + sig + '\n{\n' + '\n'.join(lines) + s[end:])
        print('variant %d written to %s' % (apply, src))
        return
    results = []
    with ThreadPoolExecutor(jobs) as ex:
        for k, score, r in ex.map(run, range(len(work))):
            results.append((score, k))
    results.sort()
    print(f'{len(work)} variants; best:')
    for score, k in results[:8]:
        print('%4d  %9s  pad %d' % (k, 'MATCH' if score == (0, 0) else '%d/%d' % score, work[k][1]))
    if results:
        print('---- best (%d, pad %d) ----' % (results[0][1], work[results[0][1]][1]))
        print(work[results[0][1]][0])


if __name__ == '__main__':
    main()
