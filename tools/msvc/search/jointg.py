#!/usr/bin/env python3
"""jointg.py FILE N SEED ADDR:NAME ... : random joint padding (0-48) before each listed function; prints combo, total insns, file match count."""
import random,re,subprocess,os,sys
from concurrent.futures import ThreadPoolExecutor
ROOT='/Users/daniel/Lemmings-For-Windows-95'
f,N,seed=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]); specs=[a.split(':',1) for a in sys.argv[4:]]
base=open(f'{ROOT}/{f}',encoding='latin1').read()
def make(t):
    s=base
    for (addr,name),n in zip(specs,t):
        k=s.index('/* '+addr)
        if n: s=s[:k]+'/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */\nextern int '+', '.join('pad_j%s_%d'%(addr[2:],i) for i in range(n))+';\n\n'+s[k:]
    return s
def run(t):
    p='/tmp/jg_'+'_'.join(map(str,t))+os.path.splitext(f)[1]
    open(p,'w',encoding='latin1').write(make(t))
    o=subprocess.run([ROOT+'/tools/msvc/try.sh','LEMMINGS.EXE',p],capture_output=True,text=True).stdout
    os.remove(p); tot=0
    for _,fn in specs:
        m=re.search(re.escape(fn)+r': (MATCH|DIFF.*?(\d+) insns)',o); tot+=int(m.group(2)) if m and m.group(2) else (0 if m else 999)
    c=re.search(r'(\d+)/\d+ functions match',o)
    return t,tot,int(c.group(1)) if c else 0
random.seed(seed); cands={tuple([0]*len(specs))}
while len(cands)<N: cands.add(tuple(random.randint(0,48) for _ in specs))
with ThreadPoolExecutor(5) as ex:
    for t,tot,c in ex.map(run,sorted(cands)): print(t,tot,c,flush=True)
print('JGDONE')
