#!/usr/bin/env python3
"""Hill-climb a function's local declaration order (and padding) toward a match.

  declclimb.py MODULE SOURCE 'SIGNATURE' BODY.txt [--pads 25-29] [--rounds 20] [--jobs 5] [--out DIR]

BODY.txt holds one function body (the text between the braces, as variants.py takes it). Its
leading block of declaration lines (up to the first blank line) is what gets reordered. Each round
builds every pairwise swap of two declarations (plus every move of one declaration to the front)
at each padding count, keeps the best by (instructions, bytes), and stops when a round does not
improve or the function matches. Declarations with initialisers that use another declared name
keep their relative order. Uses tools/msvc/variants.py; never run two searches on the same function
at once (variants.py names its scratch files after the function).
"""
import argparse, itertools, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))


def split(body):
    lines = body.strip('\n').split('\n')
    k = 0
    decl = []
    while k < len(lines) and lines[k].strip():
        decl.append(lines[k]); k += 1
    return decl, '\n'.join(lines[k:])


def ok(order):
    names = [re.search(r'(\w+)(\[[^\]]*\])?\s*(=|;)', d).group(1) for d in order]
    for i, d in enumerate(order):
        if '=' in d:
            rhs = d.split('=', 1)[1]
            for j, n in enumerate(names):
                if j > i and re.search(r'(?<![.>\w])%s\b' % n, rhs):
                    return False
    return True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('sig'); ap.add_argument('body')
    ap.add_argument('--pads', default='0'); ap.add_argument('--rounds', type=int, default=20)
    ap.add_argument('--jobs', type=int, default=5); ap.add_argument('--out', default=None)
    a = ap.parse_args()
    out = a.out or os.path.join(ROOT, 'build', 'scratch', 'declclimb')
    os.makedirs(out, exist_ok=True)
    decl, rest = split(open(a.body, encoding='latin1').read())
    lo, _, hi = a.pads.partition('-')
    pads = list(range(int(lo), int(hi or lo) + 1))
    best = None
    cur = list(decl)
    for rnd in range(a.rounds):
        cands = [cur]
        for i, j in itertools.combinations(range(len(cur)), 2):
            c = list(cur); c[i], c[j] = c[j], c[i]
            if ok(c): cands.append(c)
        for i in range(1, len(cur)):
            c = [cur[i]] + cur[:i] + cur[i + 1:]
            if ok(c) and c not in cands: cands.append(c)
        bf = os.path.join(out, 'round%d.txt' % rnd)
        open(bf, 'w', encoding='latin1').write('\n%%\n'.join('\n' + '\n'.join(c) + '\n\n' + rest + '\n' for c in cands))
        r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'msvc', 'variants.py'), a.module, a.source, a.sig, bf,
                            '--pads', '%d-%d' % (pads[0], pads[-1]), '--jobs', str(a.jobs)], capture_output=True, text=True).stdout
        m = re.search(r'^\s+(\d+)\s+(MATCH|(\d+)/(\d+))\s+pad\s+(\d+)', r, re.M)
        if not m:
            print('no result', r[:300]); break
        k = int(m.group(1)); ins = 0 if m.group(2) == 'MATCH' else int(m.group(3))
        byt = 0 if m.group(2) == 'MATCH' else int(m.group(4)); pad = int(m.group(5))
        score = (ins, byt)
        body_i = k // len(pads)
        print('round', rnd, 'best', score, 'pad', pad, flush=True)
        if best is not None and score >= best[0]:
            break
        best = (score, pad, cands[body_i])
        cur = cands[body_i]
        open(os.path.join(out, 'best.txt'), 'w', encoding='latin1').write('\n' + '\n'.join(cur) + '\n\n' + rest + '\n')
        print('\n'.join(cur), flush=True)
        if ins == 0:
            break
    print('final', best[0] if best else None, 'pad', best[1] if best else None)


if __name__ == '__main__':
    main()
