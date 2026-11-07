#!/usr/bin/env python3
"""randdecl.py SRC 'SIG' N SEED OUT : N random orders of the leading declaration block of the function body."""
import random,re,sys
src,sig,n,seed,out=sys.argv[1],sys.argv[2],int(sys.argv[3]),int(sys.argv[4]),sys.argv[5]
s=open(src,encoding='latin1').read()
i=s.index(sig+'\n{')+len(sig)+2; j=s.index('\n}\n',i); b=s[i:j]
lines=b.split('\n'); k=0
while k<len(lines) and (lines[k].strip()=='' ): k+=1
d=[]
while k<len(lines) and lines[k].strip() and re.match(r'\s+[A-Za-z_][\w \*]*[\s\*]\w+(\[[^\]]*\])?( = [^;]*)?;$',lines[k]) and '(' not in lines[k].split('=')[0]:
    d.append(lines[k]); k+=1
rest='\n'.join(lines[k:])
# keep decls with initialisers that reference other decls in relative order
names=[re.search(r'(\w+)(\[[^\]]*\])?( = |;)',x).group(1) for x in d]
deps={a:[bn for bn in names if bn!=a and re.search(r'(?<![.>\w])%s\b'%bn, x.split('=',1)[1]) ] if '=' in x else [] for a,x in zip(names,d)}
random.seed(seed); outs=['\n'+'\n'.join(d)+'\n'+rest+'\n']; seen=set()
tries=0
while len(outs)<n and tries<n*50:
    tries+=1; p=list(range(len(d))); random.shuffle(p)
    order=[names[q] for q in p]
    if any(order.index(dd)>order.index(a) for a in names for dd in deps[a]): continue
    key=tuple(p)
    if key in seen: continue
    seen.add(key); outs.append('\n'+'\n'.join(d[q] for q in p)+'\n'+rest+'\n')
open(out,'w',encoding='latin1').write('\n%%\n'.join(o.strip('\n')+'\n' for o in outs))
print(len(d),'decls',len(outs),'variants')
