#!/usr/bin/env python3
"""Map C2.EXE's 0x4xxx register-candidate ids of ONE function to their symbols (Claude s42).
usage: cands_fn.py SRC HIT [N=1500]
HIT = which stop at 0x4591ec (per-function reset of the candidate hash table 0x476430) is the target function
(see buckets_fn.py; OpenLevelItem in the oli testbeds = 5).

Findings this rests on (C2.EXE, VC++ 2.2):
* the candidate hash (sub_44d060) is opcode + type word + symbol->[+0x20]; [+0x20] is a sequence number that the
  front end gives every symbol in declaration order across the whole file (globals, params, locals, block locals),
  so the bucket of a user variable is just (seq & 31). Names do not matter; extern declarations before a function and
  locals declared before a variable shift its seq. Compiler temps (CSE) get symbols too (frame offset 0, created later).
* ids 0x4020.. are given in ascending bucket order (walk at 0x45928f..0x4593a9), so the simplify worklist order
  (id & 31 descending, see simplify_model.py) follows the seq order of the symbols.
Prints per id: kind, bucket, symbol pointer, seq ([+0x20]), frame offset ([+4]), size ([+8]); '?' = symbol not seen
by the hash breakpoint (then its seq is unknown here)."""
import os
import re
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
src = os.path.abspath(sys.argv[1]); hit = int(sys.argv[2]); n = int(sys.argv[3]) if len(sys.argv) > 3 else 1500
subprocess.run([f'{here}/run_c1.sh', src, 'ct'], check=True)
LINE = re.compile(r'0x0*[0-9a-f]+(?: c2\+0x[0-9a-f]+)?:\s+([0-9a-f ]+)\n')
STOP = re.compile(r'Stopped on breakpoint (\d) at 0x[0-9a-f]+ c2\+0x[0-9a-f]+\n')


def trace(bp, dumps, tag):
    """Two separate runs (same IL files, so the same heap addresses): a failing x/ command makes winedbg drop
    the commands after it, and ecx is not a pointer at 0x459304."""
    cmds = ["break *0x401000", "cont", "delete 1", "break *0x4591ec"] + ["cont"] * hit
    cmds += ["delete 2", "break *0x4591ec", "break *0x%x" % bp]
    for _ in range(n):
        cmds += ["cont", "info reg"] + dumps
    cmds.append("quit")
    open(f'cd_{tag}_dbg.txt', 'w').write("\n".join(cmds) + "\n")
    out = subprocess.run([f'{here}/run_c2dbg.sh', f'cd_{tag}_dbg.txt', 'ct', src, 'cd_out.obj'],
                         capture_output=True, text=True).stdout
    open(f'cd_{tag}_raw.txt', 'w').write(out)
    blocks = STOP.split(out)[1:]
    res = []
    for i in range(0, len(blocks), 2):
        if int(blocks[i]) == 3:
            break
        b = blocks[i + 1]
        regs = {m.group(1): int(m.group(2), 16) for m in re.finditer(r'\b(E[A-Z]{2}):([0-9a-f]{8})', b)}
        res.append((regs, [[int(x, 16) for x in m.group(1).split()] for m in LINE.finditer(b)]))
    return res


syms = {}
for regs, L in trace(0x44d083, ["x/12x $ecx"], 'h'):
    if 'ECX' in regs and len(L) >= 3:
        syms.setdefault(regs['ECX'], L[0] + L[1] + L[2])
rows = []
for regs, L in trace(0x459304, ["x/8x $esi", "x/4x ($esp+0x10)"], 'w'):
    if 'EAX' in regs and len(L) >= 3:
        e = L[0] + L[1]
        rows.append((regs['EAX'] & 0xffff, e[1], (L[2][0] - 0x476430) // 4, e[5]))
for nid, kind, bk, sp in rows:
    s = syms.get(sp)
    if s:
        print('%04x kind %08x bucket %2d sym %08x seq %5x off %5d size %d' % (
            nid, kind, bk, sp, s[8], (s[1] ^ 0x80000000) - 0x80000000, s[2]))
    else:
        print('%04x kind %08x bucket %2d sym %08x ?' % (nid, kind, bk, sp))
