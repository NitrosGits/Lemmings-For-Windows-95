#!/usr/bin/env python3
"""Port our byte-matched C source for a DLL into the decompdecomp project (scratch or live).

  port.py PROJECT MODULE [--apply]

Per their unit that holds ported functions: drop their definitions of those addresses, put our source in.
Adds our function names (func_ADDR -> name) to functions.json "names", builds the unit, infers data names from relocs.
"""
import csv, json, os, re, shutil, subprocess, sys

OURS = '/Users/daniel/Lemmings-For-Windows-95'
CSV = '/Users/daniel/Library/Application Support/Claude/side-session-notes/local_9bc6502a-8c50-4dba-b2bd-fe72f6e4f399/function_map_ours_to_theirs.csv'
TC = '/Users/daniel/Library/Application Support/decompnet/toolchains'
PROG = {'readdib': 'READDIB.DLL', 'vsmem': 'VSMEM.DLL', 'filehand': 'FILEHAND.DLL', 'gameover': 'GAMEOVER.DLL',
        'loadlvl': 'LOADLVL.DLL', 'render': 'RENDER.DLL', 'process': 'PROCESS.DLL'}
sys.path.insert(0, OURS + '/tools')
from coff import Obj  # noqa


def defs(text):
    """Top-level function definitions named func_ADDR: (addr, start, end)."""
    out = []
    pat = re.compile(r'^[^\n;{}#/]*?\bfunc_([0-9A-Fa-f]{8})\s*\([^;{}]*\)\s*(?:const\s*)?\{', re.M)
    pos = 0
    for m in pat.finditer(text):
        if m.start() < pos:
            continue
        depth, i = 0, m.end() - 1
        while i < len(text):
            if text[i] == '{': depth += 1
            elif text[i] == '}':
                depth -= 1
                if depth == 0: break
            i += 1
        out.append((m.group(1).upper(), m.start(), i + 1))
        pos = i + 1
    return out


HDR = set()
for f in os.listdir(TC + '/msvc/2.2/Include'):
    if f.lower().endswith('.h'):
        HDR |= set(re.findall(r'\b[A-Za-z_]\w*\b', open(TC + '/msvc/2.2/Include/' + f, errors='ignore').read()))

COMPAT = '''
// ---- compatibility for the agents' code below (their prototypes were looser than windows.h) ----
#define WriteFile(h, b, n, w, o) WriteFile(h, b, n, (LPDWORD)(w), o)
#define ReadFile(h, b, n, r, o) ReadFile(h, b, n, (LPDWORD)(r), o)
#define waveOutOpen(a, b, c, d, e, f) waveOutOpen((LPHWAVEOUT)(a), b, (const WAVEFORMATEX*)(c), (DWORD)(d), (DWORD)(e), (DWORD)(f))
'''


PATCH = {  # C -> C++ fixes in our source's copy
    'filehand': [('char *p = buf;', 'char *p = (char *)buf;')],
}


def main():
    proj, mod = sys.argv[1], sys.argv[2]
    os.chdir(proj)
    prog = PROG[mod]
    j = json.load(open('config/lemmings/functions.json'))
    rows = [r for r in csv.DictReader(open(CSV)) if r['program'] == prog and r['status_vs_original_VC2.2'] == 'MATCH']
    names = {}
    for r in rows:
        a = re.search(r'func_([0-9A-Fa-f]{8})', r['their_name_in_decomp_wtf']).group(1).upper()
        names[a] = r['our_function_name']
    # our sources
    srcs = [l.strip() for l in open(f'{OURS}/src/{mod}/objs.txt') if l.strip() and not l.startswith('#') and l.strip().endswith('.c')]
    ours = ''
    for s in srcs:
        ours += f'\n// ---- ported: {s} ----\n' + open(f'{OURS}/src/{mod}/{s}').read() + '\n'
    for a, b in PATCH.get(mod, []):
        ours = ours.replace(a, b)
    for h in os.listdir(OURS + '/src/include'):
        shutil.copy(f'{OURS}/src/include/{h}', 'include/' + h)
    # units that hold ported functions
    addr2unit = {f"{f['addr']:08X}": u for u, fs in j['units'].items() for f in fs}
    units = sorted({addr2unit[a] for a in names if a in addr2unit and addr2unit[a].startswith(mod + '/')})
    print(mod, 'ported', len(names), 'units', units)
    for k, v in names.items():
        j['names'][f'func_{k}'] = v
    json.dump(j, open('config/lemmings/functions.json', 'w'), indent=1)
    for u in units:
        p = f'src/auto/{u}.cpp'
        theirs = open(p).read()
        drop = [(a, s, e) for a, s, e in defs(theirs) if a in names]
        for a, s, e in sorted(drop, key=lambda x: -x[1]):
            sig = theirs[s:theirs.index('{', s)].strip()
            theirs = theirs[:s] + sig + ';\n' + theirs[e:]
        def strip_api(m):
            nm = re.search(r'(\w+)\s*\(', m.group(0))
            return '' if nm and nm.group(1) in HDR else m.group(0)
        theirs = re.sub(r'^extern "C"[^\n{]*;[ \t]*\n', strip_api, theirs, flags=re.M)
        theirs = re.sub(r'^(?:extern\s+)?[A-Za-z_][\w \*]*?\b(\w+)\s*\([^;{}()]*\)\s*;[ \t]*\n', lambda m: '' if m.group(1) in HDR and 'func_' not in m.group(0) else m.group(0), theirs, flags=re.M)
        theirs = theirs.replace('#include "common.h"\n', '#include "common.h"\n#include <windows.h>\n' + COMPAT, 1)
        at = len(theirs)
        theirs = theirs[:at] + '\n// ---- our matched ' + mod + ' code ----\n' + ours + '\n// ---- end ----\n' + theirs[at:]
        open(p, 'w').write(theirs)
        print(' wrote', p, 'dropped', len(drop))


if __name__ == '__main__':
    main()
