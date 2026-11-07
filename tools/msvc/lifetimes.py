#!/usr/bin/env python3
"""Search intermediate-variable lifetimes for one function (the RestoreChanged trick):

  lifetimes.py MODULE.DLL|LEMMINGS.EXE SOURCE ADDR [--pads 0,1,2,3,7] [--jobs 8] [--out DIR]

Variants, each also with padding declarations before the function:
  copy   `v = e;` / `T v = e;` becomes `vTmp = e;` and `v = vTmp;` is placed 0-3 statements later
         (only past statements that do not mention v)
  swap   two adjacent simple statements that share no identifiers trade places
  hoist  a call nested in a statement (`f(g(), ...)`) is computed into a temporary first
  move   a declaration with an initializer moves to every other slot of the declaration block
Every candidate is compiled with tools/msvc/try.sh into OUT; results are ranked by the
function's differing instructions, then by the whole file's match count.
"""
import argparse, concurrent.futures, json, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TYPES = {'GetDesktopWindow': 'HWND', 'GetParent': 'HWND', 'GetDC': 'HDC', 'GetString': 'char *',
         'GetDibWidth': 'int', 'GetDibHeight': 'int', 'GetDibBitsAddr': 'BYTE *',
         'CreateCompatibleDC': 'HDC', 'GetStockObject': 'HGDIOBJ', 'GetSysColor': 'DWORD', 'GetLastError': 'DWORD',
         'strlen': 'int', 'GetDlgItem': 'HWND', 'FindRating': 'DBENTRY *', 'GetStatusHeight': 'int',
         'SendMessage': 'LRESULT', 'GetWindowLong': 'LONG', 'GetClientRect': 'int', 'timeGetTime': 'DWORD'}
DECL = re.compile(r'^(\s+)((?:const |unsigned |volatile |signed |struct )*[A-Za-z_]\w*\s*\**)\s*(\w+)(\s*=\s*(.+))?;\s*(/\*.*\*/)?$')
ASSIGN = re.compile(r'^(\s+)(\w+) = ([^;]+);\s*(/\*.*\*/)?$')
WORD = re.compile(r'\b[A-Za-z_]\w*\b')


def split_function(text, addr):
    a = text.index('/* ' + addr)
    open_ = text.index('\n{\n', a) + 3
    close = text.index('\n}\n', open_)
    sig = text[text.index('*/', a) + 2:open_]
    return a, sig, open_, close


def types_of(sig, lines):
    t = {}
    for m in re.finditer(r'((?:const |unsigned |volatile )*[A-Za-z_]\w*\s*\**)\s*(\w+)\s*[,)]', sig):
        t[m.group(2)] = m.group(1).strip()
    for l in lines:
        m = DECL.match(l)
        if m:
            t[m.group(3)] = m.group(2).replace('volatile ', '').strip()
    return t


