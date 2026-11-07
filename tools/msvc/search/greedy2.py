#!/usr/bin/env python3
"""greedy2.py MODULE SRC 'SIG' BLOCKS.py : greedy block-permutation search; BLOCKS.py defines BLOCKS=[[line,...],...] (exact lines in body)."""
import itertools,re,subprocess,sys,runpy
ROOT='/Users/daniel/Lemmings-For-Windows-95'
mod,src,sig,bf=sys.argv[1:5]
s=open(src,encoding='latin1').read()
i=s.index(sig+'\n{')+len(sig)+2; j=s.index('\n}\n',i); base='\n'+s[i:j].strip('\n')+'\n'
B=runpy.run_path(bf)['BLOCKS']
tmp=ROOT+'/build/scratch/s30_sound/g2_'
def score(bodies):
    open(tmp+'many.txt','w',encoding='latin1').write('\n%%\n'.join(b.strip('\n')+'\n' for b in bodies))
    o=subprocess.run(['python3',ROOT+'/tools/msvc/variants.py',mod,src,sig,tmp+'many.txt','--jobs','5'],capture_output=True,text=True).stdout
    m=re.search(r'^\s+(\d+)\s+(MATCH|\d+)',o,re.M)
    return int(m.group(1)), (0 if m.group(2)=='MATCH' else int(m.group(2)))
_,best=score([base]); print('base',best,flush=True)
for blk in B:
    old='\n'.join(blk)
    if old not in base: print('missing',blk[0].strip()); continue
    cands=[base.replace(old,'\n'.join(p),1) for p in itertools.permutations(blk)]
    k,sc=score(cands); print(blk[0].strip(),'->',sc,flush=True)
    if sc<best: best=sc; base=cands[k]
open(tmp+'best.txt','w',encoding='latin1').write(base)
print('final',best)
