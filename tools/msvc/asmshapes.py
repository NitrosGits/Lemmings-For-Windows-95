#!/usr/bin/env python3
"""Compile every combination of a realname.py-style spec and group them by the instructions that
OUR build emits between two markers, so the set of code shapes the compiler can produce for a
construct is visible (and whether any of them equals the original's).

  asmshapes.py MODULE SOURCE ADDR NAME SPEC.py START_RE END_RE [--jobs 3] [--max 500]

START_RE / END_RE are regexes matched against our instruction text (left column of try.sh -v), e.g.
'call +DWORD PTR ds:0x10013288' and 'lea +eax,\\[esp\\+0x8\\]'. Output: one block per distinct shape
with the combination names that produce it; the shape equal to the original's (right column) is marked.
"""
import argparse, concurrent.futures, itertools, os, re, runpy, shutil, subprocess, collections

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    ap = argparse.ArgumentParser()
    for n in ('module', 'source', 'addr', 'name', 'spec', 'start', 'end'):
        ap.add_argument(n)
    ap.add_argument('--jobs', type=int, default=3); ap.add_argument('--max', type=int, default=500)
    a = ap.parse_args()
    spec = runpy.run_path(a.spec)
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    st0 = text.index('/* ' + a.addr); s = text.index('\n{\n', st0) + 3; e = text.index('\n}\n', s)
    body = text[s:e]
    slots = spec['SLOTS']
    choices = [[o] + alts for o, alts in slots]
    combos = sorted(itertools.product(*[range(len(c)) for c in choices]), key=lambda t: (sum(x > 0 for x in t), t))[:a.max]
    base = os.path.basename(src)
    sre, ere = re.compile(a.start), re.compile(a.end)

    def window(rows, col):
        out, on = [], False
        for l in rows:
            m = re.match(r'\s*\*?\s*([0-9a-f]{6,8})\s+(.*?)\s*\|\s*(.*)$', l) if col == 0 else None
            if col == 0 and not m:
                continue
            txt = (m.group(2) if col == 0 else '').strip()
            txt = re.sub(r'\s+', ' ', txt)
            if not on and sre.search(txt):
                on = True
            if on:
                out.append(re.sub(r'0x[0-9a-f]{6,}', 'ADDR', txt))
                if ere.search(txt):
                    break
        return tuple(out)

    def run(i_t):
        i, t = i_t
        b = body
        for (o, _), c, k in zip(slots, choices, t):
            b = b.replace(o, c[k], 1)
        d = os.path.join(ROOT, 'build/scratch/asmshapes', str(i)); os.makedirs(d, exist_ok=True)
        p = os.path.join(d, base)
        open(p, 'w', encoding='latin1').write(text[:s] + b + text[e:])
        o = subprocess.run(['zsh', os.path.join(ROOT, 'tools/msvc/try.sh'), a.module, p, '-v', '-f', a.name],
                           cwd=ROOT, capture_output=True, text=True).stdout.splitlines()
        shutil.rmtree(d, ignore_errors=True)
        ours = window(o, 0)
        orig = []
        on = False
        for l in o:
            m = re.match(r'\s*\*?\s*([0-9a-f]{6,8})\s+(.*?)\s*\|\s*(.*)$', l)
            if not m:
                continue
            txt = re.sub(r'\s+', ' ', m.group(3).strip())
            if not on and sre.search(txt):
                on = True
            if on:
                orig.append(re.sub(r'0x[0-9a-f]{6,}', 'ADDR', txt))
                if ere.search(txt):
                    break
        return 'c' + ''.join(map(str, t)), ours, tuple(orig)

    shapes = collections.OrderedDict()
    orig_shape = None
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
        for name, ours, orig in ex.map(run, enumerate(combos)):
            shapes.setdefault(ours, []).append(name)
            orig_shape = orig_shape or orig
    for sh, names in shapes.items():
        mark = '  <== EQUALS ORIGINAL' if sh == orig_shape else ''
        print(f'--- {len(names)} variants: {" ".join(names[:12])}{" ..." if len(names) > 12 else ""}{mark}')
        for l in sh:
            print('   ', l)
    print('--- original:')
    for l in orig_shape or ():
        print('   ', l)


if __name__ == '__main__':
    main()