def variants(body):
    lines = body.split('\n')
    nd = 0
    while nd < len(lines) and (DECL.match(lines[nd]) or not lines[nd].strip()):
        nd += 1
    decl_end = max(i for i in range(nd) if lines[i].strip()) + 1 if any(l.strip() for l in lines[:nd]) else 0
    ty = types_of('', lines[:decl_end])
    out = {}

    def emit(name, ls):
        out.setdefault('\n'.join(ls), name)

    def mentions(line, v):
        return re.search(r'\b' + v + r'\b', line)

    # copy
    for i, l in enumerate(lines):
        m = DECL.match(l) if i < decl_end else None
        if m and m.group(5):
            ind, t, v, e = m.group(1), m.group(2).strip(), m.group(3), m.group(5)
            if 'volatile' in t:
                continue
            tmp = v + 'Init'
            base = lines[:i] + [f'{ind}{t} {tmp} = {e};'] + lines[i + 1:]
            for j in range(i, decl_end):   # declare v after declaration j
                ls = base[:j + 1] + [f'{ind}{t} {v} = {tmp};'] + base[j + 1:]
                emit(f'copy_{v}_after{j - i}', ls)
            continue
        m = ASSIGN.match(l)
        if m and i >= decl_end and m.group(2) in ty and 'volatile' not in ty[m.group(2)]:
            ind, v, e = m.group(1), m.group(2), m.group(3)
            tmp = v + 'Tmp'
            k = i
            for steps in range(4):
                if steps:
                    k += 1
                    if k >= len(lines) or not ASSIGN.match(lines[k]) and not re.match(r'^\s+\w.*;\s*$', lines[k]) \
                            or mentions(lines[k], v) or lines[k][:len(ind)] != ind or lines[k][len(ind)] == ' ':
                        break
                ls = lines[:decl_end] + [f'    {ty[v]} {tmp};'] + lines[decl_end:i] + [f'{ind}{tmp} = {e};'] + \
                    lines[i + 1:k + 1] + [f'{ind}{v} = {tmp};'] + lines[k + 1:]
                emit(f'copy_{v}_L{i}_{steps}', ls)
    # swap
    simple = lambda l: re.match(r'^\s+[^{}]*;\s*(/\*.*\*/)?$', l) and not re.match(r'^\s+(return|if|for|while|do)\b', l)
    for i in range(decl_end, len(lines) - 1):
        a, b = lines[i], lines[i + 1]
        if simple(a) and simple(b) and len(a) - len(a.lstrip()) == len(b) - len(b.lstrip()):
            wa = set(WORD.findall(a.split('/*')[0])); wb = set(WORD.findall(b.split('/*')[0]))
            calls = re.search(r'\w\(', a) and re.search(r'\w\(', b)
            if not (wa & wb) and not calls:
                emit(f'swap_L{i}', lines[:i] + [b, a] + lines[i + 2:])
    # hoist
    for i in range(decl_end, len(lines)):
        for m in re.finditer(r'[(,]\s*((\w+)\(([^()]*)\))', lines[i]):
            f = m.group(2)
            if f not in TYPES:
                continue
            ind = re.match(r'\s*', lines[i]).group(0)
            tmp = f[0].lower() + f[1:] + 'Val'
            for at in sorted({decl_end, i}):
                ls = list(lines)
                ls[i] = ls[i].replace(m.group(1), tmp, 1)
                ls = ls[:decl_end] + [f'    {TYPES[f]} {tmp};'] + ls[decl_end:]
                pos = at + 1
                ls.insert(pos, f'{ind if at == i else "    "}{tmp} = {m.group(1)};')
                emit(f'hoist_{f}_L{i}_{"early" if at == decl_end else "late"}', ls)
            # as an initialized declaration
            ls = list(lines)
            ls[i] = ls[i].replace(m.group(1), tmp, 1)
            for j in range(decl_end + 1):
                emit(f'hoistdecl_{f}_L{i}_at{j}', ls[:j] + [f'    {TYPES[f]} {tmp} = {m.group(1)};'] + ls[j:])
    # move
    for i in range(decl_end):
        m = DECL.match(lines[i])
        if m and m.group(5):
            rest = lines[:i] + lines[i + 1:]
            for j in range(decl_end):
                if j != i:
                    emit(f'move_{m.group(3)}_to{j}', rest[:j] + [lines[i]] + rest[j:])
    return {n: b for b, n in out.items()}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('module'); ap.add_argument('source'); ap.add_argument('addr')
    ap.add_argument('--pads', default='0,1,2,3,7'); ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--out'); ap.add_argument('--list', action='store_true')
    a = ap.parse_args()
    src = os.path.abspath(a.source)
    text = open(src, encoding='latin1').read()
    ann, sig, s, e = split_function(text, a.addr)
    body = text[s:e]
    vs = {'baseline': body}
    vs.update(variants(body))
    if a.list:
        for n in vs:
            print(n)
        return
    out = a.out or os.path.join(ROOT, 'build/scratch/lifetimes', a.addr)
    os.makedirs(out, exist_ok=True)
    pads = [int(p) for p in a.pads.split(',')]
    cases = [(n, b, p) for n, b in vs.items() for p in pads]
    ext = os.path.splitext(src)[1]
    total = re.compile(r'(\d+)/(\d+) functions match')

    def run(case):
        n, b, p = case
        ident = re.sub(r'\W', '_', n) + f'_p{p:03d}'
        path = os.path.join(out, ident + ext)
        t = text[:s] + b + text[e:]
        if p:
            t = t[:ann] + 'extern int ' + ', '.join(f'pad_lt_{k}' for k in range(p)) + ';\n' + t[ann:]
        open(path, 'w', encoding='latin1').write(t)
        r = subprocess.run(['zsh', os.path.join(ROOT, 'tools/msvc/try.sh'), a.module, path],
                           cwd=ROOT, capture_output=True, text=True, timeout=300)
        o = r.stdout + r.stderr
        open(path[:-len(ext)] + '.log', 'w').write(o)
        line = next((l for l in o.splitlines() if l.startswith(a.addr + ' ')), '')
        m = re.search(r'(\d+) insns differ', line)
        ins = 0 if ': MATCH' in line else int(m.group(1)) if m else 9999
        tm = total.search(o)
        return {'name': n, 'pad': p, 'insns': ins, 'file': int(tm.group(1)) if tm else -1, 'line': line[:160]}

    print(f'{a.addr}: {len(vs)} bodies x {len(pads)} pads = {len(cases)} builds', flush=True)
    res = []
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as pool:
        for k, r in enumerate(pool.map(run, cases), 1):
            res.append(r)
            if r['insns'] == 0:
                print('MATCH', json.dumps(r), flush=True)
            if k % 40 == 0:
                print(f'{k}/{len(cases)} best {min(x["insns"] for x in res)}', flush=True)
    res.sort(key=lambda r: (r['insns'], -r['file'], r['pad']))
    json.dump(res, open(os.path.join(out, 'results.json'), 'w'), indent=1)
    base = [r for r in res if r['name'] == 'baseline' and r['pad'] == 0]
    print('baseline', json.dumps(base[0]) if base else '?')
    for r in res[:12]:
        print(f"{r['insns']:5} file {r['file']:3}  pad {r['pad']:2}  {r['name']}")


if __name__ == '__main__':
    main()
