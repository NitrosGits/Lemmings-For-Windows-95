"""jumptable.py OBJ [FUNC_OFF_HEX]: (dword entries, byte index table) of the first `jmp [eax*4+tbl]` switch at/after FUNC_OFF in a VC++ COFF .obj (resolves the table addresses through relocations + symbols)."""
import struct,sys
def jt(path,start=0):
    o=open(path,'rb').read()
    ns=struct.unpack_from('<H',o,2)[0]; symp,nsym=struct.unpack_from('<II',o,8); opt=struct.unpack_from('<H',o,16)[0]
    syms={}
    i=0
    while i<nsym:
        name,val,sec,typ,cls,naux=struct.unpack_from('<8sIhHBB',o,symp+18*i)
        syms[i]=(val,sec); i+=1+naux
    for s in range(ns):
        name,vs,va,size,ptr,rptr,lptr,nr,nl,ch=struct.unpack_from('<8sIIIIIIHHI',o,20+opt+40*s)
        if name.startswith(b'.text'):
            tx=o[ptr:ptr+size]; rel={}
            for k in range(nr):
                ra,si,ty=struct.unpack_from('<IIH',o,rptr+10*k)
                rel[ra]=syms[si][0]+struct.unpack_from('<I',tx,ra)[0]
            break
    j=tx.find(b'\xff\x24\x85',start)
    if j<0: return None
    dtab=rel.get(j+3)
    k=tx.rfind(b'\x8a',max(0,j-12),j)
    btab=rel.get(k+2)
    n=(btab-dtab)//4
    return n,list(tx[btab:btab+32])
if __name__=='__main__':
    print(jt(sys.argv[1],int(sys.argv[2],16) if len(sys.argv)>2 else 0))
