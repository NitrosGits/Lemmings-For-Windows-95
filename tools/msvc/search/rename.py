import re,os,subprocess,random,sys
from multiprocessing import Pool
R='/Users/daniel/Lemmings-For-Windows-95/'
srcp,mod,sig,fn,locs,N=sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5].split(','),int(sys.argv[6])
src=open(R+srcp).read()
lines=src.split('\n')
s0=next(i for i,l in enumerate(lines) if sig in l)
e0=next(i for i in range(s0,len(lines)) if lines[i]=='}')
body='\n'.join(lines[s0:e0+1])
pool=['a','b','c','d','e','i','j','k','m','n','p','q','r','s','t','u','v','w','xx','yy','px','py','cx','cy','tmp','pos','nx','ny','nxt','cur','step','row','col','idx','val','res','ret','dx','dy','x1','y1','x2','y2','sx','sy','top','end','lo','hi','pt','pnt','pix','hit','ok','dir','len','cnt','off','pad','tx','ty','newx','newy']
base=os.path.basename(srcp)
rng=random.Random(1)
def sub(body,mp):
    for old,new in mp.items():
        body=re.sub(r'(?<![\w.>])'+re.escape(old)+r'\b',new,body)
    return body
def job(k):
    rg=random.Random(k)
    names=rg.sample(pool,len(locs))
    mp=dict(zip(locs,['__'+l for l in locs]))
    b=sub(body,mp); b=sub(b,{'__'+l:n for l,n in zip(locs,names)}) if True else b
    # careful: struct fields l->x etc untouched thanks to lookbehind for . and >
    s='\n'.join(lines[:s0])+'\n'+b+'\n'+'\n'.join(lines[e0+1:])
    d=R+f'build/scratch/s36_claude/ren_{fn.replace(":","_")}/{k}/'; os.makedirs(d,exist_ok=True)
    open(d+base,'w').write(s)
    o=subprocess.run([R+'tools/msvc/try.sh',mod,d+base,'-f',fn],capture_output=True,text=True).stdout
    m=re.search(r'(\d+) insns differ',o)
    return k,names,(m.group(1) if m else ('MATCH' if '1/1 functions' in o else 'fail'))
if __name__=='__main__':
    with Pool(4) as p:
        for k,nm,r in p.imap_unordered(job,range(N)): print(k,r,' '.join(nm),flush=True)
