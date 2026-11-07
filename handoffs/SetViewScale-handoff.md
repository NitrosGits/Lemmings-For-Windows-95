# SetViewScale hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **LEMMINGS.EXE**, function at **0x406640**, source `src/lemmings/gameview.cpp`.
> Status: `0x406640 SetViewScale: DIFF (464 bytes) 297 bytes differ, first at +0x15, 46 insns differ; +0x1d ?g_tempScrollX@@3FA: original 0xe1c1c80c, expected 0x4156ac; +0x2b ?g_scale@@3FA: original 0xf7669966, expected 0x41b66c`

## Goal
Make `SetViewScale` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh LEMMINGS.EXE src/lemmings/gameview.cpp -v -f SetViewScale`) to bytes identical to the original, without breaking the other functions in `gameview.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh LEMMINGS.EXE src/lemmings/gameview.cpp -v -f SetViewScale
python3 tools/msvc/realname.py LEMMINGS.EXE src/lemmings/gameview.cpp 0x406640 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x406640: fit the view to a zoom and the window's size */
void SetViewScale(HWND hwnd, short scale)
{
    int levelW = g_resolutionNew * 1668;
    int levelH = g_resolutionNew * 160;
    int w;
    int h;
    int n;

    g_tempScrollX = g_tempScrollX / g_scale;
    g_valScrollY = g_valScrollY / g_scale;
    g_scale = scale;
    switch (scale) {
    case 1:
        g_nowRenderScale = 1;
        break;
    case 2:
    case 4:
        g_nowRenderScale = 2;
        break;
    }
    w = g_dataViewWidth / scale;
    if (w >= levelW)
        w = levelW;
    g_viewW = w;
    h = g_extraViewHeight / scale;
    if (h >= levelH)
        h = levelH;
    g_viewHLast = h;
    g_viewPixW = w * scale;
    g_viewPixHGameMain = h * scale;
    n = levelH - h;
    if (n >= g_valScrollY)
        n = g_valScrollY;
    g_valScrollY = n;
    n = levelW - w;
    if (n >= g_tempScrollX)
        n = g_tempScrollX;
    g_tempScrollX = n;
    if (g_valScrollY <= 0)
        g_valScrollY = 0;
    if (g_tempScrollX <= 0)
        g_tempScrollX = 0;
    levelW *= scale;
    levelH *= scale;
    levelH -= g_extraViewHeight;
    if (levelH <= 0)
        levelH = 0;
    g_yMaxScroll = levelH;
    levelW -= g_dataViewWidth;
    if (levelW <= 0)
        levelW = 0;
    g_maxXScroll = levelW;
    g_valScrollY = scale * g_valScrollY;
    g_tempScrollX = scale * g_tempScrollX;
    SetScrollRange(hwnd, SB_VERT, 0, g_yMaxScroll, FALSE);
    SetScrollPos(hwnd, SB_VERT, g_valScrollY, TRUE);
    SetScrollRange(hwnd, SB_HORZ, 0, g_maxXScroll, FALSE);
    SetScrollPos(hwnd, SB_HORZ, g_tempScrollX, TRUE);
    InvalidateRect(hwnd, 0, FALSE);
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     00406640  push   ebx                                    | push   ebx
     00406641  mov    eax,ds:0x4158d8                        | mov    eax,ds:0x4158d8
     00406646  push   esi                                    | push   esi
     00406647  push   edi                                    | push   edi
     00406648  push   ebp                                    | push   ebp
     00406649  lea    ecx,[eax+eax*2]                        | lea    ecx,[eax+eax*2]
     0040664c  lea    edi,[eax+eax*4]                        | lea    edi,[eax+eax*4]
     0040664f  lea    ecx,[eax+ecx*4]                        | lea    ecx,[eax+ecx*4]
     00406652  shl    ecx,0x2                                | shl    ecx,0x2
   * 00406655  shl    edi,0x5                                | mov    si,WORD PTR ds:0x41b66c
   * 00406658  lea    edx,[eax+ecx*8]                        | lea    ecx,[eax+ecx*8]
   * 0040665b  mov    ax,ds:0x4156ac                         | shl    ecx,0x2
   * 00406661  lea    ebx,[edx*4+0x0]                        | shl    edi,0x5
   * 00406668  mov    cx,WORD PTR ds:0x41b66c                | mov    ax,ds:0x4156ac
     0040666f  cwd                                           | cwd
   * 00406671  mov    si,WORD PTR [esp+0x18]                 | idiv   si
   * 00406676  idiv   cx                                     | mov    ds:0x4156ac,ax
   * 00406679  mov    ds:0x4156ac,ax                         | mov    ax,ds:0x415744
   * 0040667f  mov    ax,ds:0x415744                         | cwd
   * 00406685  cwd                                           | idiv   si
   * 00406687  idiv   cx                                     | mov    si,WORD PTR [esp+0x18]
   * 0040668a  movsx  ecx,si                                 | movsx  ebx,si
     0040668d  mov    ds:0x415744,ax                         | mov    ds:0x415744,ax
   * 00406693  cmp    ecx,0x1                                | cmp    ebx,0x1
     00406696  mov    WORD PTR ds:0x41b66c,si                | mov    WORD PTR ds:0x41b66c,si
   * 0040669d  je     0x4066ab                               | je     0x4066a7
   * 0040669f  cmp    ecx,0x2                                | cmp    ebx,0x2
   * 004066a2  je     0x4066b7                               | je     0x4066b3
   * 004066a4  cmp    ecx,0x4                                | cmp    ebx,0x4
   * 004066a7  je     0x4066b7                               | je     0x4066b3
   * 004066a9  jmp    0x4066c1                               | jmp    0x4066bd
     004066ab  mov    DWORD PTR ds:0x4157ec,0x1              | mov    DWORD PTR ds:0x4157ec,0x1
     004066b2                                                | 
   * 004066b5  jmp    0x4066c1                               | jmp    0x4066bd
     004066b7  mov    DWORD PTR ds:0x4157ec,0x2              | mov    DWORD PTR ds:0x4157ec,0x2
     004066be                                                | 
     004066c1  mov    ax,ds:0x41574c                         | mov    ax,ds:0x41574c
     004066c7  cwd                                           | cwd
     004066c9  idiv   si                                     | idiv   si
     004066cc  movsx  ebp,ax                                 | movsx  ebp,ax
   * 004066cf  cmp    ebp,ebx                                | cmp    ebp,ecx
   * 004066d1  jl     0x4066d5                               | jl     0x4066d1
   * 004066d3  mov    ebp,ebx                                | mov    ebp,ecx
     004066d5  mov    ax,ds:0x415754                         | mov    ax,ds:0x415754
     004066db  mov    DWORD PTR ds:0x415738,ebp              | mov    DWORD PTR ds:0x415738,ebp
     004066e1  cwd                                           | cwd
     004066e3  idiv   si                                     | idiv   si
     004066e6  movsx  eax,ax                                 | movsx  eax,ax
     004066e9  cmp    eax,edi                                | cmp    eax,edi
   * 004066eb  jl     0x4066ef                               | jl     0x4066eb
     004066ed  mov    eax,edi                                | mov    eax,edi
     004066ef  mov    edx,ebp                                | mov    edx,ebp
     004066f1  mov    ds:0x41575c,eax                        | mov    ds:0x41575c,eax
   * 004066f6  imul   edx,ecx                                | imul   edx,ebx
     004066f9  mov    DWORD PTR ds:0x415748,edx              | mov    DWORD PTR ds:0x415748,edx
     004066ff  mov    edx,eax                                | mov    edx,eax
   * 00406701  imul   edx,ecx                                | imul   edx,ebx
     00406704  mov    DWORD PTR ds:0x4156b0,edx              | mov    DWORD PTR ds:0x4156b0,edx
     0040670a  mov    edx,edi                                | mov    edx,edi
     0040670c  sub    edx,eax                                | sub    edx,eax
     0040670e  movsx  eax,WORD PTR ds:0x415744               | movsx  eax,WORD PTR ds:0x415744
     00406715  cmp    edx,eax                                | cmp    edx,eax
   * 00406717  jl     0x40671b                               | jl     0x406717
     00406719  mov    edx,eax                                | mov    edx,eax
   * 0040671b  mov    eax,ebx                                | movsx  eax,WORD PTR ds:0x4156ac
   * 0040671d  sub    eax,ebp                                | mov    WORD PTR ds:0x415744,dx
   * 0040671f  movsx  ebp,WORD PTR ds:0x4156ac               | mov    edx,ecx
   * 00406726  cmp    eax,ebp                                | sub    edx,ebp
   * 00406728  jl     0x40672c                               | cmp    edx,eax
   * 0040672a  mov    eax,ebp                                | jl     0x40672f
   * 0040672c  mov    WORD PTR ds:0x415744,dx                | mov    edx,eax
   * 00406733  test   dx,dx                                  | mov    ax,ds:0x415744
   * 00406736  jg     0x40673b                               | mov    WORD PTR ds:0x4156ac,dx
   * 00406738  xor    dx,dx                                  | test   ax,ax
   * 0040673b  mov    ds:0x4156ac,ax                         | jg     0x406744
   * 00406741  test   ax,ax                                  | xor    ax,ax
   * 00406744  jg     0x406749                               | mov    ds:0x415744,ax
   * 00406746  xor    ax,ax                                  | mov    ax,ds:0x4156ac
   * 00406749  imul   ebx,ecx                                | test   ax,ax
   * 0040674c  imul   edi,ecx                                | jg     0x406758
   * 0040674f  movsx  ecx,WORD PTR ds:0x415754               | xor    ax,ax
   * 00406756  sub    edi,ecx                                | imul   ecx,ebx
   * 00406758  test   edi,edi                                | mov    ds:0x4156ac,ax
   * 0040675a  jg     0x40675e                               | imul   edi,ebx
   * 0040675c  xor    edi,edi                                | movsx  eax,WORD PTR ds:0x415754
   * 0040675e  movsx  ecx,WORD PTR ds:0x41574c               | sub    edi,eax
   * 00406765  mov    WORD PTR ds:0x41573c,di                | test   edi,edi
   * 0040676c  sub    ebx,ecx                                | jg     0x406773
   * 0040676e  test   ebx,ebx                                | xor    edi,edi
   * 00406770  jg     0x406774                               | movsx  eax,WORD PTR ds:0x41574c
   * 00406772  xor    ebx,ebx                                | mov    WORD PTR ds:0x41573c,di
   * 00406774  mov    cx,si                                  | sub    ecx,eax
   * 00406777  mov    ds:0x4156ac,ax                         | test   ecx,ecx
   * 0040677d  push   0x0                                    | jg     0x406789
   * 0040677f  imul   si,ax                                  | xor    ecx,ecx
   * 00406783  movsx  eax,WORD PTR ds:0x41573c               | mov    ax,si
   * 0040678a  mov    WORD PTR ds:0x415744,dx                | push   0x0
   * 00406791  push   eax                                    | imul   ax,WORD PTR ds:0x415744
   * 00406792  imul   cx,dx                                  | 
   * 00406796  mov    WORD PTR ds:0x4156a0,bx                | mov    WORD PTR ds:0x4156a0,cx
   * 0040679d  push   0x0                                    | mov    edi,DWORD PTR [esp+0x18]
   * 0040679f  mov    WORD PTR ds:0x415744,cx                | imul   si,WORD PTR ds:0x4156ac
   * 004067a6  push   0x1                                    | 
   * 004067a8  mov    WORD PTR ds:0x4156ac,si                | mov    ds:0x415744,ax
   * 004067af  mov    edi,DWORD PTR [esp+0x24]               | mov    ebx,DWORD PTR ds:0x41f7fc
   * 004067b3  push   edi                                    | movsx  eax,WORD PTR ds:0x41573c
   * 004067b4  mov    ebx,DWORD PTR ds:0x41f7fc              | mov    WORD PTR ds:0x4156ac,si
   * 004067ba  call   ebx                                    | push   eax
   * 004067bc  push   0x1                                    | push   0x0
   * 004067be  mov    esi,DWORD PTR ds:0x41f7d8              | push   0x1
   * 004067c4  movsx  eax,WORD PTR ds:0x415744               | push   edi
   * 004067cb  push   eax                                    | call   ebx
     004067cc  push   0x1                                    | push   0x1
   * 004067ce  push   edi                                    | mov    esi,DWORD PTR ds:0x41f7d8
   * 004067cf  call   esi                                    | movsx  eax,WORD PTR ds:0x415744
   * 004067d1  movsx  ecx,WORD PTR ds:0x4156a0               | push   eax
   * 004067d8  push   0x0                                    | push   0x1
   * 004067da  push   ecx                                    | push   edi
   * 004067db  push   0x0                                    | call   esi
   * 004067dd  push   0x0                                    | movsx  ecx,WORD PTR ds:0x4156a0
   * 004067df  push   edi                                    | push   0x0
   * 004067e0  call   ebx                                    | push   ecx
   * 004067e2  movsx  eax,WORD PTR ds:0x4156ac               | push   0x0
   * 004067e9  push   0x1                                    | push   0x0
   * 004067eb  push   eax                                    | push   edi
   * 004067ec  push   0x0                                    | call   ebx
   * 004067ee  push   edi                                    | movsx  eax,WORD PTR ds:0x4156ac
   * 004067ef  call   esi                                    | push   0x1
   * 004067f1  push   0x0                                    | push   eax
     004067f3  push   0x0                                    | push   0x0
     004067f5  push   edi                                    | push   edi
   * 004067f6  call   DWORD PTR ds:0x41f6d8                  | call   esi
   * 004067fc  pop    ebp                                    | push   0x0
   * 004067fd  pop    edi                                    | push   0x0
   * 004067fe  pop    esi                                    | push   edi
   * 004067ff  pop    ebx                                    | call   DWORD PTR ds:0x41f6d8
   * 00406800  ret                                           | pop    ebp
   * 00406801  lea    esp,[esp+0x0]                          | pop    edi
   * 00406805  lea    esp,[esp+0x0]                          | pop    esi
   * 00406809  lea    esp,[esp+0x0]                          | pop    ebx
   * 0040680f                                                | ret
```

## What HANDOFF.md already records (most recent last)
- - **Broad independent EXE batch running:** `s66_batch.py` / `s66_batch.log`, n64 type/order variants with pads0/1, stopping on the first hit. Targets processed so far: OnVScroll 28, DrawPane31, DrawPreview36, OnStartLevel17, ReadIniSettings10, ShowLevelDetails37, OpenLevelItem32 (previous34); DockWnd::Drag produced no results because its declaration block is unsupported, not evidence of exhaustion. TreeEnsureVisible/SaveLayout/SetViewScale follow. Full bodies/results per address under `build/scratch/s66_batch/`.
- - Session 30: **OnVScroll MATCH**: clamp written as the ternary `n = (g_yMaxScroll - g_valScrollY < d) ? g_yMaxScroll - g_valScrollY : d;` (same form ScrollTo uses). gameview.cpp suffix re-planned from ScrollTo; gameview.cpp **33/35** (ScrollTo now 23, SetViewScale 46); full EXE **318/334**, total **668/696**; make OK, Wine HD level-1 (s30v-l1) and digger (s30v-dig) MATCH. Backup `build/scratch/s30_sound/gameview.cpp.before`.
- - Session 30: SetViewScale (46): 120 decl orders best 45; ternary clamp combos (16) best 45. g_scale/scale/levelW register rotation unsolved.
- - **EXE misses left (13):** ApplyOptions 7 (original zeroes edi before the first compare; `!= size`, `register`, extra locals all no), StartGame 19, SaveLayout 27, DrawPane 14, ScrollTo 23, SetViewScale 46, ReadIniSettings 3 (original loads both coordinates into registers before the `g_mapDock` compare; Hopper and Ghidra show nothing more), OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, DockWnd::Drag 17 (compare-orientation flips), Bitmap::BuildRows 4, MapView::DrawLemmings 7. RENDER (3): StretchChanged 3, Renderer_Render 5, CopyChanged 17.
- - gameview.cpp ScrollTo (23; 18 with 1-2 extra pads, unchanged source): the original loads `movsx edi,[g_scale]` and does a 32-bit `imul edi,eax` before the 16-bit dec/not; 7 `(short)g_scale` / int-temp spellings x pads 0-3 (`st_spec.py`) never beat 18. SetViewScale (46) not touched.
- - Remaining: PROCESS Basher 27, Builder 3, Miner 25, AddObjectType 32 (mid-experiment; was 21), AddTrapType 17, BuildCollision 2, WaveTestVolume 13; EXE ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4; RENDER Renderer_Render, CopyChanged, StretchChanged.
- - SetViewScale (46) tried: caching the old g_scale in a local (12 variants), 132 declaration orders, parameter type short/int/WORD/unsigned short/long (short is best): stays 46. The root difference is register roles: the original loads the old `g_scale` into si as the divisor and the `scale` parameter later into ebx (`mov si,[esp+0x18]` after the divisions); ours loads g_scale into cx and the parameter into si first, which then shifts every later compare/imul. Not a pad/T effect (no inline expansions).
- - **Session 36 end state (verified 2026-11-02):** **WaveTestVolume MATCHES** (src/process/wave.cpp 14/14, `build.py --funcs PROCESS` = **200/206**, `build/scratch/s36/process_wavematch.log`; `.bss` unchanged (46 pre-existing wrong); `make` OK; Wine HD level 1 and digger runs MATCH the baselines, `build/runs/s36-l1`, `s36-dig`). Totals **679/696**: PROCESS 6 misses (Basher 27, Builder 3, Miner 25, AddObjectType 32, AddTrapType 17, BuildCollision 2), LEMMINGS 8 (ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4), RENDER 3 = 17 left. **The fix:** `devs = waveOutGetNumDevs(); switch (n = devs >= 100 ? 100 : devs) {
- - **Session 36 (2026-11-03) APPLIED: ScrollTo 23 -> 18** (`gameview.cpp`: one more pad symbol `pad_scrollto_x0` before it, plus `padplan.py --start 0x406560` for OnHScroll/LevelHeight/... and 92 symbols `pad_setviewscale0..91` before SetViewScale to keep it at 46 (it went to 60), then `padplan --start 0x406810`). EXE still 326/334 (`build/scratch/s36/exe_scroll.log`), `.bss` 0 wrong, `make` OK, Wine HD level 1 + digger MATCH (`s36-l1f`, `s36-digf`); backup `build/scratch/s36/gameview.cpp.before`. Pad sweeps 0-140 over the other near-misses (`sweepall.log`): ApplyOptions 7 at pads 11-13 (not applied: 1 insn, the hoisted `xor edi,edi` and `cmp [g_zoom],esi` remain), BuildRows/AddTrapType/AddOb
- - **Session 36 (2026-11-03) late: new tool + sister check.** `tools/msvc/search/permute.py MODULE SOURCE ADDR` = random semantic-preserving mutation hill-climb (decl swaps, adjacent call-free statement swaps, comparison flips, `A+B`/`A*B` operand swaps, `x += k` <-> `x = x + k`) scored on a TRUNCATED translation unit (`build/scratch/s36/tu1/render.c` = render.c cut before 0x100013a0 with `static` removed from prototypes, `tucs/render.c` = cut before 0x10004d50; ~1.6 s per compile instead of ~7 s). First CopyChanged run (about 3,000 candidates, seed 101): stays 13. Sister project (`/Users/daniel/Decomp-dev/lemmings-lemmings`, checked 11-03 evening): 349 'Match' commits total, 89 since 11-01 (
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33 declaration climbs (`tools/msvc/search/declclimb.py`, pairwise swaps + move-to-front, pads 0-3/0-6; logs `build/scratch/s33_bp/*_climb.log`): no new matches. Builder::Process 3, DrawPreview 36, DockWnd::Drag 17, ScrollTo 22 (from 23, pad 0, not applied), SetViewScale 45 (from 46, pad 2, not applied), ShowLevelDetails 37, Miner::Process 25, Basher::Process 27, Map::AddTrapType 17. None of these is a declaration-order tie; each needs a source-shape change. WaveTestVolume skipped: the original keeps the clamped volume in esi though it is never used again (a dead value in a callee-saved register), which no declaration order explains.
- - Session 33, split-and-climb batch (new tool `tools/msvc/search/splitclimb.py MODULE SOURCE ADDR [--body F] [--pads A-B] [--apply]`: the FillObjects recipe, i.e. every initialised/mid-block declaration becomes a top declaration plus an assignment in place, then a pairwise declaration climb; compiles every variant under the REAL file name in `build/scratch/splitclimb/<func>/<k>/`, whose baseline reproduces try.sh on the real file, unlike variants.py, which gave Miner 32 instead of 25). Results, best (insns, unmatched in file): ScrollTo (short x body) 21 (pads 0-3), 18 at pad 19 but costs a file match; SetViewScale 46; OpenLevelItem 27; ShowLevelDetails 37; DrawPreview 36; AddTrapType 17; Add
-   - `LEMMINGS.EXE`: **329/334** (+3 matches; only 5 functions remain: `ScrollTo` 18, `SetViewScale` 46, `OpenLevelItem` 27, `DrawPreview` 36, `ShowLevelDetails` 37).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
