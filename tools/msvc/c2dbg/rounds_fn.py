#!/usr/bin/env python3
"""Round-by-round view of C2.EXE's allocator driver (sub_417260) for ONE function.
usage: rounds_fn.py SRC FUNC_INDEX [N_STOPS=1500]   (FUNC_INDEX: see colour_trace_fn.py)
Shows outer iterations (graph rebuilt after a spill), each simplify call (heuristic h), every colouring success
(id -> colour) and failure (id FAIL), and the spill call. Node pointers are mapped to ids with a neighbour-loop
breakpoint in the same run (a node with no neighbours shows as p<addr>)."""
import os,re,subprocess,sys
here=os.path.dirname(os.path.abspath(__file__))
src=os.path.abspath(sys.argv[1]); fi=int(sys.argv[2]); n=int(sys.argv[3]) if len(sys.argv)>3 else 1500
subprocess.run([f'{here}/run_c1.sh',src,'ct'],check=True)
B={3:0x4172fb,4:0x41735c,5:0x41741b,6:0x41761d,7:0x41766a,8:0x417430}
cmds=["break *0x401000","cont","delete 1","break *0x417260"]+["cont"]*fi
cmds+=["break *0x%x"%B[k] for k in sorted(B)]
for _ in range(n): cmds+=["cont","info reg","x/8x $esp"]
cmds.append("quit")
open('rd_dbg.txt','w').write("\n".join(cmds)+"\n")
out=subprocess.run([f'{here}/run_c2dbg.sh','rd_dbg.txt','ct',src,'rd_out.obj'],capture_output=True,text=True).stdout
open('rd_raw.txt','w').write(out)
blocks=re.split(r'Stopped on breakpoint (\d) at 0x[0-9a-f]+ c2\+0x[0-9a-f]+\n',out)[1:]
def dumps(b):
    r=[]
    for l in b.split('\n'):
        m=re.match(r'0x0*([0-9a-f]+)(?: c2\+0x[0-9a-f]+)?:\s+([0-9a-f ]+)$',l.strip())
        if m: r.append((int(m.group(1),16),[int(x,16) for x in m.group(2).split()]))
    return r
names='eax ecx edx ebx esp ebp esi edi'.split()
ev=[]; ptrid={}; drv=0
for i in range(0,len(blocks),2):
    bp=int(blocks[i]); b=blocks[i+1]
    if bp==2:
        drv+=1; ev.append(('drv',drv)); continue
    if drv!=fi: continue
    regs={m.group(1):int(m.group(2),16) for m in re.finditer(r'\b(E[A-Z]{2}):([0-9a-f]{8})',b)}
    d=dumps(b); st=[x for a,x in d if a==regs.get('ESP')]
    st=sum([x for a,x in d if regs.get('ESP',0)<=a<regs.get('ESP',0)+0x20],[])
    if bp==9:  # esi = neighbour entry: [esi+4] = neighbour ptr; ecx = neighbour node; we need its id: not dumped; use entry
        continue
    ev.append((bp,regs,st))
# ptr->id map: from colour successes we only have ptrs; get ids through a second pass with ecx dumps
cmds2=["break *0x401000","cont","delete 1","break *0x417260"]+["cont"]*fi+["break *0x4175a5"]
for _ in range(3000): cmds2+=["cont","x/8x $ecx"]
cmds2.append("quit"); open('rd2_dbg.txt','w').write("\n".join(cmds2)+"\n")
out2=subprocess.run([f'{here}/run_c2dbg.sh','rd2_dbg.txt','ct',src,'rd2_out.obj'],capture_output=True,text=True).stdout
drv=fi
for blk in re.split(r'Stopped on breakpoint (\d) at',out2)[1:]:
    pass
for m in re.finditer(r'Stopped on breakpoint (\d) at[^\n]*\n0x0*([0-9a-f]+)(?: c2\+0x[0-9a-f]+)?:\s+[0-9a-f]+ [0-9a-f]+ [0-9a-f]+ [0-9a-f]+\n0x[0-9a-f]+(?: c2\+0x[0-9a-f]+)?:\s+[0-9a-f]+ [0-9a-f]+ ([0-9a-f]+) ([0-9a-f]+)',out2):
    if m.group(1)=='3': ptrid[int(m.group(2),16)]=int(m.group(3),16)&0xffff
def nm(p): return '%04x'%ptrid[p] if p in ptrid else 'p%x'%p
for e in ev:
    if e[0]=='drv':
        if e[1]>=fi: print('== driver',e[1])
        continue
    bp,regs,st=e
    if bp==3: print('-- outer iteration (graph build)')
    elif bp==4: print('-- simplify+colour, heuristic h=%d'%st[4])
    elif bp==5: print('-- re-simplify+colour with best h=%d'%st[6])
    elif bp==6: print('   %s -> %s'%(nm(st[4]),names[regs['EDI']&0x3f] if (regs['EDI']&0x3f)<8 else hex(regs['EDI']&0x3f)))
    elif bp==7: print('   %s FAIL'%nm(st[4]))
    elif bp==8: print('-- spill (417c80)')
