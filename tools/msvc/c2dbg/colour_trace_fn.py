#!/usr/bin/env python3
"""Colour trace (see colour_trace.py) for ONE function of a large file, without tracing the functions before it.
usage: colour_trace_fn.py SRC FUNC_INDEX [N_STOPS=800]
FUNC_INDEX = 1-based index of the call of the allocator driver sub_417260 (one per compiled function, in file order;
count them with a run that only breaks on 0x417260). Prints '== driver N' at each function boundary, then the colour rows
(id, mask of colours of already coloured neighbours, colour, degree, cost) in colouring order."""
import os,re,subprocess,sys
here=os.path.dirname(os.path.abspath(__file__))
src=os.path.abspath(sys.argv[1]); fi=int(sys.argv[2]); n=int(sys.argv[3]) if len(sys.argv)>3 else 800
subprocess.run([f'{here}/run_c1.sh',src,'ct'],check=True)
cmds=["break *0x401000","cont","delete 1","break *0x417260"]
cmds+=["cont"]*fi            # now stopped at driver call number fi
cmds+=["break *0x4175c4","break *0x41761d"]
for _ in range(n): cmds+=["cont","info reg","x/8x $esp","x/8x $ebx"]
cmds.append("quit")
open('ctf_dbg.txt','w').write("\n".join(cmds)+"\n")
out=subprocess.run([f'{here}/run_c2dbg.sh','ctf_dbg.txt','ct',src,'ctf_out.obj'],capture_output=True,text=True).stdout
blocks=re.split(r'Stopped on breakpoint (\d) at 0x[0-9a-f]+ c2\+0x[0-9a-f]+\n',out)[1:]
def hx(l): return [int(x,16) for x in re.match(r'0x[0-9a-f]+(?: c2\+0x[0-9a-f]+)?:\s+(.*)',l).group(1).split()]
names='eax ecx edx ebx esp ebp esi edi'.split(); pend=None; drv=0
for i in range(0,len(blocks),2):
    bp=int(blocks[i]); b=blocks[i+1]
    if bp==2:
        drv+=1
        if drv>=fi: print('== driver',drv)
        continue
    regs={m.group(1):int(m.group(2),16) for m in re.finditer(r'\b(E[A-Z]{2}):([0-9a-f]{8})',b)}
    mem=[hx(l) for l in b.split('\n') if re.match(r'0x[0-9a-f]+(?: c2\+0x[0-9a-f]+)?:\s+[0-9a-f]{8}',l)]
    if bp==3: pend=regs['EDI']
    elif bp==4 and len(mem)>=4:
        nd=sum(mem[2:4],[])
        print('id %04x neigh-mask %02x colour %s deg %d cost %d'%(nd[6]&0xffff,pend or 0,names[regs['ECX']] if regs['ECX']<8 else regs['ECX'],nd[5]&0xffff,nd[5]>>16))
