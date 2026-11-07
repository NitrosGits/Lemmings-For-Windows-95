#!/usr/bin/env python3
"""Numbering-shift search: add k unused locals (k = 0..N) at the front or back of a function's declaration block and score.
C2.EXE's colouring order depends on node ids (id & 0x1f hash buckets), and ids follow declaration/IL order.
usage: numshift.py MODULE SRC SIGNATURE FUNCNAME OUTDIR [kmax=32] [positions=front,back] [jobs=4]"""
import sys,os,re,subprocess
from multiprocessing import Pool
R='/Users/daniel/Lemmings-For-Windows-95/'
mod,srcp,sig,fn,outd=sys.argv[1:6]
kmax=int(sys.argv[6]) if len(sys.argv)>6 else 32
poss=(sys.argv[7] if len(sys.argv)>7 else 'front,back').split(',')
jobs=int(sys.argv[8]) if len(sys.argv)>8 else 4
lines=open(srcp).read().split('\n')
s0=next(i for i,l in enumerate(lines) if sig in l)
b0=next(i for i in range(s0,len(lines)) if lines[i].strip()=='{')
decl=re.compile(r'^\s+(?!return\b|goto\b|else\b|case\b|break\b|if\b|for\b|while\b|switch\b)(?:unsigned\s+|struct\s+)?[A-Za-z_]\w*[\s\*]+\**[A-Za-z_]\w*(\[[^\]]*\])?(\s*=[^;]*)?(,\s*\**\w+(\[[^\]]*\])?(\s*=[^;]*)?)*;\s*$')
j=b0+1
while j<len(lines) and (decl.match(lines[j]) or lines[j].strip()=='' or lines[j].strip().startswith('/*') or lines[j].strip().startswith('//')):
    j+=1
# j = first statement line; back anchor = after the last declaration line before it
last=j-1
while last>b0 and not decl.match(lines[last]): last-=1
front=b0+1; back=last+1
base=os.path.basename(srcp)
def job(a):
    k,pos=a
    L=list(lines)
    ins=[f'    int dm{n};' for n in range(k)]
    at=front if pos=='front' else back
    L[at:at]=ins
    n=f'k{k:02d}_{pos}'
    d=f'{outd}/{n}/'; os.makedirs(d,exist_ok=True)
    open(d+base,'w').write('\n'.join(L))
    try:
        o=subprocess.run([R+'tools/msvc/try.sh',mod,d+base,'-f',fn],capture_output=True,text=True,timeout=400).stdout
    except Exception: return n,'timeout'
    m=re.search(r'(\d+) insns differ',o)
    return n,(m.group(1) if m else ('MATCH' if re.search(r'1/1 functions match',o) else 'fail'))
if __name__=='__main__':
    todo=[(k,p) for p in poss for k in range(kmax+1)]
    print(len(todo),'front line',front+1,'back line',back+1,flush=True)
    with Pool(jobs) as p:
        for n,r in p.imap_unordered(job,todo): print(n,r,flush=True)
