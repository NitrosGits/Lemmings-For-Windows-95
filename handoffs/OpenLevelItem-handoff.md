# OpenLevelItem hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **LEMMINGS.EXE**, function at **0x40b3e0**, source `src/lemmings/levelsel.cpp`.
> Status: `0x40b3e0 OpenLevelItem: DIFF (768 bytes) 283 bytes differ, first at +0x17, 27 insns differ; +0x1db ?g_treeNext@@3UTREE@@A: original 0x134fe800, expected 0x416880; +0x1e0 ?TreeOpen@@YAXPAUTREE@@H@Z: original 0x8c48300, expected 0x134c`

## Goal
Make `OpenLevelItem` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh LEMMINGS.EXE src/lemmings/levelsel.cpp -v -f OpenLevelItem`) to bytes identical to the original, without breaking the other functions in `levelsel.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh LEMMINGS.EXE src/lemmings/levelsel.cpp -v -f OpenLevelItem
python3 tools/msvc/realname.py LEMMINGS.EXE src/lemmings/levelsel.cpp 0x40b3e0 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x40b3e0: open or close a rating or group, or choose a level */
void OpenLevelItem(HWND hDlg, HWND list, int i, WORD sel)
{
    int added;
    int idInit = g_db->entries[i].id;
    int show;
    int prev;
    int t;
    int type = g_db->entries[i].type;
    int childType = g_db->entries[i].childType;
    int id = idInit;
    int j;
    int status;
    WORD pos;
    int kind;
    WORD children = g_db->entries[i].children;
    int shown = g_db->entries[i].shown;

    if (type == 3) {
        g_currentLevel = LevelNumber(id);
        CloseLevelDialog(hDlg, 1);
        return;
    }
    if (type == 0x101)
        kind = 0x101;
    else if (type == 0x102)
        kind = 0x102;
    else
        return;
    if (TreeIsOpen(&g_treeNext, i)) {
        TreeClose(&g_treeNext, i);
        sel++;
        j = SendMessage(list, LB_GETITEMDATA, sel, 0);
        t = g_db->entries[j].type;
        while (shown-- > 0 && t != kind) {
            t = g_db->entries[j].type;
            if ((t & 0x100) && TreeIsOpen(&g_treeNext, j)) {
                TreeClose(&g_treeNext, j);
                shown += g_db->entries[j].shown;
            }
            SendMessage(list, LB_DELETESTRING, sel, 0);
            if (shown <= 0 || kind == t)
                break;
            j = SendMessage(list, LB_GETITEMDATA, sel, 0);
            t = g_db->entries[j].type;
        }
        return;
    }
    prev = 1;
    TreeOpen(&g_treeNext, i);
    if (type == 0x101 || type == 0x102) {
        SendMessage(list, WM_SETREDRAW, FALSE, 0);
        pos = sel;
        LevelIndex(g_currentLevel);
        added = 0;
        j = id;
        while (children) {
            j++;
            t = g_db->entries[j].type;
            status = g_db->entries[j].status;
            if (childType == t) {
                children--;
                pos++;
                show = 1;
                if (t == 3)
                    show = prev || status;
                prev = status;
                if (show) {
                    SendMessage(list, LB_INSERTSTRING, pos, j);
                    added++;
                }
            }
        }
        g_db->entries[i].shown = added;
        TreeEnsureVisible(&g_treeNext, list, sel, 9);
        SendMessage(list, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(list, 0, TRUE);
    }
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     0040b3e0  mov    eax,DWORD PTR [esp+0xc]                | mov    eax,DWORD PTR [esp+0xc]
     0040b3e4  sub    esp,0x10                               | sub    esp,0x10
     0040b3e7  shl    eax,0x4                                | shl    eax,0x4
     0040b3ea  push   ebx                                    | push   ebx
     0040b3eb  add    eax,DWORD PTR [esp+0x20]               | add    eax,DWORD PTR [esp+0x20]
     0040b3ef  push   esi                                    | push   esi
     0040b3f0  mov    ecx,DWORD PTR [esp+0x24]               | mov    ecx,DWORD PTR [esp+0x24]
     0040b3f4  push   edi                                    | push   edi
     0040b3f5  push   ebp                                    | push   ebp
   * 0040b3f6  xor    ebx,ebx                                | xor    edi,edi
     0040b3f8  lea    edx,[ecx+eax*8]                        | lea    edx,[ecx+eax*8]
   * 0040b3fb  mov    ecx,DWORD PTR ds:0xcad40d8b            | xor    ebx,ebx
   * 0040b401  add    ecx,edx                                | mov    ecx,DWORD PTR ds:0x41cad4
     0040b403  xor    eax,eax                                | xor    eax,eax
   * 0040b405  mov    bx,WORD PTR [ecx+0xa]                  | add    ecx,edx
   * 0040b409  xor    edi,edi                                | mov    DWORD PTR [esp+0x1c],edx
   * 0040b40b  mov    ax,WORD PTR [ecx+0xc]                  | mov    di,WORD PTR [ecx+0x8]
     0040b40f  xor    esi,esi                                | xor    esi,esi
   * 0040b411  mov    di,WORD PTR [ecx+0x8]                  | mov    bx,WORD PTR [ecx+0xa]
   * 0040b415  mov    DWORD PTR [esp+0x1c],edx               | mov    ax,WORD PTR [ecx+0xc]
   * 0040b419  mov    bp,WORD PTR [ecx+0x7e]                 | cmp    ebx,0x3
   * 0040b41d  mov    DWORD PTR [esp+0x18],eax               | mov    bp,WORD PTR [ecx+0x7e]
   * 0040b421  mov    si,WORD PTR [ecx+0x80]                 | mov    DWORD PTR [esp+0x18],eax
   * 0040b428  cmp    ebx,0x3                                | mov    si,WORD PTR [ecx+0x80]
     0040b42b  jne    0x40b452                               | jne    0x40b452
     0040b42d  push   edi                                    | push   edi
     0040b42e  call   0x40a800                               | call   0x40a800
     0040b433  add    esp,0x4                                | add    esp,0x4
     0040b436  mov    ds:0x41ba3c,eax                        | mov    ds:0x41ba3c,eax
     0040b43b  mov    eax,DWORD PTR [esp+0x24]               | mov    eax,DWORD PTR [esp+0x24]
     0040b43f  push   0x1                                    | push   0x1
     0040b441  push   eax                                    | push   eax
     0040b442  call   0x40a840                               | call   0x40a840
     0040b447  add    esp,0x8                                | add    esp,0x8
     0040b44a  pop    ebp                                    | pop    ebp
     0040b44b  pop    edi                                    | pop    edi
     0040b44c  pop    esi                                    | pop    esi
     0040b44d  pop    ebx                                    | pop    ebx
     0040b44e  add    esp,0x10                               | add    esp,0x10
     0040b451  ret                                           | ret
     0040b452  cmp    ebx,0x101                              | cmp    ebx,0x101
     0040b458  jne    0x40b464                               | jne    0x40b464
     0040b45a  mov    DWORD PTR [esp+0x14],0x101             | mov    DWORD PTR [esp+0x14],0x101
     0040b461                                                | 
     0040b462  jmp    0x40b478                               | jmp    0x40b478
     0040b464  cmp    ebx,0x102                              | cmp    ebx,0x102
   * 0040b46a  jne    0x40b6d3                               | jne    0x40b6d9
     0040b470  mov    DWORD PTR [esp+0x14],0x102             | mov    DWORD PTR [esp+0x14],0x102
     0040b477                                                | 
     0040b478  mov    ecx,DWORD PTR [esp+0x2c]               | mov    ecx,DWORD PTR [esp+0x2c]
     0040b47c  push   ecx                                    | push   ecx
     0040b47d  push   0x416880                               | push   0x416880
     0040b482  call   0x40c8e0                               | call   0x40c8e0
     0040b487  add    esp,0x8                                | add    esp,0x8
     0040b48a  test   eax,eax                                | test   eax,eax
     0040b48c  je     0x40b5ad                               | je     0x40b5ad
     0040b492  mov    ecx,DWORD PTR [esp+0x2c]               | mov    ecx,DWORD PTR [esp+0x2c]
     0040b496  push   ecx                                    | push   ecx
     0040b497  push   0x416880                               | push   0x416880
     0040b49c  call   0x40c960                               | call   0x40c960
     0040b4a1  inc    WORD PTR [esp+0x38]                    | inc    WORD PTR [esp+0x38]
     0040b4a6  add    esp,0x8                                | add    esp,0x8
     0040b4a9  mov    eax,DWORD PTR [esp+0x30]               | mov    eax,DWORD PTR [esp+0x30]
     0040b4ad  push   0x0                                    | push   0x0
     0040b4af  and    eax,0xffff                             | and    eax,0xffff
     0040b4b4  mov    ecx,DWORD PTR [esp+0x2c]               | mov    ecx,DWORD PTR [esp+0x2c]
     0040b4b8  mov    DWORD PTR [esp+0x20],eax               | mov    DWORD PTR [esp+0x20],eax
     0040b4bc  push   eax                                    | push   eax
     0040b4bd  push   0x199                                  | push   0x199
     0040b4c2  push   ecx                                    | push   ecx
     0040b4c3  call   DWORD PTR ds:0x41f7f4                  | call   DWORD PTR ds:0x41f7f4
     0040b4c9  mov    edi,eax                                | mov    edi,eax
     0040b4cb  mov    edx,DWORD PTR ds:0x41cad4              | mov    edx,DWORD PTR ds:0x41cad4
     0040b4d1  shl    eax,0x4                                | shl    eax,0x4
     0040b4d4  xor    ecx,ecx                                | xor    ecx,ecx
     0040b4d6  add    eax,edi                                | add    eax,edi
     0040b4d8  lea    ebx,[edi+eax*8]                        | lea    ebx,[edi+eax*8]
     0040b4db  mov    eax,esi                                | mov    eax,esi
     0040b4dd  mov    cx,WORD PTR [ebx+edx*1+0xa]            | mov    cx,WORD PTR [ebx+edx*1+0xa]
     0040b4e2  dec    esi                                    | dec    esi
     0040b4e3  test   eax,eax                                | test   eax,eax
   * 0040b4e5  jle    0x40b6d3                               | jle    0x40b6d9
     0040b4eb  cmp    DWORD PTR [esp+0x14],ecx               | cmp    DWORD PTR [esp+0x14],ecx
   * 0040b4ef  je     0x40b6d3                               | je     0x40b6d9
     0040b4f5  mov    eax,edi                                | mov    eax,edi
     0040b4f7  mov    ecx,DWORD PTR ds:0x41cad4              | mov    ecx,DWORD PTR ds:0x41cad4
     0040b4fd  shl    eax,0x4                                | shl    eax,0x4
     0040b500  xor    ebx,ebx                                | xor    ebx,ebx
     0040b502  add    eax,edi                                | add    eax,edi
     0040b504  lea    ebp,[edi+eax*8]                        | lea    ebp,[edi+eax*8]
     0040b507  mov    bx,WORD PTR [ebp+ecx*1+0xa]            | mov    bx,WORD PTR [ebp+ecx*1+0xa]
     0040b50c  test   bh,0x1                                 | test   bh,0x1
     0040b50f  je     0x40b543                               | je     0x40b543
     0040b511  push   edi                                    | push   edi
     0040b512  push   0x416880                               | push   0x416880
     0040b517  call   0x40c8e0                               | call   0x40c8e0
     0040b51c  add    esp,0x8                                | add    esp,0x8
     0040b51f  test   eax,eax                                | test   eax,eax
     0040b521  je     0x40b543                               | je     0x40b543
     0040b523  push   edi                                    | push   edi
     0040b524  push   0x416880                               | push   0x416880
     0040b529  call   0x40c960                               | call   0x40c960
     0040b52e  add    esp,0x8                                | add    esp,0x8
     0040b531  mov    ecx,DWORD PTR ds:0x41cad4              | mov    ecx,DWORD PTR ds:0x41cad4
     0040b537  xor    eax,eax                                | xor    eax,eax
     0040b539  mov    ax,WORD PTR [ebp+ecx*1+0x80]           | mov    ax,WORD PTR [ebp+ecx*1+0x80]
     0040b540                                                | 
     0040b541  add    esi,eax                                | add    esi,eax
     0040b543  mov    eax,DWORD PTR [esp+0x1c]               | mov    eax,DWORD PTR [esp+0x1c]
     0040b547  push   0x0                                    | push   0x0
     0040b549  mov    ecx,DWORD PTR [esp+0x2c]               | mov    ecx,DWORD PTR [esp+0x2c]
     0040b54d  push   eax                                    | push   eax
     0040b54e  push   0x182                                  | push   0x182
     0040b553  push   ecx                                    | push   ecx
     0040b554  call   DWORD PTR ds:0x41f7f4                  | call   DWORD PTR ds:0x41f7f4
     0040b55a  test   esi,esi                                | test   esi,esi
   * 0040b55c  jle    0x40b6d3                               | jle    0x40b6d9
     0040b562  cmp    DWORD PTR [esp+0x14],ebx               | cmp    DWORD PTR [esp+0x14],ebx
   * 0040b566  je     0x40b6d3                               | je     0x40b6d9
     0040b56c  mov    eax,DWORD PTR [esp+0x1c]               | mov    eax,DWORD PTR [esp+0x1c]
     0040b570  push   0x0                                    | push   0x0
     0040b572  mov    ecx,DWORD PTR [esp+0x2c]               | mov    ecx,DWORD PTR [esp+0x2c]
     0040b576  push   eax                                    | push   eax
     0040b577  push   0x199                                  | push   0x199
     0040b57c  push   ecx                                    | push   ecx
     0040b57d  call   DWORD PTR ds:0x41f7f4                  | call   DWORD PTR ds:0x41f7f4
     0040b583  mov    edi,eax                                | mov    edi,eax
     0040b585  mov    edx,DWORD PTR ds:0x41cad4              | mov    edx,DWORD PTR ds:0x41cad4
     0040b58b  shl    eax,0x4                                | shl    eax,0x4
     0040b58e  xor    ecx,ecx                                | xor    ecx,ecx
     0040b590  add    eax,edi                                | add    eax,edi
     0040b592  lea    ebx,[edi+eax*8]                        | lea    ebx,[edi+eax*8]
     0040b595  mov    eax,esi                                | mov    eax,esi
     0040b597  mov    cx,WORD PTR [ebx+edx*1+0xa]            | mov    cx,WORD PTR [ebx+edx*1+0xa]
     0040b59c  dec    esi                                    | dec    esi
     0040b59d  test   eax,eax                                | test   eax,eax
     0040b59f  jg     0x40b4eb                               | jg     0x40b4eb
     0040b5a5  pop    ebp                                    | pop    ebp
     0040b5a6  pop    edi                                    | pop    edi
     0040b5a7  pop    esi                                    | pop    esi
     0040b5a8  pop    ebx                                    | pop    ebx
     0040b5a9  add    esp,0x10                               | add    esp,0x10
     0040b5ac  ret                                           | ret
   * 0040b5ad  mov    DWORD PTR [esp+0x14],0x1               | mov    esi,0x1
   * 0040b5b4                                                | mov    ecx,DWORD PTR [esp+0x2c]
   * 0040b5b5  mov    ecx,DWORD PTR [esp+0x2c]               | push   ecx
   * 0040b5b9  push   ecx                                    | push   0x416880
   * 0040b5ba  push   0x416880                               | call   0x40c910
   * 0040b5bf  call   0x40c910                               | add    esp,0x8
   * 0040b5c4  add    esp,0x8                                | cmp    ebx,0x101
   * 0040b5c7  cmp    ebx,0x101                              | je     0x40b5d8
   * 0040b5cd  je     0x40b5db                               | cmp    ebx,0x102
   * 0040b5cf  cmp    ebx,0x102                              | jne    0x40b6d9
   * 0040b5d5  jne    0x40b6d3                               | mov    eax,DWORD PTR [esp+0x28]
   * 0040b5db  mov    eax,DWORD PTR [esp+0x28]               | push   0x0
     0040b5df  push   0x0                                    | push   0x0
   * 0040b5e1  push   0x0                                    | push   0xb
   * 0040b5e3  push   0xb                                    | push   eax
   * 0040b5e5  push   eax                                    | call   DWORD PTR ds:0x41f7f4
   * 0040b5e6  call   DWORD PTR ds:0x41f7f4                  | mov    ax,WORD PTR [esp+0x30]
   * 0040b5ec  mov    ax,WORD PTR [esp+0x30]                 | mov    ecx,DWORD PTR ds:0x41ba3c
   * 0040b5f1  mov    ecx,DWORD PTR ds:0x41ba3c              | mov    WORD PTR [esp+0x12],ax
   * 0040b5f7  mov    WORD PTR [esp+0x12],ax                 | push   ecx
   * 0040b5fc  push   ecx                                    | call   0x40b390
   * 0040b5fd  xor    esi,esi                                | mov    DWORD PTR [esp+0x18],0x0
   * 0040b5ff  call   0x40b390                               | 
     0040b604  add    esp,0x4                                | add    esp,0x4
     0040b607  test   bp,bp                                  | test   bp,bp
   * 0040b60a  je     0x40b688                               | je     0x40b689
     0040b60c  mov    eax,edi                                | mov    eax,edi
     0040b60e  shl    eax,0x4                                | shl    eax,0x4
     0040b611  add    eax,edi                                | add    eax,edi
     0040b613  lea    ebx,[edi+eax*8]                        | lea    ebx,[edi+eax*8]
     0040b616  add    ebx,0x89                               | add    ebx,0x89
     0040b61c  inc    edi                                    | inc    edi
   * 0040b61d  mov    eax,ds:0xd4a14700                      | mov    eax,ds:0x41cad4
     0040b622  xor    ecx,ecx                                | xor    ecx,ecx
     0040b624  add    eax,ebx                                | add    eax,ebx
     0040b626  xor    edx,edx                                | xor    edx,edx
     0040b628  mov    cx,WORD PTR [eax+0xa]                  | mov    cx,WORD PTR [eax+0xa]
     0040b62c  mov    dx,WORD PTR [eax+0x82]                 | mov    dx,WORD PTR [eax+0x82]
     0040b633  cmp    DWORD PTR [esp+0x18],ecx               | cmp    DWORD PTR [esp+0x18],ecx
   * 0040b637  jne    0x40b683                               | jne    0x40b684
     0040b639  dec    bp                                     | dec    bp
     0040b63b  mov    eax,0x1                                | mov    eax,0x1
     0040b640  inc    WORD PTR [esp+0x12]                    | inc    WORD PTR [esp+0x12]
     0040b645  cmp    ecx,0x3                                | cmp    ecx,0x3
     0040b648  jne    0x40b65f                               | jne    0x40b65f
   * 0040b64a  cmp    DWORD PTR [esp+0x14],0x0               | test   esi,esi
     0040b64f  jne    0x40b65a                               | jne    0x40b65a
     0040b651  mov    eax,0x0                                | mov    eax,0x0
     0040b656  test   edx,edx                                | test   edx,edx
     0040b658  je     0x40b65f                               | je     0x40b65f
     0040b65a  mov    eax,0x1                                | mov    eax,0x1
   * 0040b65f  mov    DWORD PTR [esp+0x14],edx               | mov    esi,edx
     0040b663  test   eax,eax                                | test   eax,eax
   * 0040b665  je     0x40b683                               | je     0x40b684
     0040b667  mov    eax,DWORD PTR [esp+0x12]               | mov    eax,DWORD PTR [esp+0x12]
     0040b66b  push   edi                                    | push   edi
     0040b66c  and    eax,0xffff                             | and    eax,0xffff
     0040b671  mov    ecx,DWORD PTR [esp+0x2c]               | mov    ecx,DWORD PTR [esp+0x2c]
     0040b675  push   eax                                    | push   eax
   * 0040b676  inc    esi                                    | push   0x181
   * 0040b677  push   0x181                                  | push   ecx
   * 0040b67c  push   ecx                                    | call   DWORD PTR ds:0x41f7f4
   * 0040b67d  call   DWORD PTR ds:0x41f7f4                  | inc    DWORD PTR [esp+0x14]
     0040b683  test   bp,bp                                  | test   bp,bp
   * 0040b686  jne    0x40b616                               | jne    0x40b619
   * 0040b688  mov    ecx,DWORD PTR ds:0x1424448b            | mov    ax,WORD PTR [esp+0x14]
   * 0040b68e  mov    eax,DWORD PTR [esp+0x1c]               | mov    edx,DWORD PTR ds:0x41cad4
   * 0040b692  mov    WORD PTR [eax+ecx*1+0x80],si           | mov    ecx,DWORD PTR [esp+0x1c]
   * 0040b699                                                | push   0x9
   * 0040b69a  push   0x9                                    | mov    WORD PTR [ecx+edx*1+0x80],ax
   * 0040b69c  mov    edx,DWORD PTR [esp+0x34]               | 
   * 0040b6a0  mov    ecx,DWORD PTR [esp+0x2c]               | mov    edx,DWORD PTR [esp+0x34]
   * 0040b6a4  push   edx                                    | mov    ecx,DWORD PTR [esp+0x2c]
   * 0040b6a5  push   ecx                                    | push   edx
   * 0040b6a6  push   0x416880                               | push   ecx
   * 0040b6ab  call   0x40c9d0                               | push   0x416880
   * 0040b6b0  mov    ecx,DWORD PTR [esp+0x38]               | call   0x40c9d0
   * 0040b6b4  add    esp,0x10                               | mov    ecx,DWORD PTR [esp+0x38]
   * 0040b6b7  push   0x0                                    | add    esp,0x10
   * 0040b6b9  push   0x1                                    | push   0x0
   * 0040b6bb  push   0xb                                    | push   0x1
   * 0040b6bd  push   ecx                                    | push   0xb
   * 0040b6be  call   DWORD PTR ds:0x41f7f4                  | push   ecx
   * 0040b6c4  mov    ecx,DWORD PTR [esp+0x28]               | call   DWORD PTR ds:0x41f7f4
   * 0040b6c8  push   0x1                                    | mov    ecx,DWORD PTR [esp+0x28]
   * 0040b6ca  push   0x0                                    | push   0x1
   * 0040b6cc  push   ecx                                    | push   0x0
   * 0040b6cd  call   DWORD PTR ds:0x41f6d8                  | push   ecx
   * 0040b6d3  pop    ebp                                    | call   DWORD PTR ds:0x41f6d8
   * 0040b6d4  pop    edi                                    | pop    ebp
   * 0040b6d5  pop    esi                                    | pop    edi
   * 0040b6d6  pop    ebx                                    | pop    esi
   * 0040b6d7  add    esp,0x10                               | pop    ebx
   * 0040b6da  ret                                           | add    esp,0x10
   * 0040b6db  add    eax,0x0                                | ret
```

## What HANDOFF.md already records (most recent last)
- - Session 30: pointer-alias trick tried on SoundInit (SOUNDFX *fx per entry: 10, worse), AddObjectType/AddTrapType (SPRITEREC *f per frame: 21/17 unchanged), WaveTestVolume (dead GetNumDevs forms: 13 unchanged). Scanner (original keeps a constant in a register where ours uses immediates; in try.sh -v output the RIGHT column is the original): hits only in ApplyOptions (esi=1/2/4 zoom), LayoutWindows/OpenLevelItem (esi=1), ShowLevelDetails (ebp=0x156 loop id, edi=&g_infoLsSkills), DrawPreview (edi=0x41cbd4, ecx=-1), Basher::Process (ecx=0x10009008), SoundInit (table base), WaveTestVolume (100). These are loop/induction shapes rather than struct-store aliasing; untested beyond the above.
- - Session 30: handle-local/pointer-alias/block-scope tries on OpenLevelItem (DBENTRY *e alias, dropped idInit: 35 best), DrawPreview (moving mem/old* locals into the if-block, 48 builds: 36), ShowLevelDetails (byte-swap expression forms x DBENTRY *e alias, 24 builds: 37). No gain; levelsel.cpp unchanged at 36/39. Their diffs are register-allocation swaps over many locals (10-13), too many for a full declaration-order search.
- - Session 30: random declaration-order samples (200 each, `build/scratch/s30_sound/randdecl.py`): ShowLevelDetails best 37, DrawPreview best 36 (baseline). OpenLevelItem skipped (local names equal struct field names; the sampler's dependency check needs fixing).
- - Session 30: two-swap header search (`s30_hswap/hswap2.py`, multi-name extern lines split): ScrollTo, DrawLemmings, ApplyOptions, DockWnd::Drag no gain (ReadIniSettings single swaps running). OpenLevelItem random decl orders (sampler fixed for field-name clashes): 35 -> 33 applied (variant 176 of `s30_sound/rd_OpenLevelItem.txt`); levelsel.cpp still 36/39. Backup `build/scratch/s30_sound/levelsel.cpp.before2`.
- - Session 30: OpenLevelItem 33 -> 28 with a second 400-order sample (variant 122 of `rd2_OpenLevelItem.txt`), applied; levelsel.cpp 36/39. ShowLevelDetails and DrawPreview 400-order samples: no gain (37, 36). ReadIniSettings single header swaps (split lines): no gain (3). Next: 600 more OpenLevelItem orders (`rd3_OpenLevelItem.txt`).
- - Session 30: third OpenLevelItem sample (600 orders) and pads 0-40: plateau at 28. EXE 319, make OK.
- - Session 31 (2026-11-01): OpenLevelItem 28 -> 27 via greedy block-permutation (`build/scratch/s30_sound/greedy2.py` + `oli_blocks.py`; only the children--/show/pos block helped), applied; levelsel.cpp 36/39. Total still 669/696. Next: Dock::Layout loop-shape variants scored over StartGame/SaveLayout/LayoutWindows/DockWnd::Drag.
- - **Session 32 padding sweep of the other near-misses** (production body, pad lines 0-40 before the function; `build/scratch/s32/padsweep.log`, `ps/`): ApplyOptions 7, ReadIniSettings 3, DockWnd::DockWnd 4, BuildRows 4, DrawLemmings 7, DrawPane 14, OpenLevelItem 27: no pad count improves them. **Improvements that cost other matches:** DockWnd::Drag 21 -> **6** at 17-18 extra pads (dockwnd.cpp 7 -> 6/9), StartGame 19 -> 10 at 19 pads (mainwnd 24 -> 22/26), ScrollTo 23 -> 18 at 1-2 pads (gameview 33 -> 32/35).
- - **EXE misses left (13):** ApplyOptions 7 (original zeroes edi before the first compare; `!= size`, `register`, extra locals all no), StartGame 19, SaveLayout 27, DrawPane 14, ScrollTo 23, SetViewScale 46, ReadIniSettings 3 (original loads both coordinates into registers before the `g_mapDock` compare; Hopper and Ghidra show nothing more), OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, DockWnd::Drag 17 (compare-orientation flips), Bitmap::BuildRows 4, MapView::DrawLemmings 7. RENDER (3): StretchChanged 3, Renderer_Render 5, CopyChanged 17.
- - Remaining: PROCESS Basher 27, Builder 3, Miner 25, AddObjectType 32 (mid-experiment; was 21), AddTrapType 17, BuildCollision 2, WaveTestVolume 13; EXE ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4; RENDER Renderer_Render, CopyChanged, StretchChanged.
- - **Session 36 end state (verified 2026-11-02):** **WaveTestVolume MATCHES** (src/process/wave.cpp 14/14, `build.py --funcs PROCESS` = **200/206**, `build/scratch/s36/process_wavematch.log`; `.bss` unchanged (46 pre-existing wrong); `make` OK; Wine HD level 1 and digger runs MATCH the baselines, `build/runs/s36-l1`, `s36-dig`). Totals **679/696**: PROCESS 6 misses (Basher 27, Builder 3, Miner 25, AddObjectType 32, AddTrapType 17, BuildCollision 2), LEMMINGS 8 (ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4), RENDER 3 = 17 left. **The fix:** `devs = waveOutGetNumDevs(); switch (n = devs >= 100 ? 100 : devs) {
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33, split-and-climb batch (new tool `tools/msvc/search/splitclimb.py MODULE SOURCE ADDR [--body F] [--pads A-B] [--apply]`: the FillObjects recipe, i.e. every initialised/mid-block declaration becomes a top declaration plus an assignment in place, then a pairwise declaration climb; compiles every variant under the REAL file name in `build/scratch/splitclimb/<func>/<k>/`, whose baseline reproduces try.sh on the real file, unlike variants.py, which gave Miner 32 instead of 25). Results, best (insns, unmatched in file): ScrollTo (short x body) 21 (pads 0-3), 18 at pad 19 but costs a file match; SetViewScale 46; OpenLevelItem 27; ShowLevelDetails 37; DrawPreview 36; AddTrapType 17; Add
-   - `LEMMINGS.EXE`: **329/334** (+3 matches; only 5 functions remain: `ScrollTo` 18, `SetViewScale` 46, `OpenLevelItem` 27, `DrawPreview` 36, `ShowLevelDetails` 37).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
