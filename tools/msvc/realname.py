#!/usr/bin/env python3
"""Like shapes.py, but compiles every variant under the REAL file name (the name changes results:
a body that matches as cN_p000.cpp can differ as dockwnd.cpp; see HANDOFF.md session 34).

  realname.py MODULE SOURCE ADDR SPEC.py [--header HDR] [--jobs 3] [--max N]

SPEC.py defines SLOTS = [(original_text, [alternative, ...]), ...] as for shapes.py. Each combination
is written to build/scratch/realname/<n>/<basename of SOURCE> (plus HDR copied beside it under its own
name, so a private window.h overrides src/include/window.h) and scored with tools/msvc/try.sh.
Prints the best variants by the function's differing instructions, then by the file's match count.
"""
import argparse, concurrent.futures, itertools, os, re, runpy, shutil, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr'); ap.add_argument('spec')
    ap.add_argument('--header'); ap.add_argument('--jobs', type=int, default=3); ap.add_argument('--max', type=int, default=500)
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

    def run(i_t):
        i, t = i_t
        b = body
        for (o, _), c, k in zip(slots, choices, t):
            b = b.replace(o, c[k], 1)
        d = os.path.join(ROOT, 'build/scratch/realname', str(i)); os.makedirs(d, exist_ok=True)
        if a.header: shutil.copy(a.header, os.path.join(d, os.path.basename(a.header)))
        p = os.path.join(d, base)
        open(p, 'w', encoding='latin1').write(text[:s] + b + text[e:])
        o = subprocess.run(['zsh', os.path.join(ROOT, 'tools/msvc/try.sh'), a.module, p], cwd=ROOT, capture_output=True, text=True).stdout
        shutil.rmtree(d, ignore_errors=True)
        line = next((l for l in o.splitlines() if re.match(re.escape(a.addr) + r'\s', l)), '')
        m = re.search(r'(\d+) insns differ', line)
        ins = 0 if ': MATCH' in line else int(m.group(1)) if m else 9999
        tm = re.search(r'(\d+)/(\d+) functions match', o)
        return ins, -(int(tm.group(1)) if tm else 0), 'c' + ''.join(map(str, t))

    with concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
        res = sorted(ex.map(run, enumerate(combos)))
    for ins, nf, name in res[:10]:
        print(f'{ins:5} insns  file {-nf:3}  {name}')


if __name__ == '__main__':
    main()
