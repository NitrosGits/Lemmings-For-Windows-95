#!/usr/bin/env python3
"""Infer the original address of every .bss global from the code:

  bssaddr.py MODULE OBJDIR [--names]

For each annotated function of each source in the module's objs.txt, every
DIR32 relocation to a .bss symbol is looked up in the original at the same
offset (exact only where the function's bytes line up, so functions that
match vote first). Prints name, inferred original address and our address
(from the link map), marking differences; --names prints lines for
tools/names instead.
"""
import os
import re
import struct
import sys
from collections import Counter, defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from coff import Obj
from pe import PE

sys.path.insert(0, os.path.join(ROOT, 'tools', 'msvc'))
from bsshash import undecorate


def annotations(src):
    lines = open(src, encoding='latin1').read().split('\n')
    out = []
    for i, line in enumerate(lines):
        m = re.match(r'\s*/\*\s*(0x[0-9a-fA-F]+)\b.*\*/\s*$', line)
        if not m:
            continue
        head = ' '.join(lines[i + 1:i + 4])
        f = re.match(r'[^(;{=]*?(\w+(?:::~?\w+)?)\s*\(', head)
        if f:
            out.append((int(m.group(1), 16), f.group(1)))
    return out


ONLY = set(open(os.environ['MATCHED']).read().split()) if os.environ.get('MATCHED') else None


def main():
    mod, objdir = sys.argv[1], sys.argv[2]
    exe = {'LEMMINGS': 'LEMMINGS.EXE'}.get(mod, mod + '.DLL')
    pe = PE(os.path.join(ROOT, 'orig', 'WINLEMM', exe))
    srcdir = os.path.join(ROOT, 'src', mod.lower())
    votes = defaultdict(Counter)
    for line in open(os.path.join(srcdir, 'objs.txt')):
        src = line.strip()
        if not src.endswith(('.c', '.cpp')):
            continue
        objp = os.path.join(objdir, os.path.splitext(src)[0] + '.obj')
        if not os.path.exists(objp):
            continue
        o = Obj(objp)
        funcs = {}
        for s in o.symbols.values():
            if s.is_function and s.secnum > 0:
                funcs.setdefault(s.secnum, []).append(s)
        ann = annotations(os.path.join(srcdir, src))
        bynm = {}
        for a, n in ann:
            bynm.setdefault(n, []).append(a)
        for sec in o.sections:
            if not sec.name.startswith('.text'):
                continue
            for fs in [funcs.get(sec.idx, [])]:
                for f in sorted(fs, key=lambda s: s.value):
                    nm = undecorate(f.name)
                    nm = re.sub(r'^\?', '', f.name).split('@@')[0]
                    short = nm.split('@')[0]
                    cls = nm.split('@')[1] if '@' in nm else None
                    key = f'{cls}::{short}' if cls else short
                    addrs = bynm.get(key) or bynm.get(short)
                    if not addrs:
                        continue
                    addr = addrs.pop(0)
                    if ONLY and key not in ONLY and short not in ONLY:
                        continue
                    end = min([g.value for g in fs if g.value > f.value] + [len(sec.data)])
                    for off, si, typ in sec.relocs:
                        if not (f.value <= off < end) or typ != 6:
                            continue
                        t = o.symbols.get(si)
                        if t is None or t.secnum <= 0 or o.section(t.secnum).name != '.bss' and \
                                not (t.secnum == 0):
                            pass
                        if t is None:
                            continue
                        tsec = o.section(t.secnum).name if t.secnum > 0 else None
                        if tsec != '.bss' and not (t.secnum == 0 and t.name.startswith('?g_')):
                            continue
                        addend = struct.unpack_from('<I', sec.data, off)[0]
                        try:
                            ov = pe.u32(addr + off - f.value)
                        except Exception:
                            continue
                        votes[undecorate(t.name)][ov - addend] += 1
    ours = {}
    mp = os.path.join(objdir, mod + '.map')
    if os.path.exists(mp):
        for m in re.finditer(r'^ \d{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', open(mp).read(), re.M):
            ours[undecorate(m.group(1))] = int(m.group(2), 16)
    for n, c in sorted(votes.items(), key=lambda kv: kv[1].most_common(1)[0][0]):
        a, k = c.most_common(1)[0]
        if '--names' in sys.argv:
            print(f'{a:x} {n}')
        else:
            mine = ours.get(n)
            mark = '' if mine == a else f'   ours {mine:#x} ({a - mine:+#x})' if mine else ''
            print(f'{a:#x} {n:24} votes {k}/{sum(c.values())}{mark}')


if __name__ == '__main__':
    main()
