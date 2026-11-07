#!/usr/bin/env python3
"""Register-tie search: vary the signedness/width of integer locals and the order of the
declarations of one function (the CreateFrontDib trick), then score every variant with shapes.py.

  typeshuffle.py MODULE SOURCE ADDR [--n 300] [--pads 0] [--seed 1] [--jobs 8] [--out DIR]

Uninitialised declarations are shuffled freely; initialised ones keep their relative order
(their initialisers run in that order). Integer locals (int/short/long/unsigned/DWORD/WORD/BYTE)
get a random alternative type from the same width class; pointers/handles stay as written.
Variants are written to a spec for tools/msvc/shapes.py and run there.
"""
import argparse, os, random, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DECL = re.compile(r'^(\s+)((?:unsigned |signed |const |struct )*[A-Za-z_]\w*(?:\s*\*+)?)\s*(\*?\s*\w+(?:\[[^\]]*\])?)\s*(=\s*[^;]+)?;\s*(/\*.*\*/)?$')
CLASSES = [
    ({'int', 'unsigned int', 'long', 'unsigned long', 'DWORD', 'UINT', 'unsigned'},
     ['int', 'unsigned int', 'long', 'DWORD', 'UINT', 'unsigned long']),
    ({'short', 'unsigned short', 'WORD'}, ['short', 'unsigned short', 'WORD']),
    ({'BYTE', 'unsigned char', 'char'}, ['BYTE', 'unsigned char', 'char']),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr')
    ap.add_argument('--n', type=int, default=300); ap.add_argument('--pads', default='0')
    ap.add_argument('--seed', type=int, default=1); ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--out'); ap.add_argument('--anywhere', action='store_true', help='also retype integer declarations inside nested blocks')
    ap.add_argument('--types', action='store_true', help='types only, keep order')
    ap.add_argument('--order', action='store_true', help='order only, keep types')
    a = ap.parse_args()
    random.seed(a.seed)
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    ann = text.index('/* ' + a.addr)
    s = text.index('\n{\n', ann) + 3
    e = text.index('\n}\n', s)
    body = text[s:e]
    lines = body.split('\n')
    n = 0
    decls = []
    while n < len(lines) and (DECL.match(lines[n]) or not lines[n].strip()):
        if lines[n].strip():
            decls.append(lines[n])
        n += 1
        if not lines[n - 1].strip():
            break
    first_stmt = len(decls) + (1 if n > len(decls) else 0)
    # count real header lines (decls + optional one blank)
    hdr_lines = n
    rest = '\n'.join(lines[hdr_lines:])
    parsed = []
    for d in decls:
        m = DECL.match(d)
        parsed.append((m.group(1), m.group(2).strip(), m.group(3).strip(), m.group(4), d))
    if not parsed:
        print('no declarations found'); return
    free = [i for i, p in enumerate(parsed) if not p[3]]
    inited = [i for i, p in enumerate(parsed) if p[3]]

    def retype(p):
        ind, ty, nm, init, raw = p
        if ty.endswith('*') or nm.startswith('*') or '[' in nm:
            return raw
        for cls, alts in CLASSES:
            if ty in cls:
                nt = random.choice(alts)
                return f'{ind}{nt} {nm}{" " + init if init else ""};'
        return raw

    variants = set()
    tries = 0
    while len(variants) < a.n and tries < a.n * 20:
        tries += 1
        items = list(parsed)
        if not a.types:
            # shuffle free declarations among all positions, inited keep relative order
            order = list(range(len(items)))
            pos = list(range(len(items)))
            random.shuffle(pos)
            slots = {}
            ini_seq = iter(inited)
            fr = free[:]; random.shuffle(fr)
            fr_it = iter(fr)
            new = [None] * len(items)
            # choose positions for inited decls keeping order
            ipos = sorted(random.sample(range(len(items)), len(inited)))
            for k, i in zip(ipos, inited):
                new[k] = i
            for k in range(len(items)):
                if new[k] is None:
                    new[k] = next(fr_it)
            items = [parsed[i] for i in new]
        out = []
        for p in items:
            out.append(p[4] if a.order else retype(p))
        rest2 = rest
        if a.anywhere:
            rl = []
            for ln in rest.split('\n'):
                m = DECL.match(ln)
                rl.append(retype((m.group(1), m.group(2).strip(), m.group(3).strip(), m.group(4), ln)) if m else ln)
            rest2 = '\n'.join(rl)
        v = '\n'.join(out) + '\n' + ('\n' if n > len(decls) else '') + rest2
        variants.add(v)
    spec_dir = a.out or os.path.join(ROOT, 'build/scratch/typeshuffle', a.addr)
    os.makedirs(spec_dir, exist_ok=True)
    spec = os.path.join(spec_dir, 'spec.py')
    orig = '\n'.join(lines)
    variants.discard(orig)
    open(spec, 'w').write('SLOTS=[(%r,%r)]\n' % (orig, sorted(variants)))
    print(f'{len(variants)} variants -> shapes.py', flush=True)
    subprocess.run([sys.executable, os.path.join(ROOT, 'tools/msvc/shapes.py'), a.module, a.source, a.addr, spec,
                    '--pads', a.pads, '--jobs', str(a.jobs), '--max', str(a.n + 10), '--out', spec_dir])


if __name__ == '__main__':
    main()
