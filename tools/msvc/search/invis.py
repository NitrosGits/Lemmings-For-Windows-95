#!/usr/bin/env python3
"""Invisible-statement matrix: insert `switch(v){case 1:break;}` / `if(v);` / `v;` after every
statement line of one function and score. usage: invis.py MODULE SRC SIGNATURE FUNCNAME OUTDIR vars,csv [forms,csv]"""
import sys,os,subprocess,re
from multiprocessing import Pool
R='/Users/daniel/Lemmings-For-Windows-95/'
mod,srcp,sig,fn,outd,vars_=sys.argv[1:7]
forms=(sys.argv[7] if len(sys.argv)>7 else 'sw,if,ex').split(',')
PADS=[int(v) for v in (sys.argv[8] if len(sys.argv)>8 else '').split(',') if v!='']
PADRE=sys.argv[9] if len(sys.argv)>9 else ''
vars_=vars_.split(',')
src=open(srcp).read()
lines=src.split('\n')
s0=next(i for i,l in enumerate(lines) if sig in l)
e0=next(i for i in range(s0,len(lines)) if lines[i]=='}')
F={'sw':'switch ({x}) {{ case 1: break; }}','if':'if ({x}) ;','ex':'{x};'}
anchors=[i for i in range(s0+2,e0) if lines[i].rstrip().endswith(';') and not lines[i].strip().startswith(('for','case','return'))]
# only insert where the previous non-blank line is not an unbraced if/else/for header
def ok(i):
    p=lines[i-1].strip()
    return not (re.match(r'(if|else|for|while)\b.*\)$',p) or p=='else')
anchors=[i for i in anchors if ok(i)]
anchors=[0]+anchors  # 0 = baseline
base=os.path.basename(srcp)
def job(a):
    i,f,x,pd=a
    n=f'a{i}_{f}_{x}_p{pd}'
    d=f'{outd}/{n}/'; os.makedirs(d,exist_ok=True)
    L=list(lines)
    if pd>=0:
        k=next(j for j,l in enumerate(L) if l.startswith('extern int '+PADRE))
        L[k]=('extern int '+', '.join(f'{PADRE}{q}' for q in range(pd))+';') if pd else ''
    if i:
        ind=re.match(r'\s*',L[i]).group(0)
        L.insert(i+1,ind+F[f].format(x=x))
    open(d+base,'w').write('\n'.join(L))
    o=subprocess.run([R+'tools/msvc/try.sh',mod,d+base,'-f',fn],capture_output=True,text=True).stdout
    m=re.search(r'(\d+) insns differ',o)
    return n,(m.group(1) if m else ('MATCH' if re.search(r'1/1 functions match',o) else 'fail'))
if __name__=='__main__':
    pl=PADS or [-1]
    jobs=[(0,'sw',vars_[0],pd) for pd in pl]+[(i,f,x,pd) for pd in pl for i in anchors[1:] for f in forms for x in vars_]
    print(len(jobs),flush=True)
    with Pool(4) as p:
        for n,r in p.imap_unordered(job,jobs): print(n,r,flush=True)
