# ShowLevelDetails hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **LEMMINGS.EXE**, function at **0x40bbe0**, source `src/lemmings/levelsel.cpp`.
> Status: `0x40bbe0 ShowLevelDetails: DIFF (1024 bytes) 250 bytes differ, first at +0x1e2, 37 insns differ; +0x282 ?g_infoLsSkills@@3QAHA: original 0xbf08e0, expected 0x416800; +0x2bc ?g_infoLsSkills@@3QAHA: original 0xfc578900, expected 0x416820`

## Goal
Make `ShowLevelDetails` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh LEMMINGS.EXE src/lemmings/levelsel.cpp -v -f ShowLevelDetails`) to bytes identical to the original, without breaking the other functions in `levelsel.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh LEMMINGS.EXE src/lemmings/levelsel.cpp -v -f ShowLevelDetails
python3 tools/msvc/realname.py LEMMINGS.EXE src/lemmings/levelsel.cpp 0x40bbe0 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x40bbe0 */
void ShowLevelDetails(HWND hDlg, int i, char *file, char *bitmap)
{
    char num[20];
    char path[128];
    char picture[128];
    DBENTRY *rating = FindRating(i);
    HANDLE f;
    int id;
    HWND item;
    int j;
    WORD *p;
    int *s;

    strcpy(path, "level\\");
    strcat(path, rating->file);
    strcat(path, "\\");
    strcpy(picture, "level\\");
    strcat(picture, rating->file);
    strcat(picture, "\\");
    strcat(path, file);
    strcat(picture, bitmap);
    UnrealizeObject(g_hereDlgPalette);
    if (g_newPreview) {
        DeleteObject(g_newPreview);
        DeleteObject(g_previewPalNewUsed);
        g_newPreview = 0;
        g_previewPalNewUsed = 0;
    }
    g_newPreview = LoadDIBitMap(picture, &g_previewPalNewUsed);
    f = FILE_ReadOpen(path);
    FILE_Read(f, g_lvlBuf, 0x800);
    FILE_Close(f);
    g_lastLsCustom = g_db->entries[i].custom;
    if (g_lastLsCustom) {
        g_stateLsRate = g_db->entries[i].rate;
        g_lsLemmingsSpareMain = g_db->entries[i].lemmings;
        g_lsRescueState = g_db->entries[i].rescue;
        g_spareLsTime = g_db->entries[i].time;
        for (j = 0; j < 8; j++)
            g_infoLsSkills[j] = g_db->entries[i].skills[j];
    } else {
        g_lsLemmingsSpareMain = (g_lvlBuf[2] << 8) + g_lvlBuf[3];
        g_lsRescueState = (g_lvlBuf[4] << 8) + g_lvlBuf[5];
        g_spareLsTime = g_lvlBuf[7];
        for (s = g_infoLsSkills, p = (WORD *)(g_lvlBuf + 8); s < g_infoLsSkills + 8; s++, p++)
            *s = HIBYTE(*p) + (LOBYTE(*p) << 8);
    }
    strncpy(g_lsName, g_db->entries[i].name, 49);
    item = GetDlgItem(hDlg, 0x14a);
    InvalidateRect(item, 0, FALSE);
    item = GetDlgItem(hDlg, 0x153);
    itoa(g_lsLemmingsSpareMain, num, 10);
    SetWindowText(item, num);
    InvalidateRect(item, 0, FALSE);
    item = GetDlgItem(hDlg, 0x154);
    itoa(g_lsRescueState, num, 10);
    SetWindowText(item, num);
    InvalidateRect(item, 0, FALSE);
    item = GetDlgItem(hDlg, 0x155);
    wsprintf(num, "%c:00", g_spareLsTime + '0');
    SetWindowText(item, num);
    InvalidateRect(item, 0, FALSE);
    for (id = 0x156; id <= 0x15d; id++) {
        item = GetDlgItem(hDlg, id);
        itoa(g_infoLsSkills[id - 0x156], num, 10);
        SetWindowText(item, num);
        InvalidateRect(item, 0, FALSE);
    }
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     0040bbe0  sub    esp,0x114                              | sub    esp,0x114
     0040bbe6  push   ebx                                    | push   ebx
     0040bbe7  push   esi                                    | push   esi
     0040bbe8  mov    ebx,DWORD PTR [esp+0x124]              | mov    ebx,DWORD PTR [esp+0x124]
     0040bbef  push   edi                                    | push   edi
     0040bbf0  push   ebp                                    | push   ebp
     0040bbf1  mov    edi,0x41cc08                           | mov    edi,0x41cc08
     0040bbf6  push   ebx                                    | push   ebx
     0040bbf7  call   0x40a7d0                               | call   0x40a7d0
     0040bbfc  add    esp,0x4                                | add    esp,0x4
     0040bbff  mov    edx,eax                                | mov    edx,eax
     0040bc01  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bc06  sub    eax,eax                                | sub    eax,eax
     0040bc08  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bc0a  not    ecx                                    | not    ecx
     0040bc0c  sub    edi,ecx                                | sub    edi,ecx
     0040bc0e  mov    eax,ecx                                | mov    eax,ecx
     0040bc10  shr    ecx,0x2                                | shr    ecx,0x2
     0040bc13  mov    esi,edi                                | mov    esi,edi
     0040bc15  lea    edi,[esp+0x24]                         | lea    edi,[esp+0x24]
     0040bc19  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi | rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
     0040bc1b  mov    ecx,eax                                | mov    ecx,eax
     0040bc1d  add    edx,0x32                               | add    edx,0x32
     0040bc20  and    ecx,0x3                                | and    ecx,0x3
     0040bc23  rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]  | rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]
     0040bc25  mov    edi,edx                                | mov    edi,edx
     0040bc27  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bc2c  sub    eax,eax                                | sub    eax,eax
     0040bc2e  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bc30  not    ecx                                    | not    ecx
     0040bc32  sub    edi,ecx                                | sub    edi,ecx
     0040bc34  mov    ebp,ecx                                | mov    ebp,ecx
     0040bc36  mov    esi,edi                                | mov    esi,edi
     0040bc38  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bc3d  lea    edi,[esp+0x24]                         | lea    edi,[esp+0x24]
     0040bc41  sub    eax,eax                                | sub    eax,eax
     0040bc43  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bc45  dec    edi                                    | dec    edi
     0040bc46  mov    ecx,ebp                                | mov    ecx,ebp
     0040bc48  shr    ecx,0x2                                | shr    ecx,0x2
     0040bc4b  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi | rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
     0040bc4d  mov    ecx,ebp                                | mov    ecx,ebp
     0040bc4f  and    ecx,0x3                                | and    ecx,0x3
     0040bc52  rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]  | rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]
     0040bc54  lea    edi,[esp+0x24]                         | lea    edi,[esp+0x24]
     0040bc58  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bc5d  sub    eax,eax                                | sub    eax,eax
     0040bc5f  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bc61  mov    ax,ds:0x41cc10                         | mov    ax,ds:0x41cc10
     0040bc67  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bc6c  mov    WORD PTR [edi-0x1],ax                  | mov    WORD PTR [edi-0x1],ax
     0040bc70  mov    edi,0x41cc14                           | mov    edi,0x41cc14
     0040bc75  sub    eax,eax                                | sub    eax,eax
     0040bc77  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bc79  not    ecx                                    | not    ecx
     0040bc7b  sub    edi,ecx                                | sub    edi,ecx
     0040bc7d  mov    ebp,ecx                                | mov    ebp,ecx
     0040bc7f  shr    ecx,0x2                                | shr    ecx,0x2
     0040bc82  mov    esi,edi                                | mov    esi,edi
     0040bc84  lea    edi,[esp+0xa4]                         | lea    edi,[esp+0xa4]
     0040bc8b  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi | rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
     0040bc8d  mov    ecx,ebp                                | mov    ecx,ebp
     0040bc8f  and    ecx,0x3                                | and    ecx,0x3
     0040bc92  rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]  | rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]
     0040bc94  mov    edi,edx                                | mov    edi,edx
     0040bc96  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bc9b  sub    eax,eax                                | sub    eax,eax
     0040bc9d  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bc9f  not    ecx                                    | not    ecx
     0040bca1  sub    edi,ecx                                | sub    edi,ecx
     0040bca3  mov    edx,ecx                                | mov    edx,ecx
     0040bca5  mov    esi,edi                                | mov    esi,edi
     0040bca7  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bcac  lea    edi,[esp+0xa4]                         | lea    edi,[esp+0xa4]
     0040bcb3  sub    eax,eax                                | sub    eax,eax
     0040bcb5  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bcb7  dec    edi                                    | dec    edi
     0040bcb8  mov    ecx,edx                                | mov    ecx,edx
     0040bcba  shr    ecx,0x2                                | shr    ecx,0x2
     0040bcbd  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi | rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
     0040bcbf  mov    ecx,edx                                | mov    ecx,edx
     0040bcc1  and    ecx,0x3                                | and    ecx,0x3
     0040bcc4  rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]  | rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]
     0040bcc6  lea    edi,[esp+0xa4]                         | lea    edi,[esp+0xa4]
     0040bccd  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bcd2  sub    eax,eax                                | sub    eax,eax
     0040bcd4  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bcd6  mov    ax,ds:0x41cc1c                         | mov    ax,ds:0x41cc1c
     0040bcdc  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bce1  mov    WORD PTR [edi-0x1],ax                  | mov    WORD PTR [edi-0x1],ax
     0040bce5  mov    edi,DWORD PTR [esp+0x130]              | mov    edi,DWORD PTR [esp+0x130]
     0040bcec  sub    eax,eax                                | sub    eax,eax
     0040bcee  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bcf0  not    ecx                                    | not    ecx
     0040bcf2  sub    edi,ecx                                | sub    edi,ecx
     0040bcf4  mov    edx,ecx                                | mov    edx,ecx
     0040bcf6  mov    esi,edi                                | mov    esi,edi
     0040bcf8  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bcfd  lea    edi,[esp+0x24]                         | lea    edi,[esp+0x24]
     0040bd01  sub    eax,eax                                | sub    eax,eax
     0040bd03  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bd05  dec    edi                                    | dec    edi
     0040bd06  mov    ecx,edx                                | mov    ecx,edx
     0040bd08  shr    ecx,0x2                                | shr    ecx,0x2
     0040bd0b  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi | rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
     0040bd0d  mov    ecx,edx                                | mov    ecx,edx
     0040bd0f  and    ecx,0x3                                | and    ecx,0x3
     0040bd12  rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]  | rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]
     0040bd14  mov    edi,DWORD PTR [esp+0x134]              | mov    edi,DWORD PTR [esp+0x134]
     0040bd1b  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bd20  sub    eax,eax                                | sub    eax,eax
     0040bd22  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bd24  not    ecx                                    | not    ecx
     0040bd26  sub    edi,ecx                                | sub    edi,ecx
     0040bd28  mov    edx,ecx                                | mov    edx,ecx
     0040bd2a  mov    esi,edi                                | mov    esi,edi
     0040bd2c  mov    ecx,0xffffffff                         | mov    ecx,0xffffffff
     0040bd31  lea    edi,[esp+0xa4]                         | lea    edi,[esp+0xa4]
     0040bd38  sub    eax,eax                                | sub    eax,eax
     0040bd3a  repnz scas al,BYTE PTR es:[edi]               | repnz scas al,BYTE PTR es:[edi]
     0040bd3c  dec    edi                                    | dec    edi
     0040bd3d  mov    ecx,edx                                | mov    ecx,edx
     0040bd3f  shr    ecx,0x2                                | shr    ecx,0x2
     0040bd42  rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi | rep movs DWORD PTR es:[edi],DWORD PTR ds:[esi]
     0040bd44  mov    ecx,edx                                | mov    ecx,edx
     0040bd46  and    ecx,0x3                                | and    ecx,0x3
     0040bd49  rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]  | rep movs BYTE PTR es:[edi],BYTE PTR ds:[esi]
     0040bd4b  mov    eax,ds:0x41682c                        | mov    eax,ds:0x41682c
     0040bd50  mov    ebp,DWORD PTR ds:0x41f520              | mov    ebp,DWORD PTR ds:0x41f520
     0040bd56  push   eax                                    | push   eax
     0040bd57  xor    esi,esi                                | xor    esi,esi
     0040bd59  call   ebp                                    | call   ebp
     0040bd5b  cmp    DWORD PTR ds:0x416824,esi              | cmp    DWORD PTR ds:0x416824,esi
     0040bd61  je     0x40bd86                               | je     0x40bd86
     0040bd63  mov    eax,ds:0x416824                        | mov    eax,ds:0x416824
     0040bd68  mov    edi,DWORD PTR ds:0x41f570              | mov    edi,DWORD PTR ds:0x41f570
     0040bd6e  push   eax                                    | push   eax
     0040bd6f  call   edi                                    | call   edi
     0040bd71  mov    ecx,DWORD PTR ds:0x4168a8              | mov    ecx,DWORD PTR ds:0x4168a8
     0040bd77  push   ecx                                    | push   ecx
     0040bd78  call   edi                                    | call   edi
     0040bd7a  mov    DWORD PTR ds:0x416824,esi              | mov    DWORD PTR ds:0x416824,esi
     0040bd80  mov    DWORD PTR ds:0x4168a8,esi              | mov    DWORD PTR ds:0x4168a8,esi
     0040bd86  lea    eax,[esp+0xa4]                         | lea    eax,[esp+0xa4]
     0040bd8d  push   0x4168a8                               | push   0x4168a8
     0040bd92  push   eax                                    | push   eax
     0040bd93  call   0x41489e                               | call   0x41489e
     0040bd98  mov    ds:0x416824,eax                        | mov    ds:0x416824,eax
     0040bd9d  lea    eax,[esp+0x24]                         | lea    eax,[esp+0x24]
     0040bda1  push   eax                                    | push   eax
     0040bda2  call   0x41499a                               | call   0x41499a
     0040bda7  mov    esi,eax                                | mov    esi,eax
     0040bda9  push   0x800                                  | push   0x800
     0040bdae  push   0x4168b0                               | push   0x4168b0
     0040bdb3  push   esi                                    | push   esi
     0040bdb4  call   0x41498e                               | call   0x41498e
     0040bdb9  push   esi                                    | push   esi
     0040bdba  call   0x414988                               | call   0x414988
     0040bdbf  mov    eax,ebx                                | mov    eax,ebx
   * 0040bdc1  mov    ecx,DWORD PTR ds:0x41cad4              | mov    esi,DWORD PTR ds:0x41cad4
     0040bdc7  shl    eax,0x4                                | shl    eax,0x4
     0040bdca  add    eax,ebx                                | add    eax,ebx
   * 0040bdcc  lea    esi,[ebx+eax*8]                        | lea    edi,[ebx+eax*8]
     0040bdcf  xor    eax,eax                                | xor    eax,eax
   * 0040bdd1  add    ecx,esi                                | add    esi,edi
   * 0040bdd3  mov    al,BYTE PTR [ecx+0x84]                 | mov    al,BYTE PTR [esi+0x84]
     0040bdd9  mov    ds:0x41686c,eax                        | mov    ds:0x41686c,eax
     0040bdde  test   eax,eax                                | test   eax,eax
     0040bde0  je     0x40be39                               | je     0x40be39
     0040bde2  xor    eax,eax                                | xor    eax,eax
   * 0040bde4  mov    al,BYTE PTR [ecx+0x90]                 | mov    al,BYTE PTR [esi+0x90]
     0040bdea  mov    ds:0x416874,eax                        | mov    ds:0x416874,eax
     0040bdef  xor    eax,eax                                | xor    eax,eax
   * 0040bdf1  mov    al,BYTE PTR [ecx+0x8f]                 | mov    al,BYTE PTR [esi+0x8f]
     0040bdf7  mov    ds:0x416820,eax                        | mov    ds:0x416820,eax
     0040bdfc  xor    eax,eax                                | xor    eax,eax
   * 0040bdfe  mov    al,BYTE PTR [ecx+0x8e]                 | mov    al,BYTE PTR [esi+0x8e]
     0040be04  mov    ds:0x4168ac,eax                        | mov    ds:0x4168ac,eax
     0040be09  xor    eax,eax                                | xor    eax,eax
   * 0040be0b  mov    al,BYTE PTR [ecx+0x8d]                 | mov    al,BYTE PTR [esi+0x8d]
     0040be11  mov    ds:0x416830,eax                        | mov    ds:0x416830,eax
     0040be16  xor    eax,eax                                | xor    eax,eax
   * 0040be18  lea    edi,[esi+eax*1]                        | lea    ebx,[edi+eax*1]
     0040be1b  mov    ecx,DWORD PTR ds:0x41cad4              | mov    ecx,DWORD PTR ds:0x41cad4
     0040be21  xor    edx,edx                                | xor    edx,edx
     0040be23  inc    eax                                    | inc    eax
   * 0040be24  mov    dl,BYTE PTR [edi+ecx*1+0x85]           | mov    dl,BYTE PTR [ebx+ecx*1+0x85]
     0040be2b  cmp    eax,0x8                                | cmp    eax,0x8
     0040be2e  mov    DWORD PTR [eax*4+0x4167fc],edx         | mov    DWORD PTR [eax*4+0x4167fc],edx
     0040be35  jl     0x40be18                               | jl     0x40be18
     0040be37  jmp    0x40bea2                               | jmp    0x40bea2
     0040be39  xor    eax,eax                                | xor    eax,eax
     0040be3b  xor    ecx,ecx                                | xor    ecx,ecx
     0040be3d  mov    al,ds:0x4168b2                         | mov    al,ds:0x4168b2
     0040be42  mov    cl,BYTE PTR ds:0x4168b3                | mov    cl,BYTE PTR ds:0x4168b3
     0040be48  shl    eax,0x8                                | shl    eax,0x8
     0040be4b  add    eax,ecx                                | add    eax,ecx
     0040be4d  xor    ecx,ecx                                | xor    ecx,ecx
     0040be4f  mov    cl,BYTE PTR ds:0x4168b5                | mov    cl,BYTE PTR ds:0x4168b5
     0040be55  mov    ds:0x416820,eax                        | mov    ds:0x416820,eax
     0040be5a  xor    eax,eax                                | xor    eax,eax
     0040be5c  mov    al,ds:0x4168b4                         | mov    al,ds:0x4168b4
   * 0040be61  mov    edi,0x416800                           | shl    eax,0x8
   * 0040be66  shl    eax,0x8                                | mov    edi,0x416800
     0040be69  add    eax,ecx                                | add    eax,ecx
     0040be6b  mov    ds:0x4168ac,eax                        | mov    ds:0x4168ac,eax
     0040be70  xor    eax,eax                                | xor    eax,eax
     0040be72  mov    al,ds:0x4168b7                         | mov    al,ds:0x4168b7
     0040be77  mov    ds:0x416830,eax                        | mov    ds:0x416830,eax
     0040be7c  mov    eax,0x4168b8                           | mov    eax,0x4168b8
     0040be81  mov    cx,WORD PTR [eax]                      | mov    cx,WORD PTR [eax]
     0040be84  xor    edx,edx                                | xor    edx,edx
   * 0040be86  mov    dl,cl                                  | mov    dl,ch
     0040be88  xor    ebx,ebx                                | xor    ebx,ebx
   * 0040be8a  shl    edx,0x8                                | mov    bl,cl
   * 0040be8d  mov    bl,ch                                  | add    edi,0x4
   * 0040be8f  add    edx,ebx                                | shl    ebx,0x8
   * 0040be91  add    edi,0x4                                | add    eax,0x2
   * 0040be94  add    eax,0x2                                | add    edx,ebx
   * 0040be97  mov    DWORD PTR [edi-0x4],edx                | cmp    edi,0x416820
   * 0040be9a  cmp    edi,0x416820                           | mov    DWORD PTR [edi-0x4],edx
     0040bea0  jb     0x40be81                               | jb     0x40be81
     0040bea2  push   0x31                                   | push   0x31
   * 0040bea4  add    esi,DWORD PTR ds:0x38685612            | add    esi,0x12
   * 0040beaa  add    esi,0x12                               | push   esi
   * 0040bead  push   esi                                    | push   0x416838
   * 0040beae  push   0x416838                               | call   0x411f8c
   * 0040beb3  call   0x41f2dc                               | mov    edi,DWORD PTR [esp+0x134]
   * 0040beb8  mov    esi,DWORD PTR [esp+0x134]              | add    esp,0xc
   * 0040bebf  add    esp,0xc                                | push   0x14a
   * 0040bec2  push   0x14a                                  | push   edi
   * 0040bec7  push   esi                                    | call   DWORD PTR ds:0x41f7c8
   * 0040bec8  call   DWORD PTR ds:0x41f7c8                  | push   0x0
   * 0040bece  push   0x0                                    | mov    esi,DWORD PTR ds:0x41f6d8
   * 0040bed0  mov    edi,DWORD PTR ds:0x41f6d8              | push   0x0
   * 0040bed6  push   0x0                                    | push   eax
   * 0040bed8  push   eax                                    | call   esi
   * 0040bed9  call   edi                                    | push   0x153
   * 0040bedb  push   0x153                                  | push   edi
   * 0040bee0  push   esi                                    | call   DWORD PTR ds:0x41f7c8
   * 0040bee1  call   DWORD PTR ds:0x41f7c8                  | mov    ebx,eax
   * 0040bee7  mov    ebx,eax                                | push   0xa
   * 0040bee9  push   0xa                                    | lea    eax,[esp+0x14]
   * 0040beeb  lea    eax,[esp+0x14]                         | mov    ecx,DWORD PTR ds:0x416820
   * 0040beef  mov    ecx,DWORD PTR ds:0x416820              | push   eax
   * 0040bef5  push   eax                                    | push   ecx
   * 0040bef6  push   ecx                                    | call   0x414a00
   * 0040bef7  call   0x414a00                               | lea    ecx,[esp+0x1c]
   * 0040befc  lea    ecx,[esp+0x1c]                         | add    esp,0xc
   * 0040bf00  add    esp,0xc                                | push   ecx
   * 0040bf03  push   ecx                                    | push   ebx
   * 0040bf04  push   ebx                                    | call   DWORD PTR ds:0x41f6e4
   * 0040bf05  call   DWORD PTR ds:0x41f6e4                  | push   0x0
     0040bf0b  push   0x0                                    | push   0x0
   * 0040bf0d  push   0x0                                    | push   ebx
   * 0040bf0f  push   ebx                                    | call   esi
   * 0040bf10  call   edi                                    | push   0x154
   * 0040bf12  push   0x154                                  | push   edi
   * 0040bf17  push   esi                                    | call   DWORD PTR ds:0x41f7c8
   * 0040bf18  call   DWORD PTR ds:0x41f7c8                  | mov    ebx,eax
   * 0040bf1e  mov    ebx,eax                                | push   0xa
   * 0040bf20  push   0xa                                    | lea    ecx,[esp+0x14]
   * 0040bf22  lea    ecx,[esp+0x14]                         | mov    eax,ds:0x4168ac
   * 0040bf26  mov    eax,ds:0x4168ac                        | push   ecx
   * 0040bf2b  push   ecx                                    | push   eax
   * 0040bf2c  push   eax                                    | call   0x414a00
   * 0040bf2d  call   0x414a00                               | lea    ecx,[esp+0x1c]
   * 0040bf32  lea    ecx,[esp+0x1c]                         | add    esp,0xc
   * 0040bf36  add    esp,0xc                                | push   ecx
   * 0040bf39  push   ecx                                    | push   ebx
   * 0040bf3a  push   ebx                                    | call   DWORD PTR ds:0x41f6e4
   * 0040bf3b  call   DWORD PTR ds:0x41f6e4                  | push   0x0
     0040bf41  push   0x0                                    | push   0x0
   * 0040bf43  push   0x0                                    | push   ebx
   * 0040bf45  push   ebx                                    | call   esi
   * 0040bf46  call   edi                                    | push   0x155
   * 0040bf48  push   0x155                                  | push   edi
   * 0040bf4d  push   esi                                    | call   DWORD PTR ds:0x41f7c8
   * 0040bf4e  call   DWORD PTR ds:0x41f7c8                  | mov    ebx,eax
   * 0040bf54  mov    ebx,eax                                | mov    eax,ds:0x416830
   * 0040bf56  mov    eax,ds:0x416830                        | add    eax,0x30
   * 0040bf5b  add    eax,0x30                               | push   eax
   * 0040bf5e  push   eax                                    | push   0x41cc20
   * 0040bf5f  lea    ecx,[esp+0x14]                         | lea    ecx,[esp+0x18]
   * 0040bf63  push   0x0                                    | mov    ebp,DWORD PTR ds:0x41f778
     0040bf68  push   ecx                                    | push   ecx
   * 0040bf69  mov    ebp,DWORD PTR ds:0x41f778              | call   ebp
   * 0040bf6f  call   ebp                                    | lea    ecx,[esp+0x1c]
   * 0040bf71  lea    ecx,[esp+0x1c]                         | add    esp,0xc
   * 0040bf75  add    esp,0xc                                | push   ecx
   * 0040bf78  push   ecx                                    | push   ebx
   * 0040bf79  push   ebx                                    | mov    ebp,0x156
   * 0040bf7a  mov    ebp,0x156                              | call   DWORD PTR ds:0x41f6e4
   * 0040bf7f  call   DWORD PTR ds:0x41f6e4                  | push   0x0
     0040bf85  push   0x0                                    | push   0x0
   * 0040bf87  push   0x0                                    | push   ebx
   * 0040bf89  push   ebx                                    | call   esi
   * 0040bf8a  call   edi                                    | push   ebp
   * 0040bf8c  push   ebp                                    | push   edi
   * 0040bf8d  push   esi                                    | inc    ebp
   * 0040bf8e  inc    ebp                                    | call   DWORD PTR ds:0x41f7c8
   * 0040bf8f  call   DWORD PTR ds:0x41f7c8                  | mov    ebx,eax
   * 0040bf95  mov    ebx,eax                                | push   0xa
   * 0040bf97  push   0xa                                    | lea    eax,[esp+0x14]
   * 0040bf99  lea    eax,[esp+0x14]                         | mov    ecx,DWORD PTR [ebp*4+0x4162a4]
   * 0040bf9d  mov    ecx,DWORD PTR [ebp*4+0x4162a4]         | push   eax
   * 0040bfa4  push   eax                                    | push   ecx
   * 0040bfa5  push   ecx                                    | call   0x414a00
   * 0040bfa6  call   0x414a00                               | lea    ecx,[esp+0x1c]
   * 0040bfab  lea    ecx,[esp+0x1c]                         | add    esp,0xc
   * 0040bfaf  add    esp,0xc                                | push   ecx
   * 0040bfb2  push   ecx                                    | push   ebx
   * 0040bfb3  push   ebx                                    | call   DWORD PTR ds:0x41f6e4
   * 0040bfb4  call   DWORD PTR ds:0x41f6e4                  | push   0x0
     0040bfba  push   0x0                                    | push   0x0
   * 0040bfbc  push   0x0                                    | push   ebx
   * 0040bfbe  push   ebx                                    | call   esi
   * 0040bfbf  call   edi                                    | cmp    ebp,0x15d
   * 0040bfc1  cmp    ebp,0x15d                              | jle    0x40bf86
   * 0040bfc7  jle    0x40bf8c                               | pop    ebp
   * 0040bfc9  pop    ebp                                    | pop    edi
   * 0040bfca  pop    edi                                    | pop    esi
   * 0040bfcb  pop    esi                                    | pop    ebx
   * 0040bfcc  pop    ebx                                    | add    esp,0x114
   * 0040bfcd  add    esp,0x114                              | ret
   * 0040bfd3  ret                                           | mov    edi,edi
   * 0040bfd4  add    eax,0x0                                | ret
     0040bfd9  lea    esp,[esp+0x0]                          | lea    esp,[esp+0x0]
   * 0040bfd5                                                | lea    esp,[esp+0x0]
   * 0040bfd9                                                | lea    esp,[esp+0x0]
