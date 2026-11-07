#!/usr/bin/env python3
"""Annotated disassembly of an original module.

  dis.py MODULE [START [END]]     MODULE is e.g. VSMEM.DLL; addresses in hex
  dis.py MODULE -f ADDR           one function (bounds from analysis/MODULE/functions.tsv)

Import slots become __imp_Name, addresses with a name in names/MODULE.txt (or an
export) are shown by name, and strings in data are quoted after the instruction.
"""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from pe import PE  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJDUMP = os.path.expanduser('~/llvm-mingw-20260826-ucrt-macos-universal/bin/llvm-objdump')


def load_names(mod, pe):
    names = {}
    for o, a, n in pe.exports:
        if n:
            names[a] = n
    p = os.path.join(ROOT, 'tools', 'names', mod + '.txt')
    if os.path.exists(p):
        for line in open(p):
            line = line.split('#', 1)[0].strip()
            if not line:
                continue
            a, n = line.split()[:2]
            names[int(a, 16)] = n
    return names


def functions(mod):
    rows = []
    p = os.path.join(ROOT, 'analysis', mod, 'functions.tsv')
    for line in open(p).readlines()[1:]:
        f = line.split('\t')
        rows.append((int(f[0], 16), int(f[1], 16), f[3]))
    return rows


def strval(pe, a):
    s = pe.sec_of(a)
    if not s or s['chars'] & 0x20000000:
        return None
    b = pe.cstr(a, 300)
    if len(b) >= 2 and all(32 <= c < 127 or c in (9, 10, 13) for c in b):
        return b.decode('latin1')
    return None


def main():
    mod = sys.argv[1]
    path = os.path.join(ROOT, 'orig', 'WINLEMM', mod)
    pe = PE(path)
    names = load_names(mod, pe)
    funcs = functions(mod)
    if len(sys.argv) > 2 and sys.argv[2] == '-f':
        a = int(sys.argv[3], 16)
        f = next(f for f in funcs if f[0] <= a <= f[1])
        start, end = f[0], f[1] + 1
    else:
        text = pe.sec('.text')
        start = int(sys.argv[2], 16) if len(sys.argv) > 2 else text['va']
        end = int(sys.argv[3], 16) if len(sys.argv) > 3 else text['va'] + text['vsize']
    for fa, fe, fn in funcs:
        names.setdefault(fa, fn)
    out = subprocess.run([OBJDUMP, '-d', '--no-show-raw-insn', '-M', 'intel', '--no-leading-addr' if False else '-d',
                          f'--start-address={start:#x}', f'--stop-address={end:#x}', path],
                         capture_output=True, text=True).stdout
    fstarts = {f[0] for f in funcs}

    def sub(m):
        v = int(m.group(0), 16)
        if v in pe.imports:
            return '__imp_' + pe.imports[v][1]
        if v in names:
            return names[v]
        return m.group(0)

    for line in out.splitlines():
        m = re.match(r'\s*([0-9a-f]+):\s*(.*)', line)
        if not m:
            continue
        a = int(m.group(1), 16)
        ins = m.group(2)
        ins = re.sub(r'\s+<[^>]*>', '', ins)
        if a in fstarts or a in names:
            print(f'\n{names.get(a, "")}:  ; {a:#x}')
        notes = []
        for h in re.findall(r'0x[0-9a-f]{5,8}', ins):
            v = int(h, 16)
            sv = strval(pe, v)
            if sv is not None and v not in pe.imports:
                notes.append(repr(sv)[:80])
        ins = re.sub(r'0x[0-9a-f]{5,8}', sub, ins)
        ins = re.sub(r'\s+', ' ', ins.replace('\t', ' ')).strip()
        if re.fullmatch(r'lea (esp, \[esp\]|ecx, \[ecx\]|ebx, \[ebx\])|mov edi, edi|nop|add eax, 0x0', ins):
            continue
        print(f'  {a:08x}  {ins}' + (f'   ; {" ".join(notes)}' if notes else ''))


if __name__ == '__main__':
    main()
