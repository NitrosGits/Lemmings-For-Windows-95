#!/usr/bin/env python3
"""Expression-shape search for one function:

  shapes.py MODULE SOURCE ADDR SPEC.py [--pads 0,7,15,31] [--jobs 8] [--out DIR] [--max N]

SPEC.py defines SLOTS = [(original_text, [alternative, ...]), ...] (each original_text must
occur in the function body; the original itself is always the first choice) and optionally
EXTRA = {name: body_transform(body) -> body}. Every combination of slot choices (capped at
--max, smallest changes first) is compiled with each padding count; results are ranked like
lifetimes.py.
"""
import argparse, concurrent.futures, itertools, json, os, re, runpy, subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr'); ap.add_argument('spec')
    ap.add_argument('--pads', default='0,7,15,31'); ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--out'); ap.add_argument('--max', type=int, default=3000)
    a = ap.parse_args()
    spec = runpy.run_path(a.spec)
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    ann = text.index('/* ' + a.addr)
    s = text.index('\n{\n', ann) + 3
    e = text.index('\n}\n', s)
    body = text[s:e]
    slots = spec['SLOTS']
    for orig, _ in slots:
        assert orig in body, 'not in body: ' + orig
    choices = [[orig] + alts for orig, alts in slots]
    combos = sorted(itertools.product(*[range(len(c)) for c in choices]), key=lambda t: (sum(x > 0 for x in t), t))
    vs = {}
    for t in combos:
        b = body
        for (orig, _), c, k in zip(slots, choices, t):
            b = b.replace(orig, c[k], 1)
        vs.setdefault(b, 'c' + ''.join(map(str, t)))
        if len(vs) >= a.max:
            break
    for name, f in spec.get('EXTRA', {}).items():
        vs.setdefault(f(body), name)
    vs = {n: b for b, n in vs.items()}
    out = a.out or os.path.join(ROOT, 'build/scratch/shapes', a.addr)
    os.makedirs(out, exist_ok=True)
    json.dump({'slots': slots, 'bodies': vs}, open(os.path.join(out, 'bodies.json'), 'w'), indent=1)
    pads = [int(p) for p in a.pads.split(',')]
    cases = [(n, b, p) for n, b in vs.items() for p in pads]
    ext = os.path.splitext(src)[1]

    def run(case):
        n, b, p = case
        path = os.path.join(out, f'{n}_p{p:03d}{ext}')
        t = text[:s] + b + text[e:]
        if p:
            t = t[:ann] + 'extern int ' + ', '.join(f'pad_sh_{k}' for k in range(p)) + ';\n' + t[ann:]
        open(path, 'w', encoding='latin1').write(t)
        r = subprocess.run(['zsh', os.path.join(ROOT, 'tools/msvc/try.sh'), a.module, path],
                           cwd=ROOT, capture_output=True, text=True, timeout=300)
        o = r.stdout + r.stderr
        os.remove(path)
        line = next((l for l in o.splitlines() if l.startswith(a.addr + ' ')), '')
        m = re.search(r'(\d+) insns differ', line)
        ins = 0 if ': MATCH' in line else int(m.group(1)) if m else 9999
        tm = re.search(r'(\d+)/(\d+) functions match', o)
        return {'name': n, 'pad': p, 'insns': ins, 'file': int(tm.group(1)) if tm else -1, 'line': line[:160]}

    print(f'{a.addr}: {len(vs)} bodies x {len(pads)} pads = {len(cases)} builds', flush=True)
    res = []
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as pool:
        for k, r in enumerate(pool.map(run, cases), 1):
            res.append(r)
            if r['insns'] == 0:
                print('MATCH', json.dumps(r), flush=True)
            if k % 100 == 0:
                print(f'{k}/{len(cases)} best {min(x["insns"] for x in res)}', flush=True)
    res.sort(key=lambda r: (r['insns'], -r['file'], r['pad']))
    json.dump(res, open(os.path.join(out, 'results.json'), 'w'), indent=1)
    base = next((r for r in res if r['name'] == 'c' + '0' * len(slots) and r['pad'] == 0), None)
    print('baseline', json.dumps(base))
    for r in res[:12]:
        print(f"{r['insns']:5} file {r['file']:3}  pad {r['pad']:2}  {r['name']}")


if __name__ == '__main__':
    main()
