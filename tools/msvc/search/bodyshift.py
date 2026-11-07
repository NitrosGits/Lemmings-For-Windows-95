#!/usr/bin/env python3
"""Cross a set of alternative function bodies with the numbering shift (k unused locals at the front).
usage: bodyshift.py MODULE SRC SIGNATURE FUNCNAME BODIES.json OUTDIR [kmin=0] [kmax=32] [jobs=4]
BODIES.json = [[label, body_text], ...] where body_text replaces everything between the function's '{' and its closing '}'."""
import sys,os,re,json,subprocess
from multiprocessing import Pool
R='/Users/daniel/Lemmings-For-Windows-95/'
mod,srcp,sig,fn,bj,outd=sys.argv[1:7]
kmin=int(sys.argv[7]) if len(sys.argv)>7 else 0
kmax=int(sys.argv[8]) if len(sys.argv)>8 else 32
jobs=int(sys.argv[9]) if len(sys.argv)>9 else 4
src=open(srcp).read()
i=src.index(sig); b=src.index('{\n',i)+2; e=src.index('\n}\n',b)
pre,post=src[:b],src[e:]
bodies=json.load(open(bj))
base=os.path.basename(srcp)
def job(a):
    bi,k=a
    body=bodies[bi][1].strip('\n')
    txt=pre+''.join(f'    int dm{n};\n' for n in range(k))+body+post
    n=f'b{bi:03d}_k{k:02d}'
    d=f'{outd}/{n}/'; os.makedirs(d,exist_ok=True)
    open(d+base,'w').write(txt)
    try: o=subprocess.run([R+'tools/msvc/try.sh',mod,d+base,'-f',fn],capture_output=True,text=True,timeout=400).stdout
    except Exception: return n,'timeout'
    m=re.search(r'(\d+) insns differ',o)
    if m: return n,m.group(1)
    return n,('MATCH' if re.search(r'1/1 functions match',o) else 'fail')
if __name__=='__main__':
    todo=[(bi,k) for bi in range(len(bodies)) for k in range(kmin,kmax+1)]
    print(len(todo),'bodies',len(bodies),flush=True)
    with Pool(jobs) as p:
        for n,r in p.imap_unordered(job,todo): print(n,r,flush=True)
