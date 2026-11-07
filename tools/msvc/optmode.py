#!/usr/bin/env python3
"""Per-function optimisation-mode probe: wrap ONE function in a `#pragma optimize` and see
whether it then matches the original (the SoundInit finding: its original code is what VC 2.2
emits with /Os or `#pragma optimize("t", off)`, not with the project's /Ox).

  optmode.py MODULE SOURCE ADDR [--modes base,s,t-off] [--jobs 8] [--out DIR]
  optmode.py --list-modes

Each mode compiles a copy of SOURCE with the pragma before `/* ADDR` and a restoring pragma after
the function, via tools/msvc/try.sh, and prints the function's result line plus the file's match
count. Caveats:
  * a size-mode function has no trailing `npad`, so every later function in the module moves by the
    missing padding bytes; try.sh then shows the NEXT function as DIFF (its extent now includes the
    padding it no longer has). That is not a real regression of the neighbour - the padding still
    has to be put back before the full link can match (see SoundInit-handoff.md).
  * `#pragma optimize` is rejected inside a function body (C2156), so it can only go between functions.
"""
import argparse, concurrent.futures, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MODES = {
    'base': ('', ''),
    's': ('#pragma optimize("s", on)\n', '#pragma optimize("", on)\n'),
    't-off': ('#pragma optimize("t", off)\n', '#pragma optimize("t", on)\n'),
    'i-off': ('#pragma optimize("i", off)\n', '#pragma optimize("i", on)\n'),
    'g-off': ('#pragma optimize("g", off)\n', '#pragma optimize("g", on)\n'),
    'y-off': ('#pragma optimize("y", off)\n', '#pragma optimize("y", on)\n'),
    'a': ('#pragma optimize("a", on)\n', '#pragma optimize("a", off)\n'),
    'w': ('#pragma optimize("w", on)\n', '#pragma optimize("w", off)\n'),
    'g-off-e': ('#pragma optimize("g", off)\n#pragma optimize("e", on)\n', '#pragma optimize("", on)\n'),
    'ge-off': ('#pragma optimize("ge", off)\n', '#pragma optimize("", on)\n'),
    'e-on': ('#pragma optimize("e", on)\n', '#pragma optimize("", on)\n'),
    'e-off': ('#pragma optimize("e", off)\n', '#pragma optimize("", on)\n'),
    'all-off': ('#pragma optimize("", off)\n', '#pragma optimize("", on)\n'),
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module', nargs='?'); ap.add_argument('source', nargs='?'); ap.add_argument('addr', nargs='?')
    ap.add_argument('--modes', default='base,s,t-off'); ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--out'); ap.add_argument('--list-modes', action='store_true')
    a = ap.parse_args()
    if a.list_modes:
        for n, (b, e) in MODES.items():
            print(f'{n:6} {b.strip()!r} ... {e.strip()!r}')
        return
    if not (a.module and a.source and a.addr):
        ap.error('MODULE SOURCE ADDR required')
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    start = text.index('/* ' + a.addr)
    end = text.index('\n}\n', start) + 3
    out = a.out or os.path.join(ROOT, 'build/scratch/optmode', re.sub(r'\W', '_', a.addr))
    os.makedirs(out, exist_ok=True)
    ext = os.path.splitext(src)[1]
    fn = re.compile(r'^' + re.escape(a.addr) + r'\s')

    def run(mode):
        before, after = MODES[mode]
        path = os.path.join(out, mode.replace('-', '_') + ext)
        open(path, 'w', encoding='latin1').write(text[:start] + before + text[start:end] + after + text[end:])
        r = subprocess.run(['zsh', os.path.join(ROOT, 'tools/msvc/try.sh'), a.module, path],
                           cwd=ROOT, capture_output=True, text=True, timeout=600)
        o = r.stdout + r.stderr
        line = next((l for l in o.splitlines() if fn.match(l)), '')
        tm = re.search(r'(\d+)/(\d+) functions match', o)
        if 'error' in o and not line:
            line = next((l for l in o.splitlines() if 'error' in l), 'compile error')
        return mode, line[:110], (tm.group(0) if tm else '?')

    modes = [m.strip() for m in a.modes.split(',')]
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as pool:
        for mode, line, total in pool.map(run, modes):
            print(f'{mode:6} {total:24} {line}')


if __name__ == '__main__':
    main()
