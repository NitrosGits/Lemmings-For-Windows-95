#!/usr/bin/env python3
"""Cache-a-global search (the OnStartLevel `rate` trick): for every read-only global used twice or more
in one function, try `T tmp = g;` (at the top, or just before the first use) replacing all uses.

  cacheglobals.py MODULE SOURCE ADDR [--pads 0,1,2] [--subsets 2]

Types come from the declaration in src/include; short/WORD/int/long alternatives are tried.
Writes a spec for shapes.py and runs it.
"""
import argparse, itertools, os, re, subprocess, sys
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr')
    ap.add_argument('--pads', default='0,1,2'); ap.add_argument('--subsets', type=int, default=2)
    ap.add_argument('--jobs', type=int, default=8); ap.add_argument('--out')
    a = ap.parse_args()
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    ann = text.index('/* ' + a.addr); s = text.index('\n{\n', ann) + 3; e = text.index('\n}\n', s)
    body = text[s:e]
    names = {}
    for n in re.findall(r'\bg_\w+', body):
        names[n] = names.get(n, 0) + 1
    cands = []
    for n, c in names.items():
        if c < 2: continue
        if re.search(r'\b%s\b\s*(?:[-+*/&|^]?=(?!=)|\+\+|--)' % n, body) or re.search(r'(?:\+\+|--)\s*%s\b' % n, body) or re.search(r'&\s*%s\b' % n, body):
            continue
        cands.append(n)
    print('read-only globals used >=2x:', cands, flush=True)
    hdr = ''
    for f in os.listdir(os.path.join(ROOT, 'src/include')):
        if f.endswith('.h'): hdr += open(os.path.join(ROOT, 'src/include', f), encoding='latin1').read()
    def gtype(n):
        m = re.search(r'extern\s+([\w ]+?)\s+[^;]*\b%s\b[^;]*;' % n, hdr)
        return m.group(1).strip() if m else 'int'
    lines = body.split('\n')
    nd = 0
    while nd < len(lines) and (re.match(r'^\s+[A-Za-z_][\w \*]*\s+\**\w+(\[[^\]]*\])?(\s*=[^;]*)?;\s*$', lines[nd]) or not lines[nd].strip()):
        nd += 1
    alts = []
    for k in range(1, a.subsets + 1):
        for combo in itertools.combinations(cands, k):
            for tyc in itertools.product(['same', 'int', 'long'], repeat=k):
                for where in ('top', 'first'):
                    b = body
                    decls = []; inits = []
                    for n, tc in zip(combo, tyc):
                        ty = gtype(n) if tc == 'same' else tc
                        tmp = n[2:] + 'C'
                        b = re.sub(r'\b%s\b' % n, tmp, b)
                        decls.append('    %s %s;\n' % (ty, tmp)); inits.append((tmp, n))
                    bl = b.split('\n')
                    if where == 'top':
                        ins = ''.join('    %s = %s;\n' % (t, n) for t, n in inits)
                        b = '\n'.join(bl[:nd]) + '\n' + ''.join(decls) + ('\n' if True else '') + ins + '\n'.join(bl[nd:])
                    else:
                        out = []
                        pending = dict(inits)
                        for l in bl[nd:]:
                            for t in list(pending):
                                if re.search(r'\b%s\b' % t, l):
                                    out.append('    %s = %s;' % (t, pending.pop(t)))
                            out.append(l)
                        b = '\n'.join(bl[:nd]) + '\n' + ''.join(decls) + '\n'.join(out)
                    alts.append(b)
    alts = sorted(set(alts) - {body})
    od = a.out or os.path.join(ROOT, 'build/scratch/cacheglobals', a.addr)
    os.makedirs(od, exist_ok=True)
    spec = os.path.join(od, 'spec.py')
    open(spec, 'w').write('SLOTS=[(%r,%r)]\n' % (body, alts))
    print(len(alts), 'variants', flush=True)
    subprocess.run([sys.executable, os.path.join(ROOT, 'tools/msvc/shapes.py'), a.module, a.source, a.addr, spec,
                    '--pads', a.pads, '--jobs', str(a.jobs), '--max', str(len(alts) + 5), '--out', od])

if __name__ == '__main__':
    main()
