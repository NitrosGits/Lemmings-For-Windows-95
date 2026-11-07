# Basher::Process hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **PROCESS.DLL**, function at **0x10001040**, source `src/process/basher.cpp`.
> Status: `0x10001040 Basher::Process: DIFF (512 bytes) 57 bytes differ, first at +0x44, 27 insns differ; +0x6b ?g_sprites@@3PAEA: original 0x14e08, expected 0x10009014`

## Goal
Make `Basher::Process` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh PROCESS.DLL src/process/basher.cpp -v -f Basher::Process`) to bytes identical to the original, without breaking the other functions in `basher.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh PROCESS.DLL src/process/basher.cpp -v -f Basher::Process
python3 tools/msvc/realname.py PROCESS.DLL src/process/basher.cpp 0x10001040 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x10001040 */
void Basher::Process(LEMMING *l, DLENTRY **list)
{
    int f;
    int x, y;
    PNT at;

    if ((f = ++l->frame) >= l->frames)
        l->frame = l->loop - l->frames + f;
    switch (l->frame) {
    case 2: case 3: case 4: case 5:
    case 18: case 19: case 20: case 21:
        x = l->xoff + l->x;
        y = l->y + l->yoff - 6;
        g_map->EraseSprite(list, g_sprites, (l->dir == -1 ? 4 : 0) + (l->frame & 15) + SPR_DIGHOLE, x, y);
        x = (l->x & ~3) + l->dir * 8;
        y = (l->y & ~3) - 8;
        if (CantBash(l, x, y)) {
            at.x = l->x;
            at.y = l->y + l->yoff;
            SoundPlayFX(at, 6);
            Leave(l);
            g_walker.Enter(l);
            return;
        }
        if (l->frame == 5) {
            x = l->x + l->dir * 8;
            y = l->y - 6;
            if (!g_map->GetPixel(x, y) && !g_map->GetPixel(x += l->dir, y) && !g_map->GetPixel(x += l->dir, y)) {
                Leave(l);
                g_walker.Enter(l);
                return;
            }
        }
        break;
    case 7: case 9: case 17: case 23: case 25:
        break;
    case 11: case 12: case 13: case 14: case 15:
    case 27: case 28: case 29: case 30: case 31:
        x = l->x += l->dir;
        y = l->y;
        if (!g_map->GetPixel(x, y) && !g_map->GetPixel(x, ++y) && !g_map->GetPixel(x, ++y))
            Message(l, MSG_FALL, 0, 0);
        break;
    }
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     10001040  sub    esp,0x8                                | sub    esp,0x8
     10001043  push   ebx                                    | push   ebx
     10001044  push   esi                                    | push   esi
     10001045  mov    esi,DWORD PTR [esp+0x14]               | mov    esi,DWORD PTR [esp+0x14]
     10001049  push   edi                                    | push   edi
     1000104a  push   ebp                                    | push   ebp
     1000104b  mov    edi,ecx                                | mov    edi,ecx
     1000104d  mov    eax,DWORD PTR [esi+0x14]               | mov    eax,DWORD PTR [esi+0x14]
     10001050  mov    ecx,DWORD PTR [esi+0x1c]               | mov    ecx,DWORD PTR [esi+0x1c]
     10001053  inc    eax                                    | inc    eax
     10001054  mov    DWORD PTR [esi+0x14],eax               | mov    DWORD PTR [esi+0x14],eax
     10001057  cmp    ecx,eax                                | cmp    ecx,eax
     10001059  jg     0x10001065                             | jg     0x10001065
     1000105b  mov    edx,DWORD PTR [esi+0x20]               | mov    edx,DWORD PTR [esi+0x20]
     1000105e  sub    edx,ecx                                | sub    edx,ecx
     10001060  add    edx,eax                                | add    edx,eax
     10001062  mov    DWORD PTR [esi+0x14],edx               | mov    DWORD PTR [esi+0x14],edx
     10001065  mov    ecx,DWORD PTR [esi+0x14]               | mov    ecx,DWORD PTR [esi+0x14]
     10001068  lea    edx,[ecx-0x2]                          | lea    edx,[ecx-0x2]
     1000106b  cmp    edx,0x1d                               | cmp    edx,0x1d
     1000106e  ja     0x100011f9                             | ja     0x100011f9
     10001074  xor    eax,eax                                | xor    eax,eax
     10001076  mov    al,BYTE PTR [edx+0x1000122c]           | mov    al,BYTE PTR [edx+0x1000122c]
     1000107c  jmp    DWORD PTR [eax*4+0x10001204]           | jmp    DWORD PTR [eax*4+0x10001204]
   * 10001083  mov    ebx,DWORD PTR [esi+0x8]                | mov    edx,DWORD PTR [esi+0x8]
     10001086  mov    eax,DWORD PTR [esi+0x4]                | mov    eax,DWORD PTR [esi+0x4]
   * 10001089  add    ebx,DWORD PTR [esi]                    | add    edx,DWORD PTR [esi]
   * 1000108b  mov    edx,DWORD PTR [esi+0xc]                | mov    ebx,DWORD PTR [esi+0xc]
   * 1000108e  lea    ebp,[eax+edx*1-0x6]                    | lea    ebp,[eax+ebx*1-0x6]
     10001092  mov    eax,DWORD PTR [esi+0x10]               | mov    eax,DWORD PTR [esi+0x10]
   * 10001095  push   ebp                                    | inc    eax
   * 10001096  inc    eax                                    | push   ebp
   * 10001097  push   ebx                                    | push   edx
     10001098  cmp    eax,0x1                                | cmp    eax,0x1
     1000109b  sbb    eax,eax                                | sbb    eax,eax
   * 1000109d  and    ecx,0xf                                | mov    edx,DWORD PTR ds:0x10009014
     100010a0  and    eax,0x4                                | and    eax,0x4
   * 100010a3  mov    edx,DWORD PTR [esp+0x28]               | and    ecx,0xf
   * 100010a7  add    eax,ecx                                | lea    eax,[eax+ecx*1+0x14e]
   * 100010a9  mov    ecx,DWORD PTR ds:0x10009014            | mov    ecx,DWORD PTR [esp+0x28]
   * 100010af  add    eax,0x14e                              | push   eax
   * 100010b4  push   eax                                    | push   edx
     100010b5  push   ecx                                    | push   ecx
   * 100010b6  push   edx                                    | mov    ecx,DWORD PTR ds:0x10009288
   * 100010b7  mov    ecx,DWORD PTR ds:0x10009288            | call   0x10004e10
   * 100010bd  call   0x10004e10                             | mov    edx,DWORD PTR [esi+0x10]
   * 100010c2  mov    edx,DWORD PTR [esi+0x10]               | mov    ecx,DWORD PTR [esi]
   * 100010c5  mov    ecx,DWORD PTR [esi]                    | and    ecx,0xfffffffc
   * 100010c7  and    ecx,0xfffffffc                         | lea    eax,[ecx+edx*8]
   * 100010ca  lea    eax,[ecx+edx*8]                        | mov    ecx,DWORD PTR [esi+0x4]
   * 100010cd  mov    ecx,DWORD PTR [esi+0x4]                | and    ecx,0xfffffffc
   * 100010d0  and    ecx,0xfffffffc                         | sub    ecx,0x8
   * 100010d3  sub    ecx,0x8                                | push   ecx
   * 100010d6  push   ecx                                    | push   eax
   * 100010d7  push   eax                                    | push   esi
   * 100010d8  push   esi                                    | call   0x10001000
   * 100010d9  call   0x10001000                             | add    esp,0xc
   * 100010de  add    esp,0xc                                | test   eax,eax
   * 100010e1  test   eax,eax                                | je     0x1000111b
   * 100010e3  je     0x1000111b                             | mov    eax,DWORD PTR [esi]
   * 100010e5  mov    eax,DWORD PTR [esi]                    | push   0x6
   * 100010e7  push   0x6                                    | mov    DWORD PTR [esp+0x14],eax
   * 100010e9  mov    DWORD PTR [esp+0x14],eax               | mov    eax,DWORD PTR [esi+0x4]
   * 100010ed  mov    eax,DWORD PTR [esi+0x4]                | add    eax,DWORD PTR [esi+0xc]
   * 100010f0  add    eax,DWORD PTR [esi+0xc]                | mov    edx,DWORD PTR [esp+0x14]
   * 100010f3  mov    edx,DWORD PTR [esp+0x14]               | push   eax
   * 100010f7  push   eax                                    | push   edx
   * 100010f8  push   edx                                    | call   0x10005e30
   * 100010f9  call   0x10005e30                             | push   esi
   * 100010fe  push   esi                                    | mov    ebx,DWORD PTR [edi]
   * 100010ff  mov    ebx,DWORD PTR [edi]                    | mov    ecx,edi
   * 10001101  mov    ecx,edi                                | call   DWORD PTR [ebx+0x8]
   * 10001103  call   DWORD PTR [ebx+0x8]                    | push   esi
   * 10001106  push   esi                                    | mov    ecx,0x10009008
   * 10001107  mov    ecx,0x10009008                         | call   0x10004630
   * 1000110c  call   0x10004630                             | pop    ebp
   * 10001111  pop    ebp                                    | pop    edi
   * 10001112  pop    edi                                    | pop    esi
   * 10001113  pop    esi                                    | pop    ebx
   * 10001114  pop    ebx                                    | add    esp,0x8
   * 10001115  add    esp,0x8                                | ret    0x8
   * 10001118  ret    0x8                                    | cmp    DWORD PTR [esi+0x14],0x5
   * 1000111b  cmp    DWORD PTR [esi+0x14],0x5               | jne    0x100011f9
   * 1000111f  jne    0x100011f9                             | mov    ebx,DWORD PTR [esi+0x10]
   * 10001125  mov    ebx,DWORD PTR [esi+0x10]               | mov    ebp,DWORD PTR [esi+0x4]
   * 10001128  mov    ebp,DWORD PTR [esi+0x4]                | shl    ebx,0x3
   * 1000112b  shl    ebx,0x3                                | sub    ebp,0x6
   * 1000112e  sub    ebp,0x6                                | add    ebx,DWORD PTR [esi]
   * 10001131  add    ebx,DWORD PTR [esi]                    | push   ebp
   * 10001133  push   ebp                                    | push   ebx
   * 10001134  push   ebx                                    | mov    ecx,DWORD PTR ds:0x10009288
   * 10001135  mov    ecx,DWORD PTR ds:0x10009288            | call   0x100054c0
   * 1000113b  call   0x100054c0                             | test   al,al
   * 10001140  test   al,al                                  | jne    0x100011f9
   * 10001142  jne    0x100011f9                             | add    ebx,DWORD PTR [esi+0x10]
   * 10001148  add    ebx,DWORD PTR [esi+0x10]               | push   ebp
   * 1000114b  push   ebp                                    | push   ebx
   * 1000114c  push   ebx                                    | mov    ecx,DWORD PTR ds:0x10009288
   * 1000114d  mov    ecx,DWORD PTR ds:0x10009288            | call   0x100054c0
   * 10001153  call   0x100054c0                             | test   al,al
   * 10001158  test   al,al                                  | jne    0x100011f9
   * 1000115a  jne    0x100011f9                             | add    ebx,DWORD PTR [esi+0x10]
   * 10001160  add    ebx,DWORD PTR [esi+0x10]               | push   ebp
   * 10001163  push   ebp                                    | push   ebx
   * 10001164  push   ebx                                    | mov    ecx,DWORD PTR ds:0x10009288
   * 10001165  mov    ecx,DWORD PTR ds:0x10009288            | call   0x100054c0
   * 1000116b  call   0x100054c0                             | test   al,al
   * 10001170  test   al,al                                  | jne    0x100011f9
   * 10001172  jne    0x100011f9                             | push   esi
   * 10001178  push   esi                                    | mov    eax,DWORD PTR [edi]
   * 10001179  mov    eax,DWORD PTR [edi]                    | mov    ecx,edi
   * 1000117b  mov    ecx,edi                                | call   DWORD PTR [eax+0x8]
   * 1000117d  call   DWORD PTR [eax+0x8]                    | push   esi
   * 10001180  push   esi                                    | mov    ecx,0x10009008
   * 10001181  mov    ecx,0x10009008                         | call   0x10004630
   * 10001186  call   0x10004630                             | pop    ebp
   * 1000118b  pop    ebp                                    | pop    edi
   * 1000118c  pop    edi                                    | pop    esi
   * 1000118d  pop    esi                                    | pop    ebx
   * 1000118e  pop    ebx                                    | add    esp,0x8
   * 1000118f  add    esp,0x8                                | ret    0x8
   * 10001192  ret    0x8                                    | mov    ebx,DWORD PTR [esi+0x10]
   * 10001195  mov    ebx,DWORD PTR [esi+0x10]               | mov    edi,DWORD PTR [esi+0x4]
   * 10001198  mov    edi,DWORD PTR [esi+0x4]                | add    ebx,DWORD PTR [esi]
   * 1000119b  add    ebx,DWORD PTR [esi]                    | push   edi
   * 1000119d  push   edi                                    | push   ebx
   * 1000119e  push   ebx                                    | mov    DWORD PTR [esi],ebx
   * 1000119f  mov    DWORD PTR [esi],ebx                    | mov    ecx,DWORD PTR ds:0x10009288
   * 100011a1  mov    ecx,DWORD PTR ds:0x10009288            | call   0x100054c0
   * 100011a7  call   0x100054c0                             | test   al,al
   * 100011ac  test   al,al                                  | jne    0x100011f9
   * 100011ae  jne    0x100011f9                             | inc    edi
   * 100011b0  inc    edi                                    | mov    ecx,DWORD PTR ds:0x10009288
   * 100011b1  mov    ecx,DWORD PTR ds:0x10009288            | push   edi
   * 100011b7  push   edi                                    | push   ebx
   * 100011b8  push   ebx                                    | call   0x100054c0
   * 100011b9  call   0x100054c0                             | test   al,al
   * 100011be  test   al,al                                  | jne    0x100011f9
   * 100011c0  jne    0x100011f9                             | inc    edi
   * 100011c2  inc    edi                                    | mov    ecx,DWORD PTR ds:0x10009288
   * 100011c3  mov    ecx,DWORD PTR ds:0x10009288            | push   edi
   * 100011c9  push   edi                                    | push   ebx
   * 100011ca  push   ebx                                    | call   0x100054c0
   * 100011cb  call   0x100054c0                             | test   al,al
   * 100011d0  test   al,al                                  | jne    0x100011f9
   * 100011d2  jne    0x100011f9                             | mov    edi,DWORD PTR [esi+0x64]
   * 100011d4  mov    edi,DWORD PTR [esi+0x64]               | test   edi,edi
   * 100011d7  test   edi,edi                                | je     0x100011f9
   * 100011d9  je     0x100011f9                             | push   esi
   * 100011db  push   esi                                    | push   0x0
     100011dc  push   0x0                                    | push   0x0
   * 100011de  push   0x0                                    | push   0x101
   * 100011e0  push   0x101                                  | call   DWORD PTR [edi]
   * 100011e5  call   DWORD PTR [edi]                        | add    esp,0x10
   * 100011e7  add    esp,0x10                               | test   eax,eax
   * 100011ea  test   eax,eax                                | je     0x100011f2
   * 100011ec  je     0x100011f2                             | xor    edi,edi
   * 100011ee  xor    edi,edi                                | jmp    0x100011f5
   * 100011f0  jmp    0x100011f5                             | mov    edi,DWORD PTR [edi+0x4]
   * 100011f2  mov    edi,DWORD PTR [edi+0x4]                | test   edi,edi
   * 100011f5  test   edi,edi                                | jne    0x100011db
   * 100011f7  jne    0x100011db                             | pop    ebp
   * 100011f9  pop    ebp                                    | pop    edi
   * 100011fa  pop    edi                                    | pop    esi
   * 100011fb  pop    esi                                    | pop    ebx
   * 100011fc  pop    ebx                                    | add    esp,0x8
   * 100011fd  add    esp,0x8                                | ret    0x8
   * 10001200  ret    0x8                                    | nop
   * 10001203  nop                                           | adc    DWORD PTR [eax],0x0
   * 10001204  adc    DWORD PTR [eax],0x0                    | adc    cl,bh
   * 10001207  adc    cl,bh                                  | adc    DWORD PTR [eax],eax
   * 10001209  adc    DWORD PTR [eax],eax                    | adc    cl,bh
   * 1000120b  adc    cl,bh                                  | adc    DWORD PTR [eax],eax
   * 1000120d  adc    DWORD PTR [eax],eax                    | adc    BYTE PTR [ebp-0x6efffef],dl
   * 1000120f  adc    BYTE PTR [ebp-0x6efffef],dl            | adc    DWORD PTR [eax],eax
   * 10001215  adc    DWORD PTR [eax],eax                    | adc    BYTE PTR [ebx-0x6effff0],al
   * 10001217  adc    BYTE PTR [ebx-0x6effff0],al            | adc    DWORD PTR [eax],eax
   * 1000121d  adc    DWORD PTR [eax],eax                    | adc    cl,bh
   * 1000121f  adc    BYTE PTR [eax],al                      | adc    DWORD PTR [eax],eax
   * 10001221  add    BYTE PTR [eax],al                      | adc    BYTE PTR [ebp-0x6efffef],dl
   * 10001223  add    BYTE PTR [esi],al                      | adc    DWORD PTR [eax],eax
   * 10001225  push   es                                     | adc    BYTE PTR [eax],al
   * 10001226  push   es                                     | add    BYTE PTR [eax],al
   * 10001227  add    DWORD PTR [esi],eax                    | add    BYTE PTR [ecx],cl
   * 10001229  add    al,BYTE PTR [edx]                      | add    DWORD PTR [ecx],ecx
   * 1000122b  add    al,BYTE PTR [edx]                      | add    cl,BYTE PTR [ecx]
   * 1000122d  add    al,BYTE PTR [esi]                      | add    eax,DWORD PTR [ebx]
   * 1000122f  push   es                                     | add    eax,DWORD PTR [ebx]
   * 10001230  add    eax,DWORD PTR [ebx]                    | add    ecx,DWORD PTR [ecx]
   * 10001232  add    eax,DWORD PTR [ebx]                    | add    al,0x5
   * 10001234  push   es                                     | add    eax,0x6090505
   * 10001235  push   es                                     | or     DWORD PTR [edi],eax
   * 10001236  push   es                                     | or     DWORD PTR [eax],ecx
   * 10001237  add    al,0x6                                 | or     BYTE PTR [eax],cl
   * 10001239  add    eax,0x5050505                          | or     BYTE PTR [eax],cl
   * 1000123e  mov    edi,edi                                | lea    ebx,[ebx+0x0]
```

## What HANDOFF.md already records (most recent last)
- - Session 30: pointer-alias trick tried on SoundInit (SOUNDFX *fx per entry: 10, worse), AddObjectType/AddTrapType (SPRITEREC *f per frame: 21/17 unchanged), WaveTestVolume (dead GetNumDevs forms: 13 unchanged). Scanner (original keeps a constant in a register where ours uses immediates; in try.sh -v output the RIGHT column is the original): hits only in ApplyOptions (esi=1/2/4 zoom), LayoutWindows/OpenLevelItem (esi=1), ShowLevelDetails (ebp=0x156 loop id, edi=&g_infoLsSkills), DrawPreview (edi=0x41cbd4, ecx=-1), Basher::Process (ecx=0x10009008), SoundInit (table base), WaveTestVolume (100). These are loop/induction shapes rather than struct-store aliasing; untested beyond the above.
- - **PROCESS misses left (8):** Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectType 21, Miner::Process 25, Basher::Process 27, BuildCollision 32.
- - **General lesson (try on other register-tie misses: AddTrapType/AddObjectType, Miner/Basher::Process, DrawLemmings, BuildRows...):** wrap individual repeated expression statements in `{ T tmp = expr; use(tmp); }` blocks, one position at a time (2^n sweep), instead of only reordering declarations.
- ### Session 32: block-temp sweep on Miner::Process and Basher::Process - no gain
- - Basher::Process (27): 255 variants over its 8 local-assigning statements (`ba_spec.py`): nothing below 27 (many worse, 34).
- - Remaining: PROCESS Basher 27, Builder 3, Miner 25, AddObjectType 32 (mid-experiment; was 21), AddTrapType 17, BuildCollision 2, WaveTestVolume 13; EXE ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4; RENDER Renderer_Render, CopyChanged, StretchChanged.
- - Tried this session (negative): BuildCollision real-name pad sweep 0-63 (stays 2: the prologue order is not pad-sensitive); inline `Message()` locals (hook `MESSAGE_PAD`, k 0-31) on Miner and Basher: no gain (hook reverted); Builder block temps (7 variants): stays 3.
- - Heavy sweep done (`build/scratch/s35/kt.py`, `kt_miner.log`, `kt_basher.log`): inline `Message()` locals k 0-15 x total padding T 0-31 (512 builds each, 3 jobs): Miner::Process best 25 and Basher::Process best 27 (= current); the switch-driven misses are not inline-expansion phase effects. The `MESSAGE_PAD` hook in lemming.h was removed again.
- - **Session 36 end state (verified 2026-11-02):** **WaveTestVolume MATCHES** (src/process/wave.cpp 14/14, `build.py --funcs PROCESS` = **200/206**, `build/scratch/s36/process_wavematch.log`; `.bss` unchanged (46 pre-existing wrong); `make` OK; Wine HD level 1 and digger runs MATCH the baselines, `build/runs/s36-l1`, `s36-dig`). Totals **679/696**: PROCESS 6 misses (Basher 27, Builder 3, Miner 25, AddObjectType 32, AddTrapType 17, BuildCollision 2), LEMMINGS 8 (ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4), RENDER 3 = 17 left. **The fix:** `devs = waveOutGetNumDevs(); switch (n = devs >= 100 ? 100 : devs) {
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33 declaration climbs (`tools/msvc/search/declclimb.py`, pairwise swaps + move-to-front, pads 0-3/0-6; logs `build/scratch/s33_bp/*_climb.log`): no new matches. Builder::Process 3, DrawPreview 36, DockWnd::Drag 17, ScrollTo 22 (from 23, pad 0, not applied), SetViewScale 45 (from 46, pad 2, not applied), ShowLevelDetails 37, Miner::Process 25, Basher::Process 27, Map::AddTrapType 17. None of these is a declaration-order tie; each needs a source-shape change. WaveTestVolume skipped: the original keeps the clamped volume in esi though it is never used again (a dead value in a callee-saved register), which no declaration order explains.
- - Session 33, inline/padding hunt (no new match): `Message` (lemming.h, a plain inline function, not a class member) dummy locals 0-8 x padding for Miner (pad_top 0-40) and Basher (pad_basherprocess 0-40): no effect (hook removed again, lemming.h unchanged). End-of-file padding (file-wide symbol count) does not move non-inline functions (Builder, WaveTestVolume, Miner, Basher, AddObjectType all unchanged), so the count effect is specific to class-member inlines. pad_top 0-64 for AddObjectType/AddTrapType: no gain. BuildCollision with all declarations at the top + declclimb: 2. Renderer_Render: 8 best setup orders x pads 0-64 and 5 canvas-pointer spellings: 5.
- - Session 33, split-and-climb batch (new tool `tools/msvc/search/splitclimb.py MODULE SOURCE ADDR [--body F] [--pads A-B] [--apply]`: the FillObjects recipe, i.e. every initialised/mid-block declaration becomes a top declaration plus an assignment in place, then a pairwise declaration climb; compiles every variant under the REAL file name in `build/scratch/splitclimb/<func>/<k>/`, whose baseline reproduces try.sh on the real file, unlike variants.py, which gave Miner 32 instead of 25). Results, best (insns, unmatched in file): ScrollTo (short x body) 21 (pads 0-3), 18 at pad 19 but costs a file match; SetViewScale 46; OpenLevelItem 27; ShowLevelDetails 37; DrawPreview 36; AddTrapType 17; Add
-   - `PROCESS.DLL`: **201/206** (+1 match; only 5 functions remain: `Basher::Process` 27, `Builder::Process` 3, `Miner::Process` 25, `Map::AddObjectType` 32, `Map::AddTrapType` 17).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
