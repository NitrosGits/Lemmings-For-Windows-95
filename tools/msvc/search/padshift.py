#!/usr/bin/env python3
"""2-D numbering grid: P pad externs (global symbol numbering) x K unused locals at the front of the function (local numbering).
usage: padshift.py MODULE SRC SIGNATURE FUNCNAME OUTDIR PADPREFIX [pmax=40] [kmax=32] [jobs=4]
If a line 'extern int PADPREFIX0, ...;' exists it is replaced by P names (P=0 drops it); otherwise the pad line is inserted
right before the function's leading comment/signature."""
import sys,os,re,subprocess
from multiprocessing import Pool
R='/Users/daniel/Lemmings-For-Windows-95/'
mod,srcp,sig,fn,outd,pre=sys.argv[1:7]
pmax=int(sys.argv[7]) if len(sys.argv)>7 else 40
kmax=int(sys.argv[8]) if len(sys.argv)>8 else 32
jobs=int(sys.argv[9]) if len(sys.argv)>9 else 4
lines=open(srcp).read().split('\n')
s0=next(i for i,l in enumerate(lines) if sig in l)
b0=next(i for i in range(s0,len(lines)) if lines[i].strip()=='{')
pl=next((i for i,l in enumerate(lines) if re.match(r'\s*extern int '+re.escape(pre)+r'_?0\b',l)),None)
base=os.path.basename(srcp)
sep='_' if pl is not None and re.match(r'\s*extern int '+re.escape(pre)+'_0',lines[pl]) else ''
# insertion point for a new block: before the comment lines directly above the signature
ins=s0
while ins>0 and (lines[ins-1].strip().startswith(('/*','*','//')) ): ins-=1
def job(a):
    p,k=a
    L=list(lines)
    L[b0+1:b0+1]=[f'    int dm{n};' for n in range(k)]
    names=', '.join(f'{pre}{sep}{n}' for n in range(p))
    if pl is not None: L[pl]=(f'extern int {names};' if p else '')
    elif p: L.insert(ins,f'extern int {names};')
    n=f'p{p:02d}_k{k:02d}'
    d=f'{outd}/{n}/'; os.makedirs(d,exist_ok=True)
    open(d+base,'w').write('\n'.join(L))
    try: o=subprocess.run([R+'tools/msvc/try.sh',mod,d+base,'-f',fn],capture_output=True,text=True,timeout=400).stdout
    except Exception: return n,'timeout'
    m=re.search(r'(\d+) insns differ',o)
    return n,(m.group(1) if m else ('MATCH' if re.search(r'1/1 functions match',o) else 'fail'))
if __name__=='__main__':
    todo=[(p,k) for p in range(pmax+1) for k in range(kmax+1)]
    print(len(todo),'existing pad line',None if pl is None else pl+1,flush=True)
    with Pool(jobs) as po:
        for n,r in po.imap_unordered(job,todo): print(n,r,flush=True)
