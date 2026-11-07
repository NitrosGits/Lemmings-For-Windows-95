#!/usr/bin/env python3
"""Exhaustive version of permute.py for functions with few locals: every order
of the function's declarations, with 0..16 lines of symbol padding before it
(the effect of padding repeats every 17 symbols), compiled in parallel and
scored on the function alone (instructions, then bytes, that differ).

  declsearch.py MODULE SOURCE FUNCNAME [--pads A-B[/STEP]] [--sample N | --climb N] [--jobs N] [--write]

--sample N tries N random (order, padding) pairs instead of all of them.
--climb N instead runs a hill climb from the current order at each padding:
N rounds of random swaps, keeping any that score no worse.

--write puts the best version (if it is a match) into SOURCE. Functions further
down the file may then need their own padding, so work top-down.
"""
import itertools
import math
import os
import random
import re
import subprocess
import sys
import threading
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from permute import split_decls  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def main():
    args = sys.argv[1:]
    jobs, pads, write, sample, climb = 6, range(0, 17), False, 0, 0
    if '--climb' in args:
        i = args.index('--climb')
        climb = int(args[i + 1])
        del args[i:i + 2]
    if '--sample' in args:
        i = args.index('--sample')
        sample = int(args[i + 1])
        del args[i:i + 2]
    if '--jobs' in args:
        i = args.index('--jobs')
        jobs = int(args[i + 1])
        del args[i:i + 2]
    if '--pads' in args:
        i = args.index('--pads')
        lo, _, rest = args[i + 1].partition('-')       # A-B or A-B/STEP
        hi, _, step = rest.partition('/')
        pads = range(int(lo), int(hi) + 1, int(step or 1))
        del args[i:i + 2]
    if '--write' in args:
        write = True
        args.remove('--write')
    mod, src, name = args
    text = open(src).read()
    m = re.search(rf'^[^\n;{{}}]*\b{name}\s*\([^)]*\)\s*\n\{{\n', text, re.M)
    if not m:
        raise SystemExit(f'{name}: definition not found')
    body = m.end()
    blank = text.index('\n\n', body)
    decls = split_decls(text[body:blank])
    comment_start = text.rfind('\n/*', 0, m.start()) + 1
    tag = re.sub(r'\W', '', name).lower()
    outdir = os.path.join(ROOT, 'build', 'scratch', 'declsearch')
    os.makedirs(outdir, exist_ok=True)

    def build(order, pad):
        padtext = ''
        if pad:
            padtext = ('/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */\n'
                       'extern int ' + ', '.join(f'pad_{tag}{i}' for i in range(pad)) + ';\n\n')
        return (text[:comment_start] + padtext + text[comment_start:body] +
                ''.join(f'    {d}\n' for d in order) + text[blank + 1:])

    # declarations with initialisers run in order: they keep theirs among themselves
    inits = [d for d in decls if '=' in d]

    def fix_inits(order):
        it = iter(inits)
        return tuple(next(it) if '=' in d else d for d in order)

    local = threading.local()
    slots = iter(range(1000))

    def evaluate(order, pad):
        if not hasattr(local, 'slot'):
            local.slot = next(slots)            # one scratch file per worker thread
        out = os.path.join(outdir, f'{tag}_{local.slot}' + os.path.splitext(src)[1])
        open(out, 'w').write(build(order, pad))
        r = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), mod, out, '-f', name],
                           capture_output=True, text=True).stdout
        first = r.split('\n')[0]
        if ': MATCH' in first:
            return (0, 0)
        mm = re.search(r'(\d+) bytes differ.*?(\d+) insns differ', first)
        return (int(mm.group(2)), int(mm.group(1))) if mm else (99999, 99999)

    results = []                                # (score, order, pad)
    if climb:
        def chain(pad):
            rnd = random.Random(pad)
            order = tuple(decls)
            score = evaluate(order, pad)
            for _ in range(climb):
                if score == (0, 0):
                    break
                cand = list(order)
                for _ in range(rnd.randint(1, 2)):
                    i, j = rnd.randrange(len(cand)), rnd.randrange(len(cand))
                    cand[i], cand[j] = cand[j], cand[i]
                cand = fix_inits(cand)
                sc = evaluate(cand, pad)
                if sc <= score:
                    order, score = cand, sc
            return score, order, pad
        print(f'climbing {climb} rounds at pads {pads.start}-{pads.stop - 1}/{pads.step}', flush=True)
        with ThreadPoolExecutor(jobs) as ex:
            results = list(ex.map(chain, pads))
    else:
        seen = set()
        cands = []
        if sample and math.factorial(len(decls)) > 100000:
            # too many orders to list (16 locals = 2e13): draw random ones instead
            random.seed(1)
            work = []
            tries = 0
            while len(work) < sample and tries < sample * 20:
                tries += 1
                perm = list(decls)
                random.shuffle(perm)
                w = (fix_inits(perm), random.choice(list(pads)))
                if w not in seen:
                    seen.add(w)
                    work.append(w)
            cands = sorted({o for o, _ in work})
        else:
            for perm in itertools.permutations(decls):
                perm = fix_inits(perm)
                if perm not in seen:
                    seen.add(perm)
                    cands.append(perm)
            work = [(o, p) for p in pads for o in cands]
        if sample and sample < len(work):
            random.seed(1)
            work = random.sample(work, sample)
        print(f'{len(cands)} orders x {len(pads)} pads = {len(work)} builds', flush=True)
        with ThreadPoolExecutor(jobs) as ex:
            for i in range(0, len(work), jobs * 4):
                batch = work[i:i + jobs * 4]
                for (order, pad), score in zip(batch, ex.map(lambda w: evaluate(*w), batch)):
                    results.append((score, order, pad))
                if min(results)[0] == (0, 0):
                    break
    results.sort()
    for score, order, pad in results[:5]:
        print('%-9s pad %2d  %s' % ('MATCH' if score == (0, 0) else '%d/%d' % score, pad, ' '.join(order)))
    best_score, order, pad = results[0]
    if write and best_score == (0, 0):
        open(src, 'w').write(build(order, pad))
        print('written to', src)


if __name__ == '__main__':
    main()
