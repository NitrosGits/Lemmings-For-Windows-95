# Miner::Process hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **PROCESS.DLL**, function at **0x10003c60**, source `src/process/miner.cpp`.
> Status: `0x10003c60 Miner::Process: DIFF (368 bytes) 60 bytes differ, first at +0x25, 25 insns differ; +0x78 ?g_sprites@@3PAEA: original 0x90140d, expected 0x10009014`

## Goal
Make `Miner::Process` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh PROCESS.DLL src/process/miner.cpp -v -f Miner::Process`) to bytes identical to the original, without breaking the other functions in `miner.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh PROCESS.DLL src/process/miner.cpp -v -f Miner::Process
python3 tools/msvc/realname.py PROCESS.DLL src/process/miner.cpp 0x10003c60 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x10003c60 */
void Miner::Process(LEMMING *l, DLENTRY **list)
{
    int x, y;
    int n;
    PNT at;

    if (++l->frame >= l->frames)
        l->frame = l->loop - l->frames + l->frame;
    switch (l->frame) {
    case 0:
        l->y++;
        break;
    case 1: case 2:
        x = l->xoff + l->x;
        y = l->yoff + l->y - 3;
        n = l->frame - 1;
        if (n == 0) {
            x += l->dir;
            y++;
        }
        g_map->EraseSprite(list, g_sprites, SPR_MINEHOLE + n + (l->dir == -1 ? 2 : 0), x, y + 1);
        break;
    case 3:
        l->y++;
    case 15:
        l->x += l->dir * 2;
        if (!g_map->GetPixel(l->x, l->y))
            Message(l, MSG_FALL, 0, 0);
        x = l->x & ~3;
        y = l->y & ~3;
        if (CantMine(l, x, y)) {
            l->dir = -l->dir;
            at.x = l->x;
            at.y = l->yoff + l->y;
            SoundPlayFX(at, 6);
            Leave(l);
            g_walker.Enter(l);
        }
        break;
    case 5: case 7: case 9: case 11: case 13:
    case 17: case 19: case 21: case 23:
        break;
    }
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     10003c60  sub    esp,0x8                                | sub    esp,0x8
     10003c63  push   ebx                                    | push   ebx
     10003c64  push   esi                                    | push   esi
     10003c65  mov    esi,DWORD PTR [esp+0x14]               | mov    esi,DWORD PTR [esp+0x14]
     10003c69  push   edi                                    | push   edi
     10003c6a  mov    edi,ecx                                | mov    edi,ecx
     10003c6c  mov    eax,DWORD PTR [esi+0x14]               | mov    eax,DWORD PTR [esi+0x14]
     10003c6f  inc    eax                                    | inc    eax
     10003c70  mov    ecx,DWORD PTR [esi+0x1c]               | mov    ecx,DWORD PTR [esi+0x1c]
     10003c73  mov    DWORD PTR [esi+0x14],eax               | mov    DWORD PTR [esi+0x14],eax
     10003c76  cmp    ecx,eax                                | cmp    ecx,eax
     10003c78  jg     0x10003c84                             | jg     0x10003c84
     10003c7a  mov    edx,DWORD PTR [esi+0x20]               | mov    edx,DWORD PTR [esi+0x20]
     10003c7d  sub    edx,ecx                                | sub    edx,ecx
     10003c7f  add    edx,eax                                | add    edx,eax
     10003c81  mov    DWORD PTR [esi+0x14],edx               | mov    DWORD PTR [esi+0x14],edx
   * 10003c84  mov    ecx,DWORD PTR [esi+0x14]               | mov    edx,DWORD PTR [esi+0x14]
   * 10003c87  cmp    ecx,0x17                               | cmp    edx,0x17
     10003c8a  ja     0x10003d8b                             | ja     0x10003d8b
     10003c90  xor    eax,eax                                | xor    eax,eax
   * 10003c92  mov    al,BYTE PTR [ecx+0x10003dcc]           | mov    al,BYTE PTR [edx+0x10003dcc]
     10003c98  jmp    DWORD PTR [eax*4+0x10003d94]           | jmp    DWORD PTR [eax*4+0x10003d94]
     10003c9f  pop    edi                                    | pop    edi
     10003ca0  inc    DWORD PTR [esi+0x4]                    | inc    DWORD PTR [esi+0x4]
     10003ca3  pop    esi                                    | pop    esi
     10003ca4  pop    ebx                                    | pop    ebx
     10003ca5  add    esp,0x8                                | add    esp,0x8
     10003ca8  ret    0x8                                    | ret    0x8
     10003cab  mov    edi,DWORD PTR [esi+0x8]                | mov    edi,DWORD PTR [esi+0x8]
     10003cae  mov    eax,DWORD PTR [esi+0xc]                | mov    eax,DWORD PTR [esi+0xc]
     10003cb1  add    edi,DWORD PTR [esi]                    | add    edi,DWORD PTR [esi]
   * 10003cb3  mov    edx,DWORD PTR [esi+0x4]                | mov    ecx,DWORD PTR [esi+0x4]
   * 10003cb6  dec    ecx                                    | dec    edx
   * 10003cb7  lea    eax,[eax+edx*1-0x3]                    | lea    ecx,[eax+ecx*1-0x3]
     10003cbb  jne    0x10003cc1                             | jne    0x10003cc1
     10003cbd  add    edi,DWORD PTR [esi+0x10]               | add    edi,DWORD PTR [esi+0x10]
   * 10003cc0  inc    eax                                    | inc    ecx
   * 10003cc1  inc    eax                                    | mov    eax,DWORD PTR [esi+0x10]
   * 10003cc2  push   eax                                    | inc    eax
   * 10003cc3  push   edi                                    | cmp    eax,0x1
   * 10003cc4  mov    eax,DWORD PTR [esi+0x10]               | sbb    eax,eax
   * 10003cc7  inc    eax                                    | inc    ecx
   * 10003cc8  cmp    eax,0x1                                | and    eax,0x2
   * 10003ccb  sbb    eax,eax                                | push   ecx
   * 10003ccd  mov    edx,DWORD PTR [esp+0x24]               | push   edi
   * 10003cd1  and    eax,0x2                                | add    eax,edx
   * 10003cd4  add    eax,ecx                                | add    eax,0x158
     10003cd6  mov    ecx,DWORD PTR ds:0x10009014            | mov    ecx,DWORD PTR ds:0x10009014
   * 10003cdc  add    eax,0x158                              | mov    edx,DWORD PTR [esp+0x24]
     10003ce1  push   eax                                    | push   eax
     10003ce2  push   ecx                                    | push   ecx
     10003ce3  push   edx                                    | push   edx
     10003ce4  mov    ecx,DWORD PTR ds:0x10009288            | mov    ecx,DWORD PTR ds:0x10009288
     10003cea  call   0x10004e10                             | call   0x10004e10
     10003cef  pop    edi                                    | pop    edi
     10003cf0  pop    esi                                    | pop    esi
     10003cf1  pop    ebx                                    | pop    ebx
     10003cf2  add    esp,0x8                                | add    esp,0x8
     10003cf5  ret    0x8                                    | ret    0x8
     10003cf8  inc    DWORD PTR [esi+0x4]                    | inc    DWORD PTR [esi+0x4]
     10003cfb  mov    eax,DWORD PTR [esi+0x10]               | mov    eax,DWORD PTR [esi+0x10]
     10003cfe  mov    ecx,DWORD PTR [esi+0x4]                | mov    ecx,DWORD PTR [esi+0x4]
     10003d01  add    eax,eax                                | add    eax,eax
     10003d03  push   ecx                                    | push   ecx
     10003d04  add    eax,DWORD PTR [esi]                    | add    eax,DWORD PTR [esi]
     10003d06  push   eax                                    | push   eax
     10003d07  mov    DWORD PTR [esi],eax                    | mov    DWORD PTR [esi],eax
     10003d09  mov    ecx,DWORD PTR ds:0x10009288            | mov    ecx,DWORD PTR ds:0x10009288
     10003d0f  call   0x100054c0                             | call   0x100054c0
     10003d14  test   al,al                                  | test   al,al
     10003d16  jne    0x10003d3d                             | jne    0x10003d3d
     10003d18  mov    ebx,DWORD PTR [esi+0x64]               | mov    ebx,DWORD PTR [esi+0x64]
     10003d1b  test   ebx,ebx                                | test   ebx,ebx
     10003d1d  je     0x10003d3d                             | je     0x10003d3d
     10003d1f  push   esi                                    | push   esi
     10003d20  push   0x0                                    | push   0x0
     10003d22  push   0x0                                    | push   0x0
     10003d24  push   0x101                                  | push   0x101
     10003d29  call   DWORD PTR [ebx]                        | call   DWORD PTR [ebx]
     10003d2b  add    esp,0x10                               | add    esp,0x10
     10003d2e  test   eax,eax                                | test   eax,eax
     10003d30  je     0x10003d36                             | je     0x10003d36
     10003d32  xor    ebx,ebx                                | xor    ebx,ebx
     10003d34  jmp    0x10003d39                             | jmp    0x10003d39
     10003d36  mov    ebx,DWORD PTR [ebx+0x4]                | mov    ebx,DWORD PTR [ebx+0x4]
     10003d39  test   ebx,ebx                                | test   ebx,ebx
     10003d3b  jne    0x10003d1f                             | jne    0x10003d1f
     10003d3d  mov    eax,DWORD PTR [esi]                    | mov    eax,DWORD PTR [esi]
     10003d3f  mov    ecx,DWORD PTR [esi+0x4]                | mov    ecx,DWORD PTR [esi+0x4]
     10003d42  and    eax,0xfffffffc                         | and    eax,0xfffffffc
     10003d45  and    ecx,0xfffffffc                         | and    ecx,0xfffffffc
     10003d48  push   ecx                                    | push   ecx
     10003d49  push   eax                                    | push   eax
     10003d4a  push   esi                                    | push   esi
     10003d4b  call   0x10003c20                             | call   0x10003c20
     10003d50  add    esp,0xc                                | add    esp,0xc
     10003d53  test   eax,eax                                | test   eax,eax
     10003d55  je     0x10003d8b                             | je     0x10003d8b
     10003d57  mov    eax,DWORD PTR [esi+0x10]               | mov    eax,DWORD PTR [esi+0x10]
     10003d5a  mov    ecx,DWORD PTR [esi]                    | mov    ecx,DWORD PTR [esi]
     10003d5c  mov    DWORD PTR [esp+0xc],ecx                | mov    DWORD PTR [esp+0xc],ecx
     10003d60  push   0x6                                    | push   0x6
     10003d62  neg    eax                                    | neg    eax
     10003d64  mov    edx,DWORD PTR [esp+0x10]               | mov    edx,DWORD PTR [esp+0x10]
     10003d68  mov    DWORD PTR [esi+0x10],eax               | mov    DWORD PTR [esi+0x10],eax
     10003d6b  mov    eax,DWORD PTR [esi+0xc]                | mov    eax,DWORD PTR [esi+0xc]
     10003d6e  add    eax,DWORD PTR [esi+0x4]                | add    eax,DWORD PTR [esi+0x4]
     10003d71  push   eax                                    | push   eax
     10003d72  push   edx                                    | push   edx
     10003d73  call   0x10005e30                             | call   0x10005e30
     10003d78  push   esi                                    | push   esi
     10003d79  mov    ebx,DWORD PTR [edi]                    | mov    ebx,DWORD PTR [edi]
     10003d7b  mov    ecx,edi                                | mov    ecx,edi
     10003d7d  call   DWORD PTR [ebx+0x8]                    | call   DWORD PTR [ebx+0x8]
     10003d80  push   esi                                    | push   esi
     10003d81  mov    ecx,0x10009008                         | mov    ecx,0x10009008
     10003d86  call   0x10004630                             | call   0x10004630
     10003d8b  pop    edi                                    | pop    edi
     10003d8c  pop    esi                                    | pop    esi
     10003d8d  pop    ebx                                    | pop    ebx
     10003d8e  add    esp,0x8                                | add    esp,0x8
     10003d91  ret    0x8                                    | ret    0x8
     10003d94  lahf                                          | lahf
     10003d95  cmp    al,0x0                                 | cmp    al,0x0
     10003d97  adc    BYTE PTR [ebx-0x7efffc4],ch            | adc    BYTE PTR [ebx-0x7efffc4],ch
     10003d9d  cmp    al,0x0                                 | cmp    al,0x0
     10003d9f  adc    BYTE PTR [ebx-0x74efffc3],cl           | adc    BYTE PTR [ebx-0x74efffc3],cl
     10003da5  cmp    eax,0x3d8b1000                         | cmp    eax,0x3d8b1000
     10003daa  add    BYTE PTR [eax],dl                      | add    BYTE PTR [eax],dl
     10003dac  mov    edi,DWORD PTR ds:0x3d8b1000            | mov    edi,DWORD PTR ds:0x3d8b1000
     10003db2  add    BYTE PTR [eax],dl                      | add    BYTE PTR [eax],dl
     10003db4  sti                                           | sti
     10003db5  cmp    al,0x0                                 | cmp    al,0x0
   * 10003db7  adc    BYTE PTR [eax],al                      | adc    BYTE PTR [ebx-0x74efffc3],cl
   * 10003db9  add    DWORD PTR [ecx],eax                    | cmp    eax,0x3d8b1000
   * 10003dbb  add    cl,BYTE PTR [eax]                      | add    BYTE PTR [eax],dl
   * 10003dbd  or     BYTE PTR [eax],cl                      | mov    edi,DWORD PTR ds:0x3d8b1000
   * 10003dbf  add    ecx,DWORD PTR [eax]                    | add    BYTE PTR [eax],dl
   * 10003dc1  or     BYTE PTR [eax],cl                      | add    BYTE PTR [ecx],al
   * 10003dc3  add    al,0x8                                 | add    DWORD PTR [edx],eax
   * 10003dc5  or     BYTE PTR [eax],cl                      | or     eax,0xd040d03
   * 10003dc7  add    eax,0x6080808                          | add    eax,0x70d060d
   * 10003dcc  or     BYTE PTR [eax],cl                      | or     eax,0xd090d08
   * 10003dce  or     BYTE PTR [edi],al                      | or     cl,BYTE PTR ds:0x50c0d0b
```

## What HANDOFF.md already records (most recent last)
- - **Frame-advance register swap** in Basher/Miner/Builder::Process: insensitive to padding, declaration order and every source form tried.
- - **PROCESS misses left (8):** Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectType 21, Miner::Process 25, Basher::Process 27, BuildCollision 32.
- - **General lesson (try on other register-tie misses: AddTrapType/AddObjectType, Miner/Basher::Process, DrawLemmings, BuildRows...):** wrap individual repeated expression statements in `{ T tmp = expr; use(tmp); }` blocks, one position at a time (2^n sweep), instead of only reordering declarations.
- ### Session 32: block-temp sweep on Miner::Process and Basher::Process - no gain
- - Miner::Process (25): 255 variants wrapping the 8 local-assigning statements (`{ int t = expr; x = t; }`, `mi_spec.py`) and 7 switch-value spellings (`mi2_spec.py`): nothing below 25. Residual is the switch value / temp register roles (original `mov edx,[esi+0x14]` for the switch, ours ecx; the case 1/2 `ecx`/`edx` roles swapped).
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
