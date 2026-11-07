#!/usr/bin/env python3
"""Wider numbering-shift search: k dummy locals of a given kind inserted at EVERY slot of a function's declaration block
(slot s = before the s-th declaration line, last slot = after the last one). See numshift.py and docs/C2_REGALLOC.md.
(env NUMSHIFT_SLOTS=0,1 limits the slots tried)
usage: numshift2.py MODULE SRC SIGNATURE FUNCNAME OUTDIR [kmin=1] [kmax=32] [kind='int'] [jobs=4]
  kind examples: int | short | char | 'int *' | 'int'+suffix via kind='int|[2]' (text after | is appended after the name)"""
import sys,os,re,subprocess
from multiprocessing import Pool
R='/Users/daniel/Lemmings-For-Windows-95/'
mod,srcp,sig,fn,outd=sys.argv[1:6]
kmin=int(sys.argv[6]) if len(sys.argv)>6 else 1
kmax=int(sys.argv[7]) if len(sys.argv)>7 else 32
kind=sys.argv[8] if len(sys.argv)>8 else 'int'
jobs=int(sys.argv[9]) if len(sys.argv)>9 else 4
ty,_,suf=kind.partition('|')
lines=open(srcp).read().split('\n')
s0=next(i for i,l in enumerate(lines) if sig in l)
b0=next(i for i in range(s0,len(lines)) if lines[i].strip()=='{')
decl=re.compile(r'^\s*(?!return\b|goto\b|else\b|case\b|break\b|if\b|for\b|while\b|switch\b)(?:register\s+|unsigned\s+|struct\s+)?[A-Za-z_]\w*[\s\*]+\**[A-Za-z_]\w*(\[[^\]]*\])?(\s*=[^;]*)?(,\s*\**\w+(\[[^\]]*\])?(\s*=[^;]*)?)*;\s*$')
dl=[]
j=b0+1
while j<len(lines) and (decl.match(lines[j]) or lines[j].strip()=='' or lines[j].strip().startswith('/*') or lines[j].strip().startswith('//')):
    if decl.match(lines[j]): dl.append(j)
    j+=1
slots=[dl[0]]+[d+1 for d in dl]   # before first decl, then after each decl
only=os.environ.get('NUMSHIFT_SLOTS')
sel=[int(x) for x in only.split(',')] if only else list(range(len(slots)))
base=os.path.basename(srcp)
def job(a):
    k,s=a
    L=list(lines)
    L[slots[s]:slots[s]]=[f'    {ty} dm{n}{suf};' for n in range(k)]
    n=f's{s:02d}_k{k:02d}'
    d=f'{outd}/{n}/'; os.makedirs(d,exist_ok=True)
    open(d+base,'w').write('\n'.join(L))
    try:
        o=subprocess.run([R+'tools/msvc/try.sh',mod,d+base,'-f',fn],capture_output=True,text=True,timeout=400).stdout
    except Exception: return n,'timeout'
    m=re.search(r'(\d+) insns differ',o)
    return n,(m.group(1) if m else ('MATCH' if re.search(r'1/1 functions match',o) else 'fail'))
if __name__=='__main__':
    todo=[(k,s) for s in sel for k in range(kmin,kmax+1)]
    print(len(todo),'slots',len(slots),'kind',kind,flush=True)
    with Pool(jobs) as p:
        for n,r in p.imap_unordered(job,todo): print(n,r,flush=True)
