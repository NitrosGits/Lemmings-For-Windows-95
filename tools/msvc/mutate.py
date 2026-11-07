#!/usr/bin/env python3
"""Semantic-preserving micro-mutations of one function: every site where a rewrite rule applies is a
candidate; tries all single rewrites, then random combinations of 2-3. Scored by shapes.py.
  mutate.py MODULE SOURCE ADDR [--n 500] [--pads 0] [--seed 1]
Rules: a - K <-> a + -K ; x += y <-> x = x + y ; a == b <-> b == a ; a < b <-> b > a ; !x <-> x == 0 ;
       if (x) <-> if (x != 0) ; a * 2 <-> a << 1 ; a + b <-> b + a (simple identifiers) ; i++ <-> ++i <-> i += 1 ;
       (x) parenthesised idents.
"""
import argparse, os, random, re, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ID = r'[A-Za-z_][\w\.]*(?:->\w+)*(?:\[[^\]\[]*\])?'
RULES = [
    (re.compile(r'(%s) - (\d+)\b' % ID), lambda m: '%s + -%s' % (m.group(1), m.group(2))),
    (re.compile(r'(%s) \+ -(\d+)\b' % ID), lambda m: '%s - %s' % (m.group(1), m.group(2))),
    (re.compile(r'(%s) (\+|-|\*|&|\|) = ' % ID), None),
    (re.compile(r'\b(%s) \+= ([^;,)]+)' % ID), lambda m: '%s = %s + %s' % (m.group(1), m.group(1), m.group(2))),
    (re.compile(r'\b(%s) -= ([^;,)]+)' % ID), lambda m: '%s = %s - %s' % (m.group(1), m.group(1), m.group(2))),
    (re.compile(r'\b(%s) == (%s)\b' % (ID, ID)), lambda m: '%s == %s' % (m.group(2), m.group(1))),
    (re.compile(r'\b(%s) != (%s)\b' % (ID, ID)), lambda m: '%s != %s' % (m.group(2), m.group(1))),
    (re.compile(r'\b(%s) < (%s)\b' % (ID, ID)), lambda m: '%s > %s' % (m.group(2), m.group(1))),
    (re.compile(r'\b(%s) > (%s)\b' % (ID, ID)), lambda m: '%s < %s' % (m.group(2), m.group(1))),
    (re.compile(r'\b(%s) <= (%s)\b' % (ID, ID)), lambda m: '%s >= %s' % (m.group(2), m.group(1))),
    (re.compile(r'\b(%s) >= (%s)\b' % (ID, ID)), lambda m: '%s <= %s' % (m.group(2), m.group(1))),
    (re.compile(r'if \(!(%s)\)' % ID), lambda m: 'if (%s == 0)' % m.group(1)),
    (re.compile(r'if \((%s)\)' % ID), lambda m: 'if (%s != 0)' % m.group(1)),
    (re.compile(r'(%s) \* 2\b' % ID), lambda m: '%s << 1' % m.group(1)),
    (re.compile(r'(%s) \* 2\b' % ID), lambda m: '2 * %s' % m.group(1)),
    (re.compile(r'(%s) / 2\b' % ID), lambda m: '%s >> 1' % m.group(1)),
    (re.compile(r'\b(%s) \+ (%s)\b' % (ID, ID)), lambda m: '%s + %s' % (m.group(2), m.group(1))),
    (re.compile(r'\b(%s) \* (%s)\b' % (ID, ID)), lambda m: '%s * %s' % (m.group(2), m.group(1))),
    (re.compile(r'\b(%s)\+\+' % ID), lambda m: '++%s' % m.group(1)),
    (re.compile(r'\+\+(%s)\b' % ID), lambda m: '%s++' % m.group(1)),
    (re.compile(r'\b(%s)--' % ID), lambda m: '--%s' % m.group(1)),
    (re.compile(r'\b(%s) = (%s) \? ([^:;]+) : ([^;]+);' % (ID, ID)), None),
]
RULES = [r for r in RULES if r[1] is not None]
def sites(body):
    out = []
    for ri, (rx, fn) in enumerate(RULES):
        for m in rx.finditer(body):
            out.append((m.start(), m.end(), fn(m)))
    return out
def apply(body, picks):
    for st, en, rep in sorted(picks, reverse=True):
        body = body[:st] + rep + body[en:]
    return body
def overlap(picks):
    iv = sorted((p[0], p[1]) for p in picks)
    return any(iv[i][1] > iv[i + 1][0] for i in range(len(iv) - 1))
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr')
    ap.add_argument('--n', type=int, default=500); ap.add_argument('--pads', default='0')
    ap.add_argument('--seed', type=int, default=1); ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--out')
    a = ap.parse_args(); random.seed(a.seed)
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    ann = text.index('/* ' + a.addr); s = text.index('\n{\n', ann) + 3; e = text.index('\n}\n', s)
    body = text[s:e]
    ss = sites(body)
    variants = set()
    for p in ss: variants.add(apply(body, [p]))
    tries = 0
    while len(variants) < a.n and tries < a.n * 30 and len(ss) >= 2:
        tries += 1
        k = random.choice([2, 2, 3])
        picks = random.sample(ss, min(k, len(ss)))
        if overlap(picks): continue
        variants.add(apply(body, picks))
    variants.discard(body)
    od = a.out or os.path.join(ROOT, 'build/scratch/mutate', a.addr); os.makedirs(od, exist_ok=True)
    spec = os.path.join(od, 'spec.py')
    open(spec, 'w').write('SLOTS=[(%r,%r)]\n' % (body, sorted(variants)))
    print(len(ss), 'sites;', len(variants), 'variants', flush=True)
    subprocess.run([sys.executable, os.path.join(ROOT, 'tools/msvc/shapes.py'), a.module, a.source, a.addr, spec,
                    '--pads', a.pads, '--jobs', str(a.jobs), '--max', str(len(variants) + 5), '--out', od])
if __name__ == '__main__': main()
