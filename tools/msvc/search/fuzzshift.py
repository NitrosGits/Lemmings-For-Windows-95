#!/usr/bin/env python3
"""Random declaration fuzz + numbering shift: for one function write N variants (shuffled uninitialised declarations, random
integer retyping, 0..32 unused dummy locals at the front) as a bodies JSON for bodyshift.py (run it with kmin=kmax=0).
usage: fuzzshift.py SRC SIGNATURE OUT.json [n=600] [seed=1]"""
import sys,re,json,random
srcp,sig,out=sys.argv[1:4]
n=int(sys.argv[4]) if len(sys.argv)>4 else 600
random.seed(int(sys.argv[5]) if len(sys.argv)>5 else 1)
s=open(srcp).read()
i=s.index(sig); b=s.index('{\n',i)+2; e=s.index('\n}\n',b)
lines=s[b:e].split('\n')
DECL=re.compile(r'^(\s*)((?:register\s+)?(?:unsigned\s+|struct\s+)?[A-Za-z_]\w*[\s\*]+)(\**[A-Za-z_]\w*(?:\[[^\]]*\])?)(\s*=[^;]*)?;\s*$')
nd=0
while nd<len(lines) and (DECL.match(lines[nd]) or not lines[nd].strip()) and not re.match(r'\s*(return|if|for|while|switch)\b',lines[nd]): nd+=1
dl=[DECL.match(l).groups() for l in lines[:nd] if DECL.match(l)]
rest=lines[nd:]
INTS=['int','long','DWORD','UINT','unsigned int']
def ret(t):
    t0=t.strip()
    return random.choice(INTS)+' ' if t0 in INTS else t
bodies=[]; seen=set()
tries=0
while len(bodies)<n and tries<n*20:
    tries+=1
    items=[list(x) for x in dl]
    un=[k for k,x in enumerate(items) if not x[3]]
    vals=[items[k] for k in un]; random.shuffle(vals)
    for k,v in zip(un,vals): items[k]=v
    for x in items:
        if not x[3] and random.random()<0.5: x[1]=ret(x[1])
    k=random.randrange(33)
    d=[f'    int dm{j};' for j in range(k)]+[f'{x[0] or "    "}{x[1]}{x[2]}{x[3] or ""};' for x in items]
    t='\n'.join(d+['']+rest)
    if t in seen: continue
    seen.add(t); bodies.append([f'f{len(bodies):04d}_k{k}',t])
json.dump(bodies,open(out,'w'))
print(len(bodies),'variants,',len(dl),'declarations')
