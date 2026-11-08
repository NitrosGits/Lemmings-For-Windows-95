#!/usr/bin/env python3
"""Interference neighbours as seen by C2.EXE's colour assignment (sub_417580) for ONE function.
usage: neigh_trace_fn.py SRC FUNC_INDEX [N_STOPS=3000]   (FUNC_INDEX: see colour_trace_fn.py)
Breaks only inside sub_417580's neighbour loop (0x4175a5, ecx = neighbour node, [esp+0x10] = node being coloured), so no
winedbg command ever fails (a failing x/ command makes winedbg drop the following commands). For each node, in colouring
order, prints its id and its neighbours as id:colour ('-' = not coloured yet). The node's own colour is printed when
it is visible later (as a neighbour of a node coloured after it). Rounds repeat after spills; the LAST round is final.
Nodes without neighbours are not seen at all."""
import os,re,subprocess,sys
here=os.path.dirname(os.path.abspath(__file__))
src=os.path.abspath(sys.argv[1]); fi=int(sys.argv[2]); n=int(sys.argv[3]) if len(sys.argv)>3 else 3000
subprocess.run([f'{here}/run_c1.sh',src,'ct'],check=True)
cmds=["break *0x401000","cont","delete 1","break *0x417260"]
cmds+=["cont"]*fi
cmds+=["break *0x4175a5"]
for _ in range(n): cmds+=["cont","x/8x $ecx","x/8x $esp"]
cmds.append("quit")
open('nb_dbg.txt','w').write("\n".join(cmds)+"\n")
out=subprocess.run([f'{here}/run_c2dbg.sh','nb_dbg.txt','ct',src,'nb_out.obj'],capture_output=True,text=True).stdout
open('nb_raw.txt','w').write(out)
blocks=re.split(r'Stopped on breakpoint (\d) at 0x[0-9a-f]+ c2\+0x[0-9a-f]+\n',out)[1:]
def hx(l): return [int(x,16) for x in re.match(r'0x[0-9a-f]+(?: c2\+0x[0-9a-f]+)?:\s+(.*)',l).group(1).split()]
names='eax ecx edx ebx esp ebp esi edi'.split()
def cn(c):
    c&=0x3f
    return '-' if c==0x20 else (names[c] if c<8 else '%x'%c)
drv=0; seq=[]; ptrid={}; ptrcol={}
for i in range(0,len(blocks),2):
    bp=int(blocks[i]); b=blocks[i+1]
    if bp==2:
        drv+=1
        if drv>=fi: seq.append(('drv',drv))
        continue
    mem=[hx(l) for l in b.split('\n') if re.match(r'0x[0-9a-f]+(?: c2\+0x[0-9a-f]+)?:\s+[0-9a-f]{8}',l)]
    if len(mem)<4: continue
    nb=sum(mem[0:2],[]); st=sum(mem[2:4],[])
    a=re.search(r'0x0*([0-9a-f]+)(?: c2\+0x[0-9a-f]+)?:',b).group(1); nptr=int(a,16)
    node=st[4]; nid=nb[6]&0xffff
    ptrid[nptr]=nid
    if not seq or seq[-1][0]!='node' or seq[-1][1]!=node: seq.append(['node',node,[]])
    seq[-1][2].append((nid,nb[7]))
# a node's colour as seen later
for s in seq:
    if s[0]=='drv': print('== driver',s[1]); continue
    _,node,nbs=s
    print('%s | %s'%(('%04x'%ptrid[node]) if node in ptrid else 'p%x'%node,' '.join('%04x:%s'%(i,cn(c)) for i,c in nbs)))