```

## What HANDOFF.md already records (most recent last)
- - `build/scratch/s42_rev/batch_lt.sh` ran `lifetimes.py` (pads 0-2) on every EXE miss; results `build/scratch/s42_rev/batch/<file>_<addr>/results.json`, log `batch.log`. **Wins applied:** **OnViewCreate MATCH** (gameview.cpp 30/35: `g_lastCursor = 0;` moved one statement later), **StartEndScreen MATCH** (above), **ApplyOptions 23 → 2** (options.cpp: `SetZoom(...)` before `g_resolutionNew = size;`; left: the original stores the resolution *before* `cmp esi,8` (between `mov eax,4` and the compare) and compares `g_zoom` as `cmp [g_zoom],esi`; 12 spellings tried, still 2), **ShowLevelDetails 57 → 37** (levelsel.cpp: `HANDLE f;` declared before `HWND item;`; remaining diff: the original keeps the
- - **FillLevelList MATCH (session 28, DONE and verified: levelsel.cpp 34/39, full link 298/334 (`exe_after3.log`, the only change to the miss list is FillLevelList), `make` OK, Wine HD level 1 (`s28b-l1`) and digger (`s28b-dig`) MATCH; re-plan was `padplan --start 0x40b100`, 4 lines):** a declaration-order × pad search found `int i; HWND list = GetDlgItem(hDlg, 0x11d); int id;` + **10 `pad_fillevellist` declarations** before the function (source shape unchanged; only the order and the padding). Applied to `src/lemmings/levelsel.cpp` (backup `build/scratch/s28_exe/levelsel.cpp.before`); the 10 symbols shifted the rest of the file (DrawIconText, NextLevelNumber lost their matches, ShowLevelDeta
- - Next: larger structural misses (DrawPreview 41, ShowLevelDetails 61, OpenLevelItem 57, MapView::DrawLemmings 36, IconWnd::WndProc 37, SetViewScale 46, StartGame 58) with REA pseudocode first, since the small ones are register ties already searched.
- - Session 30: pointer-alias trick tried on SoundInit (SOUNDFX *fx per entry: 10, worse), AddObjectType/AddTrapType (SPRITEREC *f per frame: 21/17 unchanged), WaveTestVolume (dead GetNumDevs forms: 13 unchanged). Scanner (original keeps a constant in a register where ours uses immediates; in try.sh -v output the RIGHT column is the original): hits only in ApplyOptions (esi=1/2/4 zoom), LayoutWindows/OpenLevelItem (esi=1), ShowLevelDetails (ebp=0x156 loop id, edi=&g_infoLsSkills), DrawPreview (edi=0x41cbd4, ecx=-1), Basher::Process (ecx=0x10009008), SoundInit (table base), WaveTestVolume (100). These are loop/induction shapes rather than struct-store aliasing; untested beyond the above.
- - Session 30: handle-local/pointer-alias/block-scope tries on OpenLevelItem (DBENTRY *e alias, dropped idInit: 35 best), DrawPreview (moving mem/old* locals into the if-block, 48 builds: 36), ShowLevelDetails (byte-swap expression forms x DBENTRY *e alias, 24 builds: 37). No gain; levelsel.cpp unchanged at 36/39. Their diffs are register-allocation swaps over many locals (10-13), too many for a full declaration-order search.
- - Session 30: random declaration-order samples (200 each, `build/scratch/s30_sound/randdecl.py`): ShowLevelDetails best 37, DrawPreview best 36 (baseline). OpenLevelItem skipped (local names equal struct field names; the sampler's dependency check needs fixing).
- - Session 30: OpenLevelItem 33 -> 28 with a second 400-order sample (variant 122 of `rd2_OpenLevelItem.txt`), applied; levelsel.cpp 36/39. ShowLevelDetails and DrawPreview 400-order samples: no gain (37, 36). ReadIniSettings single header swaps (split lines): no gain (3). Next: 600 more OpenLevelItem orders (`rd3_OpenLevelItem.txt`).
- - **EXE misses left (13):** ApplyOptions 7 (original zeroes edi before the first compare; `!= size`, `register`, extra locals all no), StartGame 19, SaveLayout 27, DrawPane 14, ScrollTo 23, SetViewScale 46, ReadIniSettings 3 (original loads both coordinates into registers before the `g_mapDock` compare; Hopper and Ghidra show nothing more), OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, DockWnd::Drag 17 (compare-orientation flips), Bitmap::BuildRows 4, MapView::DrawLemmings 7. RENDER (3): StretchChanged 3, Renderer_Render 5, CopyChanged 17.
- - Remaining: PROCESS Basher 27, Builder 3, Miner 25, AddObjectType 32 (mid-experiment; was 21), AddTrapType 17, BuildCollision 2, WaveTestVolume 13; EXE ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4; RENDER Renderer_Render, CopyChanged, StretchChanged.
- - **Session 36 end state (verified 2026-11-02):** **WaveTestVolume MATCHES** (src/process/wave.cpp 14/14, `build.py --funcs PROCESS` = **200/206**, `build/scratch/s36/process_wavematch.log`; `.bss` unchanged (46 pre-existing wrong); `make` OK; Wine HD level 1 and digger runs MATCH the baselines, `build/runs/s36-l1`, `s36-dig`). Totals **679/696**: PROCESS 6 misses (Basher 27, Builder 3, Miner 25, AddObjectType 32, AddTrapType 17, BuildCollision 2), LEMMINGS 8 (ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4), RENDER 3 = 17 left. **The fix:** `devs = waveOutGetNumDevs(); switch (n = devs >= 100 ? 100 : devs) {
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33 declaration climbs (`tools/msvc/search/declclimb.py`, pairwise swaps + move-to-front, pads 0-3/0-6; logs `build/scratch/s33_bp/*_climb.log`): no new matches. Builder::Process 3, DrawPreview 36, DockWnd::Drag 17, ScrollTo 22 (from 23, pad 0, not applied), SetViewScale 45 (from 46, pad 2, not applied), ShowLevelDetails 37, Miner::Process 25, Basher::Process 27, Map::AddTrapType 17. None of these is a declaration-order tie; each needs a source-shape change. WaveTestVolume skipped: the original keeps the clamped volume in esi though it is never used again (a dead value in a callee-saved register), which no declaration order explains.
- - Session 33, split-and-climb batch (new tool `tools/msvc/search/splitclimb.py MODULE SOURCE ADDR [--body F] [--pads A-B] [--apply]`: the FillObjects recipe, i.e. every initialised/mid-block declaration becomes a top declaration plus an assignment in place, then a pairwise declaration climb; compiles every variant under the REAL file name in `build/scratch/splitclimb/<func>/<k>/`, whose baseline reproduces try.sh on the real file, unlike variants.py, which gave Miner 32 instead of 25). Results, best (insns, unmatched in file): ScrollTo (short x body) 21 (pads 0-3), 18 at pad 19 but costs a file match; SetViewScale 46; OpenLevelItem 27; ShowLevelDetails 37; DrawPreview 36; AddTrapType 17; Add
-   - `LEMMINGS.EXE`: **329/334** (+3 matches; only 5 functions remain: `ScrollTo` 18, `SetViewScale` 46, `OpenLevelItem` 27, `DrawPreview` 36, `ShowLevelDetails` 37).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
