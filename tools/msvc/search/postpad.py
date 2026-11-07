#!/usr/bin/env python3
"""postpad.py FILE FUNC NEXTADDR : insert n pad lines before NEXTADDR's comment (n=0..64), report FUNC score and file count."""
import re,subprocess,sys
ROOT='/Users/daniel/Lemmings-For-Windows-95'
f,fn,nxt=sys.argv[1:4]; MOD=sys.argv[4] if len(sys.argv)>4 else 'LEMMINGS.EXE'
s=open(f'{ROOT}/{f}',encoding='latin1').read()
k=s.index('/* '+nxt); 
for n in range(1,65):
    t=s[:k]+'/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */\nextern int '+', '.join('pad_post%d'%i for i in range(n))+';\n\n'+s[k:]
    p='/tmp/postpad'+f[f.rindex('.'):]; open(p,'w',encoding='latin1').write(t)
    o=subprocess.run([ROOT+'/tools/msvc/try.sh',MOD,p],capture_output=True,text=True).stdout
    m=re.search(re.escape(fn)+r': (MATCH|DIFF.*?(\d+) insns)',o); t2=re.search(r'(\d+)/\d+ functions match',o)
    print(n, 0 if m and m.group(1)=='MATCH' else (m.group(2) if m else '?'), t2.group(1) if t2 else '?', flush=True)
