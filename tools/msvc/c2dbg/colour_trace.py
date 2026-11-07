#!/usr/bin/env python3
"""Trace VC++ 2.2's colour assignment (C2.EXE sub_417580) for a source file under winedbg.
usage: colour_trace.py SRC.cpp [N_STOPS]   (run from any scratch dir; writes il files + dbg script there)
Prints, per coloured node: virtual register id, mask of colours already used by coloured neighbours, chosen colour,
degree, spill cost. Colour order tried: eax ecx edx esi edi ebx ebp. The node ids restart per function; later rounds repeat
nodes (the driver retries), so read the LAST block for a function's final colours."""
import os,re,subprocess,sys
here=os.path.dirname(os.path.abspath(__file__))
src=os.path.abspath(sys.argv[1]); n=int(sys.argv[2]) if len(sys.argv)>2 else 600
subprocess.run([f'{here}/run_c1.sh',src,'ct'],check=True)
cmds=["break *0x401000","cont","delete 1","break *0x4175c4","break *0x41761d"]
for _ in range(n): cmds+=["cont","info reg","x/8x $esp","x/8x $ebx"]
cmds.append("quit")
open('ct_dbg.txt','w').write("\n".join(cmds)+"\n")
out=subprocess.run([f'{here}/run_c2dbg.sh','ct_dbg.txt','ct',src,'ct_out.obj'],capture_output=True,text=True).stdout
blocks=re.split(r'Stopped on breakpoint (\d) at 0x[0-9a-f]+ c2\+0x[0-9a-f]+\n',out)[1:]
def hx(l): return [int(x,16) for x in re.match(r'0x[0-9a-f]+(?: c2\+0x[0-9a-f]+)?:\s+(.*)',l).group(1).split()]
names='eax ecx edx ebx esp ebp esi edi'.split(); pend=None
for i in range(0,len(blocks),2):
    bp=int(blocks[i]); b=blocks[i+1]
    regs={m.group(1):int(m.group(2),16) for m in re.finditer(r'\b(E[A-Z]{2}):([0-9a-f]{8})',b)}
    mem=[hx(l) for l in b.split('\n') if re.match(r'0x[0-9a-f]+(?: c2\+0x[0-9a-f]+)?:\s+[0-9a-f]{8}',l)]
    if bp==2: pend=regs['EDI']
    elif len(mem)>=4:
        nd=sum(mem[2:4],[])
        print('id %04x neigh-mask %02x colour %s deg %d cost %d'%(nd[6]&0xffff,pend or 0,names[regs['ECX']] if regs['ECX']<8 else regs['ECX'],nd[5]&0xffff,nd[5]>>16))
