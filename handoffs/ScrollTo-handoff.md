# ScrollTo hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **LEMMINGS.EXE**, function at **0x406490**, source `src/lemmings/gameview.cpp`.
> Status: `0x406490 ScrollTo: DIFF (208 bytes) 134 bytes differ, first at +0x1, 18 insns differ; +0x5 ?g_tempScrollX@@3FA: original 0xbf0f5700, expected 0x4156ac; +0xb ?g_resolutionNew@@3HA: original 0xf0041b6, expected 0x4158d8`

## Goal
Make `ScrollTo` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh LEMMINGS.EXE src/lemmings/gameview.cpp -v -f ScrollTo`) to bytes identical to the original, without breaking the other functions in `gameview.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh LEMMINGS.EXE src/lemmings/gameview.cpp -v -f ScrollTo
python3 tools/msvc/realname.py LEMMINGS.EXE src/lemmings/gameview.cpp 0x406490 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x406490 */
void ScrollTo(HWND hwnd, short dx)
{
    short mask = ~(g_scale * g_resolutionNew - 1);
    int d = (short)(dx & mask);
    int x = g_tempScrollX;
    int n = (g_maxXScroll - x < d) ? g_maxXScroll - x : d;

    if (n <= -x)
        n = -x;
    if ((short)n) {
        g_tempScrollX = (g_tempScrollX + n) & mask;
        ScrollWindow(hwnd, -(short)n, 0, 0, 0);
        SetScrollPos(hwnd, SB_HORZ, g_tempScrollX, TRUE);
        SendMessage(hwnd, 0x500, 0, 0);
        UpdateWindow(hwnd);
        if (!g_replaying)
            RecordScroll(g_recordTime, g_viewW / 2 + g_tempScrollX, 0);
    }
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     00406490  push   esi                                    | push   esi
   * 00406491  push   edi                                    | mov    eax,ds:0x4158d8
   * 00406492  mov    di,WORD PTR ds:0x4156ac                | push   edi
   * 00406499  mov    esi,DWORD PTR ds:0x4158d8              | movsx  edi,WORD PTR ds:0x41b66c
   * 0040649f  imul   si,WORD PTR ds:0x41b66c                | imul   edi,eax
   * 004064a6                                                | dec    di
   * 004064a7  dec    si                                     | movsx  edx,WORD PTR ds:0x4156a0
   * 004064a9  movsx  ecx,di                                 | not    di
   * 004064ac  not    si                                     | mov    ax,WORD PTR [esp+0x10]
   * 004064af  mov    ax,WORD PTR [esp+0x10]                 | and    ax,di
   * 004064b4  and    ax,si                                  | mov    cx,WORD PTR ds:0x4156ac
   * 004064b7  movsx  edx,ax                                 | movsx  eax,ax
   * 004064ba  movsx  eax,WORD PTR ds:0x4156a0               | movsx  esi,cx
   * 004064c1  sub    eax,ecx                                | sub    edx,esi
     004064c3  cmp    edx,eax                                | cmp    edx,eax
   * 004064c5  jl     0x4064c9                               | jl     0x4064ca
     004064c7  mov    edx,eax                                | mov    edx,eax
   * 004064c9  neg    ecx                                    | neg    esi
   * 004064cb  cmp    edx,ecx                                | cmp    edx,esi
   * 004064cd  jg     0x4064d1                               | jg     0x4064d2
   * 004064cf  mov    edx,ecx                                | mov    edx,esi
     004064d1  test   dx,dx                                  | test   dx,dx
   * 004064d4  je     0x406552                               | je     0x406554
   * 004064d6  lea    eax,[edi+edx*1]                        | add    cx,dx
     004064d9  push   0x0                                    | push   0x0
   * 004064db  and    ax,si                                  | and    cx,di
     004064de  push   0x0                                    | push   0x0
   * 004064e0  mov    ds:0x4156ac,ax                         | mov    WORD PTR ds:0x4156ac,cx
     004064e6  push   0x0                                    | push   0x0
     004064e8  movsx  eax,dx                                 | movsx  eax,dx
     004064eb  neg    eax                                    | neg    eax
     004064ed  mov    esi,DWORD PTR [esp+0x18]               | mov    esi,DWORD PTR [esp+0x18]
     004064f1  push   eax                                    | push   eax
     004064f2  push   esi                                    | push   esi
     004064f3  call   DWORD PTR ds:0x41f7f8                  | call   DWORD PTR ds:0x41f7f8
     004064f9  movsx  eax,WORD PTR ds:0x4156ac               | movsx  eax,WORD PTR ds:0x4156ac
     00406500  push   0x1                                    | push   0x1
     00406502  push   eax                                    | push   eax
     00406503  push   0x0                                    | push   0x0
     00406505  push   esi                                    | push   esi
     00406506  call   DWORD PTR ds:0x41f7d8                  | call   DWORD PTR ds:0x41f7d8
     0040650c  push   0x0                                    | push   0x0
     0040650e  push   0x0                                    | push   0x0
     00406510  push   0x500                                  | push   0x500
     00406515  push   esi                                    | push   esi
     00406516  call   DWORD PTR ds:0x41f7f4                  | call   DWORD PTR ds:0x41f7f4
     0040651c  push   esi                                    | push   esi
     0040651d  call   DWORD PTR ds:0x41f7dc                  | call   DWORD PTR ds:0x41f7dc
     00406523  cmp    DWORD PTR ds:0x41b2e0,0x0              | cmp    DWORD PTR ds:0x41b2e0,0x0
   * 0040652a  jne    0x406552                               | jne    0x406554
     0040652c  push   0x0                                    | push   0x0
     0040652e  mov    eax,ds:0x415738                        | mov    eax,ds:0x415738
     00406533  cdq                                           | cdq
     00406534  movsx  ecx,WORD PTR ds:0x4156ac               | movsx  ecx,WORD PTR ds:0x4156ac
     0040653b  sub    eax,edx                                | sub    eax,edx
     0040653d  mov    edx,DWORD PTR ds:0x41b650              | mov    edx,DWORD PTR ds:0x41b650
     00406543  sar    eax,0x1                                | sar    eax,0x1
     00406546  add    eax,ecx                                | add    eax,ecx
     00406548  push   eax                                    | push   eax
     00406549  push   edx                                    | push   edx
     0040654a  call   0x40d720                               | call   0x40d720
     0040654f  add    esp,0xc                                | add    esp,0xc
     00406552  pop    edi                                    | pop    edi
     00406553  pop    esi                                    | pop    esi
     00406554  ret                                           | ret
   * 00406555  lea    esp,[esp+0x0]                          | mov    edi,edi
     00406559  lea    esp,[esp+0x0]                          | lea    esp,[esp+0x0]
```

## What HANDOFF.md already records (most recent last)
- - Session 30: two-swap header search (`s30_hswap/hswap2.py`, multi-name extern lines split): ScrollTo, DrawLemmings, ApplyOptions, DockWnd::Drag no gain (ReadIniSettings single swaps running). OpenLevelItem random decl orders (sampler fixed for field-name clashes): 35 -> 33 applied (variant 176 of `s30_sound/rd_OpenLevelItem.txt`); levelsel.cpp still 36/39. Backup `build/scratch/s30_sound/levelsel.cpp.before2`.
- - **Session 32 padding sweep of the other near-misses** (production body, pad lines 0-40 before the function; `build/scratch/s32/padsweep.log`, `ps/`): ApplyOptions 7, ReadIniSettings 3, DockWnd::DockWnd 4, BuildRows 4, DrawLemmings 7, DrawPane 14, OpenLevelItem 27: no pad count improves them. **Improvements that cost other matches:** DockWnd::Drag 21 -> **6** at 17-18 extra pads (dockwnd.cpp 7 -> 6/9), StartGame 19 -> 10 at 19 pads (mainwnd 24 -> 22/26), ScrollTo 23 -> 18 at 1-2 pads (gameview 33 -> 32/35).
- - **EXE misses left (13):** ApplyOptions 7 (original zeroes edi before the first compare; `!= size`, `register`, extra locals all no), StartGame 19, SaveLayout 27, DrawPane 14, ScrollTo 23, SetViewScale 46, ReadIniSettings 3 (original loads both coordinates into registers before the `g_mapDock` compare; Hopper and Ghidra show nothing more), OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, DockWnd::Drag 17 (compare-orientation flips), Bitmap::BuildRows 4, MapView::DrawLemmings 7. RENDER (3): StretchChanged 3, Renderer_Render 5, CopyChanged 17.
- - gameview.cpp ScrollTo (23; 18 with 1-2 extra pads, unchanged source): the original loads `movsx edi,[g_scale]` and does a 32-bit `imul edi,eax` before the 16-bit dec/not; 7 `(short)g_scale` / int-temp spellings x pads 0-3 (`st_spec.py`) never beat 18. SetViewScale (46) not touched.
- - Remaining: PROCESS Basher 27, Builder 3, Miner 25, AddObjectType 32 (mid-experiment; was 21), AddTrapType 17, BuildCollision 2, WaveTestVolume 13; EXE ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4; RENDER Renderer_Render, CopyChanged, StretchChanged.
-   * **ScrollTo (23):** `int mask = ~(g_scale * g_resolutionNew - 1);` gives the original's 32-bit `movsx esi,[g_scale]; imul esi,eax` but then `dec esi; not esi` (32-bit); `short mask` gives the original's 16-bit `dec/not` but narrows the multiply to `imul cx,word [g_scale]`. No spelling gives both: separate `int p` (forwarded), block temp, `unsigned`, `(short)` / `(long)` / `(unsigned)` casts, `-(x)`, `^ -1`, `+ -1`, split `mask--; mask = ~mask`, an inline `Mul32()` helper (inlined and narrowed again). Specs `build/scratch/st*_spec.py`.
- - **Session 36 end state (verified 2026-11-02):** **WaveTestVolume MATCHES** (src/process/wave.cpp 14/14, `build.py --funcs PROCESS` = **200/206**, `build/scratch/s36/process_wavematch.log`; `.bss` unchanged (46 pre-existing wrong); `make` OK; Wine HD level 1 and digger runs MATCH the baselines, `build/runs/s36-l1`, `s36-dig`). Totals **679/696**: PROCESS 6 misses (Basher 27, Builder 3, Miner 25, AddObjectType 32, AddTrapType 17, BuildCollision 2), LEMMINGS 8 (ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4), RENDER 3 = 17 left. **The fix:** `devs = waveOutGetNumDevs(); switch (n = devs >= 100 ? 100 : devs) {
- - **Session 36 (2026-11-03) APPLIED: ScrollTo 23 -> 18** (`gameview.cpp`: one more pad symbol `pad_scrollto_x0` before it, plus `padplan.py --start 0x406560` for OnHScroll/LevelHeight/... and 92 symbols `pad_setviewscale0..91` before SetViewScale to keep it at 46 (it went to 60), then `padplan --start 0x406810`). EXE still 326/334 (`build/scratch/s36/exe_scroll.log`), `.bss` 0 wrong, `make` OK, Wine HD level 1 + digger MATCH (`s36-l1f`, `s36-digf`); backup `build/scratch/s36/gameview.cpp.before`. Pad sweeps 0-140 over the other near-misses (`sweepall.log`): ApplyOptions 7 at pads 11-13 (not applied: 1 insn, the hoisted `xor edi,edi` and `cmp [g_zoom],esi` remain), BuildRows/AddTrapType/AddOb
- - **Session 36 (2026-11-03) late: new tool + sister check.** `tools/msvc/search/permute.py MODULE SOURCE ADDR` = random semantic-preserving mutation hill-climb (decl swaps, adjacent call-free statement swaps, comparison flips, `A+B`/`A*B` operand swaps, `x += k` <-> `x = x + k`) scored on a TRUNCATED translation unit (`build/scratch/s36/tu1/render.c` = render.c cut before 0x100013a0 with `static` removed from prototypes, `tucs/render.c` = cut before 0x10004d50; ~1.6 s per compile instead of ~7 s). First CopyChanged run (about 3,000 candidates, seed 101): stays 13. Sister project (`/Users/daniel/Decomp-dev/lemmings-lemmings`, checked 11-03 evening): 349 'Match' commits total, 89 since 11-01 (
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33 declaration climbs (`tools/msvc/search/declclimb.py`, pairwise swaps + move-to-front, pads 0-3/0-6; logs `build/scratch/s33_bp/*_climb.log`): no new matches. Builder::Process 3, DrawPreview 36, DockWnd::Drag 17, ScrollTo 22 (from 23, pad 0, not applied), SetViewScale 45 (from 46, pad 2, not applied), ShowLevelDetails 37, Miner::Process 25, Basher::Process 27, Map::AddTrapType 17. None of these is a declaration-order tie; each needs a source-shape change. WaveTestVolume skipped: the original keeps the clamped volume in esi though it is never used again (a dead value in a callee-saved register), which no declaration order explains.
- - Session 33 ScrollTo: the original computes the mask with a 32-bit multiply (`movsx edi,g_scale; imul edi,eax`), then 16-bit `dec di; not di`, and keeps g_tempScrollX as a short in cx for the update (`add cx,dx`). Mask spellings (int temp, casts, two statements, operand order) x short/int x x (x + n) update x compare order (64 builds): best 21 (from 23); VC always narrows the multiply to `imul dx,WORD PTR`. Not applied (`build/scratch/s33_bp/st2.txt`, `st3.txt`).
- - Session 33, split-and-climb batch (new tool `tools/msvc/search/splitclimb.py MODULE SOURCE ADDR [--body F] [--pads A-B] [--apply]`: the FillObjects recipe, i.e. every initialised/mid-block declaration becomes a top declaration plus an assignment in place, then a pairwise declaration climb; compiles every variant under the REAL file name in `build/scratch/splitclimb/<func>/<k>/`, whose baseline reproduces try.sh on the real file, unlike variants.py, which gave Miner 32 instead of 25). Results, best (insns, unmatched in file): ScrollTo (short x body) 21 (pads 0-3), 18 at pad 19 but costs a file match; SetViewScale 46; OpenLevelItem 27; ShowLevelDetails 37; DrawPreview 36; AddTrapType 17; Add
-   - `LEMMINGS.EXE`: **329/334** (+3 matches; only 5 functions remain: `ScrollTo` 18, `SetViewScale` 46, `OpenLevelItem` 27, `DrawPreview` 36, `ShowLevelDetails` 37).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
