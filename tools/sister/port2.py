#!/usr/bin/env python3
"""port2.py PROJECT MODULE: C++ modules (process, lemmings): include each of our files in the units that hold its ported functions."""
import csv, json, os, re, shutil, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import port as P

OURS, TC = P.OURS, P.TC


def map_globals(mapfile):
    """Non-function publics of a link map: {undecorated name: VA}."""
    out = {}
    for l in open(mapfile, errors='ignore'):
        m = re.match(r'\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(f\s+)?(\S+)', l)
        if not m or m.group(3): continue
        nm = m.group(1)
        if nm.startswith('?') and '@@3' in nm: w = nm[1:].split('@@')[0].split('@')[0]
        elif nm.startswith('_') and '@' not in nm: w = nm[1:]
        else: continue
        out[w] = int(m.group(2), 16)
    return out


def alias_defs(text, aliases):
    out = []
    pat = re.compile(r'^[^\n;{}#/]*?(?<![\w:])(%s)\s*\([^;{}]*\)\s*(?::[^{;]*)?(?:const\s*)?\{' % '|'.join(re.escape(a) for a in sorted(aliases, key=len, reverse=True)), re.M)
    pos = 0
    for m in pat.finditer(text):
        if m.start() < pos: continue
        depth, i = 0, m.end() - 1
        while i < len(text):
            if text[i] == '{': depth += 1
            elif text[i] == '}':
                depth -= 1
                if depth == 0: break
            i += 1
        out.append((m.group(1), m.start(), i + 1))
        pos = i + 1
    return out


def main():
    proj, mod = sys.argv[1], sys.argv[2]
    os.chdir(proj)
    prog = {'process': 'PROCESS.DLL', 'lemmings': 'lemmings.exe'}[mod]
    j = json.load(open('config/lemmings/functions.json'))
    rows = [r for r in csv.DictReader(open(P.CSV)) if r['program'] == prog and r['status_vs_original_VC2.2'] == 'MATCH']
    for h in os.listdir(OURS + '/src/include'):
        shutil.copy(f'{OURS}/src/include/{h}', 'include/' + h)
    objs = [l.strip() for l in open(f'{OURS}/src/{mod}/objs.txt') if l.strip() and not l.startswith('#') and l.strip().endswith('.cpp')]
    # our address -> file, from the /* 0xADDR */ annotations
    addr_file = {}
    for f in objs:
        for a in re.findall(r'/\*\s*0x([0-9a-fA-F]+)', open(f'{OURS}/src/{mod}/{f}').read()):
            addr_file[int(a, 16)] = f
    addr2unit = {f"{f['addr']:08X}": u for u, fs in j['units'].items() for f in fs}
    names, unit_files, aliases = {}, {}, {}
    for r in rows:
        a = re.search(r'func_([0-9A-Fa-f]{8})', r['their_name_in_decomp_wtf']).group(1).upper()
        oa = int(r['our_address'], 16)
        f = addr_file.get(oa)
        u = addr2unit.get(a)
        if not f or not u: continue
        names[a] = r['our_function_name']
        unit_files.setdefault(u, {}).setdefault(f, []).append(a)
    for k, v in names.items():
        old = j['names'].get(f'func_{k}')
        aliases[k] = {f'func_{k}'} | ({old} if old else set())
        j['names'][f'func_{k}'] = v
    # data names from our link map
    mp = {'process': 'build/msvc/process/PROCESS.map', 'lemmings': 'build/msvc/lemmings/LEMMINGS.map'}[mod]
    g = map_globals(f'{OURS}/{mp}')
    known = set(j['names'].values())
    nd = 0
    for w, va in g.items():
        key = f'D_{va:08X}'
        if key not in j['names'] and w not in known:
            j['names'][key] = w; nd += 1
    json.dump(j, open('config/lemmings/functions.json', 'w'), indent=1)
    print(mod, 'ported', len(names), 'units', len(unit_files), 'data names', nd)
    for u, files in sorted(unit_files.items()):
        p = f'src/auto/{u}.cpp'
        theirs = open(p).read()
        al = set()
        for fl in files.values():
            for a in fl: al |= aliases[a]
        drop = alias_defs(theirs, al)
        for a, s, e in sorted(drop, key=lambda x: -x[1]):
            sig = theirs[s:theirs.index('{', s)].strip()
            sig = sig.split(':')[0].strip() if re.search(r'\)\s*:', sig) else sig
            theirs = theirs[:s] + ('' if '::' in sig else sig + ';') + '\n' + theirs[e:]
        def strip_api(m):
            nm = re.search(r'(\w+)\s*\(', m.group(0))
            return '' if nm and nm.group(1) in P.HDR and 'func_' not in m.group(0) else m.group(0)
        theirs = re.sub(r'^extern "C"[^\n{]*;[ \t]*\n', strip_api, theirs, flags=re.M)
        theirs = re.sub(r'^(?:extern\s+)?[A-Za-z_][\w \*]*?\b(\w+)\s*\([^;{}()]*\)\s*;[ \t]*\n',
                        lambda m: '' if m.group(1) in P.HDR and 'func_' not in m.group(0) else m.group(0), theirs, flags=re.M)
        theirs = theirs.replace('#include "common.h"\n', '#include "common.h"\n#include <windows.h>\n' + P.COMPAT, 1)
        body = ''
        for f in objs:
            if f in files:
                body += f'\n// ---- ported: {f} ----\n' + open(f'{OURS}/src/{mod}/{f}').read() + '\n'
        theirs += '\n// ---- our matched ' + mod + ' code ----\n' + body
        open(p, 'w').write(theirs)
        print(' wrote', p, 'files', list(files), 'dropped', len(drop))


if __name__ == '__main__':
    main()
