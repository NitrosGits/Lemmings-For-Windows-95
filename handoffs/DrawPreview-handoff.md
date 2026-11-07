# DrawPreview hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **LEMMINGS.EXE**, function at **0x40b840**, source `src/lemmings/levelsel.cpp`.
> Status: `0x40b840 DrawPreview: DIFF (464 bytes) 221 bytes differ, first at +0x13, 36 insns differ; +0xb7 __imp__GetLastError@0: original 0xe8685000, expected 0x41f5a4; +0xbe __imp__wsprintfA: original 0x2c24448d, expected 0x41f778`

## Goal
Make `DrawPreview` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh LEMMINGS.EXE src/lemmings/levelsel.cpp -v -f DrawPreview`) to bytes identical to the original, without breaking the other functions in `levelsel.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh LEMMINGS.EXE src/lemmings/levelsel.cpp -v -f DrawPreview
python3 tools/msvc/realname.py LEMMINGS.EXE src/lemmings/levelsel.cpp 0x40b840 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x40b840: the level's picture */
void DrawPreview(HDC hdc, RECT *r, int index)
{
    char *none = "No Level Selected";
    int w;
    int h;
    HDC mem;
    HPALETTE oldMemPal;
    HBITMAP oldBitmap;
    HPALETTE oldPal;
    HGDIOBJ old;
    COLORREF oldColour;
    int len;
    int x;

    if (g_newPreview) {
        char text[128];

        GetBitmapSize(g_newPreview, &w, &h);
        mem = CreateCompatibleDC(hdc);
        oldMemPal = SelectPalette(mem, g_previewPalNewUsed, FALSE);
        oldBitmap = (HBITMAP)SelectObject(mem, g_newPreview);
        oldPal = SelectPalette(hdc, g_previewPalNewUsed, TRUE);
        RealizePalette(hdc);
        if (!StretchBlt(hdc, r->left, r->top, r->right, r->bottom, mem, 0, 0, w, h, SRCCOPY)) {
            wsprintf(text, "StretchBlt failed = %d\n", GetLastError());
            MessageBox(0, text, "DEBUG", 0);
        }
        SelectObject(mem, oldBitmap);
        SelectPalette(hdc, oldPal, TRUE);
        SelectPalette(mem, oldMemPal, FALSE);
        DeleteDC(mem);
    } else {
        SIZE size;

        old = SelectObject(hdc, GetStockObject(DKGRAY_BRUSH));
        Rectangle(hdc, r->left, r->top, r->right, r->bottom);
        SelectObject(hdc, old);
        oldColour = SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
        len = strlen(none);
        GetTextExtentPoint(hdc, none, len, &size);
        x = (r->right - r->left) / 2 - size.cx / 2;
        ExtTextOut(hdc, x, 0, ETO_OPAQUE, r, none, len, 0);
        SetBkColor(hdc, oldColour);
    }
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     0040b840  sub    esp,0x94                               | sub    esp,0x94
     0040b846  cmp    DWORD PTR ds:0x416824,0x0              | cmp    DWORD PTR ds:0x416824,0x0
     0040b84d  push   ebx                                    | push   ebx
     0040b84e  push   esi                                    | push   esi
     0040b84f  push   edi                                    | push   edi
     0040b850  push   ebp                                    | push   ebp
   * 0040b851  je     0x40b957                               | je     0x40b952
     0040b857  lea    eax,[esp+0x20]                         | lea    eax,[esp+0x20]
     0040b85b  lea    ecx,[esp+0x1c]                         | lea    ecx,[esp+0x1c]
     0040b85f  push   eax                                    | push   eax
     0040b860  mov    edx,DWORD PTR ds:0x416824              | mov    edx,DWORD PTR ds:0x416824
     0040b866  push   ecx                                    | push   ecx
     0040b867  push   edx                                    | push   edx
     0040b868  call   0x40b810                               | call   0x40b810
     0040b86d  mov    esi,DWORD PTR [esp+0xb4]               | mov    esi,DWORD PTR [esp+0xb4]
     0040b874  add    esp,0xc                                | add    esp,0xc
     0040b877  push   esi                                    | push   esi
     0040b878  call   DWORD PTR ds:0x41f540                  | call   DWORD PTR ds:0x41f540
   * 0040b87e  mov    ebx,eax                                | mov    ebp,eax
     0040b880  push   0x0                                    | push   0x0
     0040b882  mov    eax,ds:0x4168a8                        | mov    eax,ds:0x4168a8
     0040b887  mov    edi,DWORD PTR ds:0x41f518              | mov    edi,DWORD PTR ds:0x41f518
     0040b88d  push   eax                                    | push   eax
   * 0040b88e  push   ebx                                    | push   ebp
     0040b88f  call   edi                                    | call   edi
     0040b891  mov    ecx,DWORD PTR ds:0x416824              | mov    ecx,DWORD PTR ds:0x416824
     0040b897  mov    DWORD PTR [esp+0x10],eax               | mov    DWORD PTR [esp+0x10],eax
     0040b89b  push   ecx                                    | push   ecx
   * 0040b89c  push   ebx                                    | push   ebp
     0040b89d  call   DWORD PTR ds:0x41f4f4                  | call   DWORD PTR ds:0x41f4f4
     0040b8a3  mov    DWORD PTR [esp+0x18],eax               | mov    DWORD PTR [esp+0x18],eax
     0040b8a7  push   0x1                                    | push   0x1
     0040b8a9  mov    eax,ds:0x4168a8                        | mov    eax,ds:0x4168a8
     0040b8ae  push   eax                                    | push   eax
     0040b8af  push   esi                                    | push   esi
     0040b8b0  call   edi                                    | call   edi
     0040b8b2  mov    DWORD PTR [esp+0x14],eax               | mov    DWORD PTR [esp+0x14],eax
     0040b8b6  push   esi                                    | push   esi
     0040b8b7  call   DWORD PTR ds:0x41f510                  | call   DWORD PTR ds:0x41f510
     0040b8bd  mov    eax,DWORD PTR [esp+0x20]               | mov    eax,DWORD PTR [esp+0x20]
     0040b8c1  push   0xcc0020                               | push   0xcc0020
     0040b8c6  mov    ecx,DWORD PTR [esp+0x20]               | mov    ecx,DWORD PTR [esp+0x20]
     0040b8ca  push   eax                                    | push   eax
   * 0040b8cb  mov    ebp,DWORD PTR [esp+0xb4]               | mov    ebx,DWORD PTR [esp+0xb4]
     0040b8d2  push   ecx                                    | push   ecx
     0040b8d3  push   0x0                                    | push   0x0
     0040b8d5  push   0x0                                    | push   0x0
   * 0040b8d7  push   ebx                                    | push   ebp
   * 0040b8d8  mov    eax,DWORD PTR [ebp+0xc]                | mov    eax,DWORD PTR [ebx+0xc]
     0040b8db  push   eax                                    | push   eax
   * 0040b8dc  mov    ecx,DWORD PTR [ebp+0x8]                | mov    ecx,DWORD PTR [ebx+0x8]
     0040b8df  push   ecx                                    | push   ecx
   * 0040b8e0  mov    edx,DWORD PTR [ebp+0x4]                | mov    edx,DWORD PTR [ebx+0x4]
     0040b8e3  push   edx                                    | push   edx
   * 0040b8e4  mov    eax,DWORD PTR [ebp+0x0]                | mov    eax,DWORD PTR [ebx]
     0040b8e7  push   eax                                    | push   eax
   * 0040b8e8  mov    ebp,DWORD PTR ds:0x41f548              | push   esi
   * 0040b8ee  push   esi                                    | call   DWORD PTR ds:0x41f548
   * 0040b8ef  call   ebp                                    | test   eax,eax
   * 0040b8f1  test   eax,eax                                | jne    0x40b920
   * 0040b8f3  jne    0x40b925                               | call   DWORD PTR ds:0x41f5a4
   * 0040b8f5  call   DWORD PTR ds:0x41f5a4                  | push   eax
   * 0040b8fb  push   eax                                    | push   0x41cbe8
   * 0040b8fc  mov    ebp,DWORD PTR ds:0x41f778              | lea    eax,[esp+0x2c]
   * 0040b902  lea    eax,[esp+0x28]                         | push   eax
   * 0040b906  push   0x0                                    | call   DWORD PTR ds:0x41f778
   * 0040b90b  push   eax                                    | lea    ecx,[esp+0x30]
   * 0040b90c  call   ebp                                    | add    esp,0xc
   * 0040b90e  lea    ecx,[esp+0x30]                         | push   0x0
   * 0040b912  add    esp,0xc                                | push   0x41cc00
   * 0040b915  push   0x0                                    | push   ecx
     0040b917  push   0x0                                    | push   0x0
   * 0040b91c  push   ecx                                    | call   DWORD PTR ds:0x41f77c
   * 0040b91d  push   0x0                                    | mov    eax,DWORD PTR [esp+0x18]
   * 0040b91f  call   DWORD PTR ds:0x41f77c                  | push   eax
   * 0040b925  mov    eax,DWORD PTR [esp+0x18]               | push   ebp
   * 0040b929  push   eax                                    | call   DWORD PTR ds:0x41f4f4
   * 0040b92a  push   ebx                                    | mov    eax,DWORD PTR [esp+0x14]
   * 0040b92b  call   DWORD PTR ds:0x41f4f4                  | push   0x1
   * 0040b931  mov    eax,DWORD PTR [esp+0x14]               | push   eax
   * 0040b935  push   0x1                                    | push   esi
   * 0040b937  push   eax                                    | call   edi
   * 0040b938  push   esi                                    | mov    eax,DWORD PTR [esp+0x10]
   * 0040b939  call   edi                                    | push   0x0
   * 0040b93b  mov    eax,DWORD PTR [esp+0x10]               | push   eax
   * 0040b93f  push   0x0                                    | push   ebp
   * 0040b941  push   eax                                    | call   edi
   * 0040b942  push   ebx                                    | push   ebp
   * 0040b943  call   edi                                    | call   DWORD PTR ds:0x41f514
   * 0040b945  push   ebx                                    | pop    ebp
   * 0040b946  call   DWORD PTR ds:0x41f514                  | pop    edi
   * 0040b94c  pop    ebp                                    | pop    esi
   * 0040b94d  pop    edi                                    | pop    ebx
   * 0040b94e  pop    esi                                    | add    esp,0x94
   * 0040b94f  pop    ebx                                    | ret
   * 0040b950  add    esp,0x94                               | push   0x3
   * 0040b956  ret                                           | call   DWORD PTR ds:0x41f504
   * 0040b957  push   0x3                                    | mov    esi,DWORD PTR [esp+0xa8]
   * 0040b959  call   DWORD PTR ds:0x41f504                  | push   eax
   * 0040b95f  mov    esi,DWORD PTR [esp+0xa8]               | push   esi
   * 0040b966  push   eax                                    | call   DWORD PTR ds:0x41f4f4
   * 0040b967  push   esi                                    | mov    edi,eax
   * 0040b968  call   DWORD PTR ds:0x41f4f4                  | mov    ebx,DWORD PTR [esp+0xac]
   * 0040b96e  mov    edi,eax                                | mov    eax,DWORD PTR [ebx+0xc]
   * 0040b970  mov    ebp,DWORD PTR [esp+0xac]               | mov    ecx,DWORD PTR [ebx+0x8]
   * 0040b977  mov    eax,DWORD PTR [ebp+0xc]                | push   eax
   * 0040b97a  mov    ecx,DWORD PTR [ebp+0x8]                | mov    edx,DWORD PTR [ebx+0x4]
   * 0040b97d  push   eax                                    | push   ecx
   * 0040b97e  mov    edx,DWORD PTR [ebp+0x4]                | mov    eax,DWORD PTR [ebx]
   * 0040b981  push   ecx                                    | push   edx
   * 0040b982  mov    eax,DWORD PTR [ebp+0x0]                | mov    ebp,DWORD PTR ds:0x41f538
   * 0040b985  push   edx                                    | push   eax
   * 0040b986  push   eax                                    | push   esi
   * 0040b987  push   esi                                    | call   ebp
   * 0040b988  call   DWORD PTR ds:0x41f538                  | push   edi
   * 0040b98e  push   edi                                    | push   esi
   * 0040b98f  push   esi                                    | call   DWORD PTR ds:0x41f4f4
   * 0040b990  call   DWORD PTR ds:0x41f4f4                  | push   0xf
   * 0040b996  push   0xf                                    | mov    edi,0x41cbd4
   * 0040b998  mov    edi,0x0                                | call   DWORD PTR ds:0x41f728
   * 0040b99d  call   DWORD PTR ds:0x41f728                  | push   eax
   * 0040b9a3  push   eax                                    | push   esi
   * 0040b9a4  push   esi                                    | call   DWORD PTR ds:0x41f4f8
   * 0040b9a5  call   DWORD PTR ds:0x41f4f8                  | mov    ebp,eax
   * 0040b9ab  mov    ebx,eax                                | mov    ecx,0xffffffff
   * 0040b9ad  mov    ecx,0xffffffff                         | sub    eax,eax
   * 0040b9b2  sub    eax,eax                                | repnz scas al,BYTE PTR es:[edi]
   * 0040b9b4  repnz scas al,BYTE PTR es:[edi]               | not    ecx
   * 0040b9b6  not    ecx                                    | lea    eax,[esp+0x24]
   * 0040b9b8  lea    eax,[esp+0x24]                         | lea    edi,[ecx-0x1]
   * 0040b9bc  lea    edi,[ecx-0x1]                          | push   eax
   * 0040b9bf  push   eax                                    | push   edi
   * 0040b9c0  push   edi                                    | push   0x41cbd4
   * 0040b9c1  push   0x0                                    | push   esi
   * 0040b9c6  push   esi                                    | call   DWORD PTR ds:0x41f554
   * 0040b9c7  call   DWORD PTR ds:0x41f554                  | mov    eax,DWORD PTR [ebx+0x8]
   * 0040b9cd  mov    eax,DWORD PTR [ebp+0x8]                | sub    eax,DWORD PTR [ebx]
   * 0040b9d0  sub    eax,DWORD PTR [ebp+0x0]                | cdq
   * 0040b9d3  cdq                                           | sub    eax,edx
   * 0040b9d4  sub    eax,edx                                | sar    eax,0x1
   * 0040b9d6  sar    eax,0x1                                | push   0x0
   * 0040b9d9  push   0x0                                    | mov    ecx,eax
   * 0040b9db  mov    ecx,eax                                | push   edi
   * 0040b9dd  push   edi                                    | mov    eax,DWORD PTR [esp+0x2c]
   * 0040b9de  mov    eax,DWORD PTR [esp+0x2c]               | push   0x41cbd4
   * 0040b9e2  push   0x0                                    | cdq
   * 0040b9e7  cdq                                           | sub    eax,edx
   * 0040b9e8  sub    eax,edx                                | push   ebx
   * 0040b9ea  push   ebp                                    | sar    eax,0x1
   * 0040b9eb  sar    eax,0x1                                | push   0x2
   * 0040b9ee  push   0x2                                    | sub    ecx,eax
   * 0040b9f0  sub    ecx,eax                                | push   0x0
   * 0040b9f2  push   0x0                                    | push   ecx
   * 0040b9f4  push   ecx                                    | push   esi
   * 0040b9f5  push   esi                                    | call   DWORD PTR ds:0x41f500
   * 0040b9f6  call   DWORD PTR ds:0x41f500                  | push   ebp
   * 0040b9fc  push   ebx                                    | push   esi
   * 0040b9fd  push   esi                                    | call   DWORD PTR ds:0x41f4f8
   * 0040b9fe  call   DWORD PTR ds:0x41f4f8                  | pop    ebp
   * 0040ba04  pop    ebp                                    | pop    edi
   * 0040ba05  pop    edi                                    | pop    esi
   * 0040ba06  pop    esi                                    | pop    ebx
   * 0040ba07  pop    ebx                                    | add    esp,0x94
   * 0040ba08  add    esp,0x94                               | ret
   * 0040ba0e  ret                                           | lea    ebx,[ebx+0x0]
   * 0040ba0f  nop                                           | mov    ecx,DWORD PTR [esp+0x8]
```

## What HANDOFF.md already records (most recent last)
- - Next: larger structural misses (DrawPreview 41, ShowLevelDetails 61, OpenLevelItem 57, MapView::DrawLemmings 36, IconWnd::WndProc 37, SetViewScale 46, StartGame 58) with REA pseudocode first, since the small ones are register ties already searched.
- - Session 30: pointer-alias trick tried on SoundInit (SOUNDFX *fx per entry: 10, worse), AddObjectType/AddTrapType (SPRITEREC *f per frame: 21/17 unchanged), WaveTestVolume (dead GetNumDevs forms: 13 unchanged). Scanner (original keeps a constant in a register where ours uses immediates; in try.sh -v output the RIGHT column is the original): hits only in ApplyOptions (esi=1/2/4 zoom), LayoutWindows/OpenLevelItem (esi=1), ShowLevelDetails (ebp=0x156 loop id, edi=&g_infoLsSkills), DrawPreview (edi=0x41cbd4, ecx=-1), Basher::Process (ecx=0x10009008), SoundInit (table base), WaveTestVolume (100). These are loop/induction shapes rather than struct-store aliasing; untested beyond the above.
- - Session 30: handle-local/pointer-alias/block-scope tries on OpenLevelItem (DBENTRY *e alias, dropped idInit: 35 best), DrawPreview (moving mem/old* locals into the if-block, 48 builds: 36), ShowLevelDetails (byte-swap expression forms x DBENTRY *e alias, 24 builds: 37). No gain; levelsel.cpp unchanged at 36/39. Their diffs are register-allocation swaps over many locals (10-13), too many for a full declaration-order search.
- - Session 30: random declaration-order samples (200 each, `build/scratch/s30_sound/randdecl.py`): ShowLevelDetails best 37, DrawPreview best 36 (baseline). OpenLevelItem skipped (local names equal struct field names; the sampler's dependency check needs fixing).
- - Session 30: OpenLevelItem 33 -> 28 with a second 400-order sample (variant 122 of `rd2_OpenLevelItem.txt`), applied; levelsel.cpp 36/39. ShowLevelDetails and DrawPreview 400-order samples: no gain (37, 36). ReadIniSettings single header swaps (split lines): no gain (3). Next: 600 more OpenLevelItem orders (`rd3_OpenLevelItem.txt`).
- - **EXE misses left (13):** ApplyOptions 7 (original zeroes edi before the first compare; `!= size`, `register`, extra locals all no), StartGame 19, SaveLayout 27, DrawPane 14, ScrollTo 23, SetViewScale 46, ReadIniSettings 3 (original loads both coordinates into registers before the `g_mapDock` compare; Hopper and Ghidra show nothing more), OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, DockWnd::Drag 17 (compare-orientation flips), Bitmap::BuildRows 4, MapView::DrawLemmings 7. RENDER (3): StretchChanged 3, Renderer_Render 5, CopyChanged 17.
- - Remaining: PROCESS Basher 27, Builder 3, Miner 25, AddObjectType 32 (mid-experiment; was 21), AddTrapType 17, BuildCollision 2, WaveTestVolume 13; EXE ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4; RENDER Renderer_Render, CopyChanged, StretchChanged.
-   * **DrawPreview (36):** the whole diff is `mem` in ebp / `r` in ebx in the original (ours reversed) plus ours caching the StretchBlt/wsprintf thunks in ebp where the original calls `[iat]` directly. 15 placements of the locals (function scope vs inside the `if`/`else` blocks) all 36.
- - **Session 36 end state (verified 2026-11-02):** **WaveTestVolume MATCHES** (src/process/wave.cpp 14/14, `build.py --funcs PROCESS` = **200/206**, `build/scratch/s36/process_wavematch.log`; `.bss` unchanged (46 pre-existing wrong); `make` OK; Wine HD level 1 and digger runs MATCH the baselines, `build/runs/s36-l1`, `s36-dig`). Totals **679/696**: PROCESS 6 misses (Basher 27, Builder 3, Miner 25, AddObjectType 32, AddTrapType 17, BuildCollision 2), LEMMINGS 8 (ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4), RENDER 3 = 17 left. **The fix:** `devs = waveOutGetNumDevs(); switch (n = devs >= 100 ? 100 : devs) {
- - **Session 36 (2026-11-03) APPLIED: ScrollTo 23 -> 18** (`gameview.cpp`: one more pad symbol `pad_scrollto_x0` before it, plus `padplan.py --start 0x406560` for OnHScroll/LevelHeight/... and 92 symbols `pad_setviewscale0..91` before SetViewScale to keep it at 46 (it went to 60), then `padplan --start 0x406810`). EXE still 326/334 (`build/scratch/s36/exe_scroll.log`), `.bss` 0 wrong, `make` OK, Wine HD level 1 + digger MATCH (`s36-l1f`, `s36-digf`); backup `build/scratch/s36/gameview.cpp.before`. Pad sweeps 0-140 over the other near-misses (`sweepall.log`): ApplyOptions 7 at pads 11-13 (not applied: 1 insn, the hoisted `xor edi,edi` and `cmp [g_zoom],esi` remain), BuildRows/AddTrapType/AddOb
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33 declaration climbs (`tools/msvc/search/declclimb.py`, pairwise swaps + move-to-front, pads 0-3/0-6; logs `build/scratch/s33_bp/*_climb.log`): no new matches. Builder::Process 3, DrawPreview 36, DockWnd::Drag 17, ScrollTo 22 (from 23, pad 0, not applied), SetViewScale 45 (from 46, pad 2, not applied), ShowLevelDetails 37, Miner::Process 25, Basher::Process 27, Map::AddTrapType 17. None of these is a declaration-order tie; each needs a source-shape change. WaveTestVolume skipped: the original keeps the clamped volume in esi though it is never used again (a dead value in a callee-saved register), which no declaration order explains.
- - Session 33, split-and-climb batch (new tool `tools/msvc/search/splitclimb.py MODULE SOURCE ADDR [--body F] [--pads A-B] [--apply]`: the FillObjects recipe, i.e. every initialised/mid-block declaration becomes a top declaration plus an assignment in place, then a pairwise declaration climb; compiles every variant under the REAL file name in `build/scratch/splitclimb/<func>/<k>/`, whose baseline reproduces try.sh on the real file, unlike variants.py, which gave Miner 32 instead of 25). Results, best (insns, unmatched in file): ScrollTo (short x body) 21 (pads 0-3), 18 at pad 19 but costs a file match; SetViewScale 46; OpenLevelItem 27; ShowLevelDetails 37; DrawPreview 36; AddTrapType 17; Add
-   - `LEMMINGS.EXE`: **329/334** (+3 matches; only 5 functions remain: `ScrollTo` 18, `SetViewScale` 46, `OpenLevelItem` 27, `DrawPreview` 36, `ShowLevelDetails` 37).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
