#!/usr/bin/env python3
"""How C2.EXE numbers the 0x4xxx register candidates of one function (reverse engineered by Antigravity s58).
usage: buckets_fn.py SRC HIT [N=80]
HIT = which stop at 0x4591ec (per-function reset of the candidate hash table 0x476430) is the target function
(OpenLevelItem in Antigravity's testbed = 5). Prints, in id order: id, kind word, expr[2] (per-function symbol/temp
index), hash bucket ((expr[2]-4)&31 for kind 0x00010104). The ids are given in ascending bucket order."""
import os,re,subprocess,sys
here=os.path.dirname(os.path.abspath(__file__))
src=os.path.abspath(sys.argv[1]); hit=int(sys.argv[2]); n=int(sys.argv[3]) if len(sys.argv)>3 else 80
subprocess.run([f'{here}/run_c1.sh',src,'ct'],check=True)
cmds=["break *0x401000","cont","delete 1","break *0x4591ec"]+["cont"]*hit+["delete 2","break *0x4591ec","break *0x459304"]
for _ in range(n): cmds+=["cont","info reg","x/4x $esi","x/4x ($esp+0x10)"]
cmds.append("quit")
open('bk_dbg.txt','w').write("\n".join(cmds)+"\n")
out=subprocess.run([f'{here}/run_c2dbg.sh','bk_dbg.txt','ct',src,'bk_out.obj'],capture_output=True,text=True).stdout
open('bk_raw.txt','w').write(out)
blocks=re.split(r'Stopped on breakpoint (\d) at 0x[0-9a-f]+ c2\+0x[0-9a-f]+\n',out)[1:]
for i in range(0,len(blocks),2):
    bp=int(blocks[i]); b=blocks[i+1]
    if bp==3: break                      # next function's reset
    if bp!=4: continue
    regs={m.group(1):int(m.group(2),16) for m in re.finditer(r'\b(E[A-Z]{2}):([0-9a-f]{8})',b)}
    mem=[[int(x,16) for x in m.group(2).split()] for m in re.finditer(r'0x0*([0-9a-f]+)(?: c2\+0x[0-9a-f]+)?:\s+([0-9a-f ]+)\n',b)]
    if len(mem)<2: continue
    e=mem[0]; bk=(mem[1][0]-0x476430)//4
    print('id %04x kind %08x expr2 %3x bucket %2d'%(regs['EAX']&0xffff,e[1],e[2],bk))
