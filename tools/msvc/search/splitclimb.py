#!/usr/bin/env python3
"""Split a function's initialised / mid-block declarations, then hill-climb the declaration order.

  splitclimb.py MODULE SOURCE ADDR [--pads 0-3] [--rounds 10] [--jobs 6] [--body FILE] [--apply]

The recipe that matched Map::FillObjects (session 33): every `T v = expr;` at function level becomes
`T v;` in one top block plus `v = expr;` at its original place (statement order unchanged), C++
mid-block declarations at function level are hoisted the same way, `int a, b;` is split into one
line each; locals inside nested `{ }` blocks stay put. Then every pairwise swap / move-to-front of
the top block is tried with 0..N extra padding lines before the function, keeping the best by
(function's differing instructions, file's unmatched count), until no round improves.

Unlike variants.py/declclimb.py every variant is compiled under the REAL file name, in its own
directory (build/scratch/splitclimb/<k>/<basename>): the file name changes the generated code
(session 34). ADDR is the function's `/* 0x...` comment address. --body uses a body file (the text
between the braces) instead of the current one. --apply writes the best body and padding back
into SOURCE only if the function then MATCHES.
"""
import argparse, itertools, os, re, shutil, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
DECL = re.compile(r'^    ((?:const |unsigned |struct |static |register )*[A-Za-z_]\w*(?: \*+| )\**)\s*(\w+)\s*(=\s*(.*))?;\s*(/\*.*\*/)?$')
KEYWORDS = {'return', 'if', 'else', 'while', 'for', 'do', 'switch', 'case', 'break', 'goto', 'delete', 'continue'}


def split_body(body):
    lines = body.strip('\n').split('\n')
    decls, stmts = [], []
    for ln in lines:
        m = DECL.match(ln)
        multi = re.match(r'^    (int|short|WORD|DWORD|BYTE|LONG|UINT|BOOL) (\w+(?:, *\w+)+);$', ln)
        if multi:
            decls += ['    %s %s;' % (multi.group(1), n.strip()) for n in multi.group(2).split(',')]
            continue
        if m and m.group(1).split()[0] not in KEYWORDS and '(' not in m.group(1):
            typ, name, init = m.group(1).rstrip(), m.group(2), m.group(4)
            sep = '' if typ.endswith('*') else ' '
            decls.append('    %s%s%s;' % (typ, sep, name))
            if init is not None:
                stmts.append('    %s = %s;' % (name, init))
            continue
        if ln.strip() == '' and not stmts:
            continue
        stmts.append(ln)
    return decls, stmts


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr')
    ap.add_argument('--pads', default='0-3'); ap.add_argument('--rounds', type=int, default=10)
    ap.add_argument('--jobs', type=int, default=6); ap.add_argument('--body'); ap.add_argument('--apply', action='store_true')
    a = ap.parse_args()
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    c0 = text.index('/* ' + a.addr)
    s = text.index('\n{\n', c0) + 3
    e = text.index('\n}\n', s)
    func = re.search(r'\n[^\n]*?(\w+(?:::\w+)?)\(', text[c0:s]).group(1)
    body = open(a.body, encoding='latin1').read() if a.body else text[s:e]
    decls, stmts = split_body(body)
    lo, _, hi = a.pads.partition('-')
    pads = range(int(lo), int(hi or lo) + 1)
    work = os.path.join(ROOT, 'build', 'scratch', 'splitclimb', func.replace(':', '_'))
    os.makedirs(work, exist_ok=True)
    base = os.path.basename(src)
    tag = re.sub(r'\W', '', func).lower()

    def build(order, pad, k):
        b = '\n'.join(order) + '\n\n' + '\n'.join(stmts).rstrip('\n') + '\n'
        padtext = ('extern int ' + ', '.join('pad_sc%s%d' % (tag, i) for i in range(pad)) + ';\n') if pad else ''
        t = text[:c0] + padtext + text[c0:s] + b + text[e + 1:]
        return t

    def score(args):
        order, pad, k = args
        d = os.path.join(work, str(k)); os.makedirs(d, exist_ok=True)
        p = os.path.join(d, base)
        open(p, 'w', encoding='latin1').write(build(order, pad, k))
        o = subprocess.run([os.path.join(ROOT, 'tools', 'msvc', 'try.sh'), a.module, p], capture_output=True, text=True).stdout
        m = re.search(r'^0x\w+ (?:\w+::)?%s: (MATCH|DIFF.*?(\d+) insns differ)' % re.escape(func.split('::')[-1]), o, re.M)
        ins = 999 if not m else (0 if m.group(1) == 'MATCH' else int(m.group(2)))
        mm = re.search(r'(\d+)/(\d+) functions match', o)
        unm = (int(mm.group(2)) - int(mm.group(1))) if mm else 999
        return (ins, unm), order, pad

    print('split: %d top declarations, %d statement lines' % (len(decls), len(stmts)), flush=True)
    cur = list(decls); best = None
    for rnd in range(a.rounds):
        cands = [cur]
        for i, j in itertools.combinations(range(len(cur)), 2):
            c = list(cur); c[i], c[j] = c[j], c[i]; cands.append(c)
        for i in range(1, len(cur)):
            c = [cur[i]] + cur[:i] + cur[i + 1:]
            if c not in cands: cands.append(c)
        jobs = [(c, p, n) for n, (c, p) in enumerate((c, p) for c in cands for p in pads)]
        with ThreadPoolExecutor(a.jobs) as ex:
            res = sorted(ex.map(score, jobs), key=lambda r: r[0])
        sc, order, pad = res[0]
        if rnd == 0:
            base_sc = [r for r in res if r[1] == decls and r[2] == pads[0]][0][0]
            print('baseline (split body, pad %d): %s' % (pads[0], base_sc), flush=True)
        print('round %d best %s pad %d' % (rnd, sc, pad), flush=True)
        if best is not None and sc >= best[0]:
            break
        best = (sc, order, pad); cur = order
        if sc[0] == 0:
            break
    sc, order, pad = best
    print('final %s pad %d' % (sc, pad))
    print('\n'.join(order))
    out = os.path.join(work, 'best_' + base)
    open(out, 'w', encoding='latin1').write(build(order, pad, 0))
    print('best file:', out)
    if a.apply and sc[0] == 0:
        open(src, 'w', encoding='latin1').write(build(order, pad, 0))
        print('applied to', src)


if __name__ == '__main__':
    main()
