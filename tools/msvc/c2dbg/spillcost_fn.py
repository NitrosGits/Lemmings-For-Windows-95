#!/usr/bin/env python3
"""Simplify order of C2.EXE (sub_417690) for ONE function, with the spill metrics (Claude s42).
usage: spillcost_fn.py SRC FUNC_INDEX [N=3000]   (FUNC_INDEX: see colour_trace_fn.py)

Node layout (VC++ 2.2 C2.EXE): +0x4 worklist link, +0x8 neighbour list, +0xc metric for h=0 (cost*100/degree),
+0x10 metric for h=1 (cost*100), +0x14 current degree (word), +0x16 spill cost (word), +0x18 id (word).
Selection (0x4176e6..0x417733): with K in edi, the node with the highest degree < K is removed; if every remaining
node has degree >= K, the one with the lowest metric[h] (ties: the first in the worklist). Breaks at 0x417737 (esi =
the removed node) and prints every removal of every simplify call as
    id  degree-at-removal  cost  metric[h]  '*' when it was a spill-candidate pick (degree >= K).
Calls of the simplify routine are numbered; the driver (see rounds_fn.py) runs h=0, h=1, then the better again,
then spills and rebuilds."""
import os
import re
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
src = os.path.abspath(sys.argv[1]); fi = int(sys.argv[2]); n = int(sys.argv[3]) if len(sys.argv) > 3 else 3000
subprocess.run([f'{here}/run_c1.sh', src, 'ct'], check=True)
cmds = ["break *0x401000", "cont", "delete 1", "break *0x417260"] + ["cont"] * fi
cmds += ["break *0x417690", "break *0x417737"]
for _ in range(n):
    cmds += ["cont", "info reg", "x/8x $esi"]
cmds.append("quit")
open('sc_dbg.txt', 'w').write("\n".join(cmds) + "\n")
out = subprocess.run([f'{here}/run_c2dbg.sh', 'sc_dbg.txt', 'ct', src, 'sc_out.obj'],
                     capture_output=True, text=True).stdout
open('sc_raw.txt', 'w').write(out)
blocks = re.split(r'Stopped on breakpoint (\d) at 0x[0-9a-f]+ c2\+0x[0-9a-f]+\n', out)[1:]
LINE = re.compile(r'0x0*[0-9a-f]+(?: c2\+0x[0-9a-f]+)?:\s+([0-9a-f ]+)\n')
call = drv = 0
for i in range(0, len(blocks), 2):
    bp = int(blocks[i]); b = blocks[i + 1]
    if bp == 2:
        drv += 1
        if drv > fi:
            print('== next function (driver call)'); break
        continue
    if drv < fi:
        continue
    if bp == 3:
        call += 1; print('-- simplify call %d' % call); continue
    regs = {m.group(1): int(m.group(2), 16) for m in re.finditer(r'\b(E[A-Z]{2}):([0-9a-f]{8})', b)}
    L = sum([[int(x, 16) for x in m.group(1).split()] for m in LINE.finditer(b)], [])
    if len(L) < 7 or 'ECX' not in regs:
        continue
    h = regs['ECX'] & 1; k = regs['EDI'] & 0xffff
    deg = L[5] & 0xffff; cost = L[5] >> 16; nid = L[6] & 0xffff
    deg = deg - 0x10000 if deg & 0x8000 else deg
    print('%04x deg %2d cost %5d metric %8d%s' % (nid, deg, cost, L[3 + h], ' *' if deg >= k else ''))
