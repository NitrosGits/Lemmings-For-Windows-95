# CopyChanged hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **RENDER.DLL**, function at **0x10004af0**, source `src/render/render.c`.
> Status: `0x10004af0 CopyChanged: DIFF (224 bytes) 51 bytes differ, first at +0xc, 13 insns differ`

## Goal
Make `CopyChanged` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh RENDER.DLL src/render/render.c -v -f CopyChanged`) to bytes identical to the original, without breaking the other functions in `render.c` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh RENDER.DLL src/render/render.c -v -f CopyChanged
python3 tools/msvc/realname.py RENDER.DLL src/render/render.c 0x10004af0 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x10004af0: copies the changed cells of the rectangle to the screen, a run of cells at a time */
static void CopyChanged(HDC dst, int dx, int dy, HDC src, int left, int top, int right, int bottom,
                        BYTE *changed, int across, int down, BYTE *bits, BITMAPINFO *bmi)
{
    int cy1;
    BYTE *row;
    int cx0;
    int n;
    int y = 0;
    int cx1;
    int cx;
    int cy;
    int x;
    int cy0;

    cx0 = left >> 4;
    cy1 = (bottom + 7) >> 3;
    cy0 = top >> 3;
    row = changed + across * cy0;
    cx1 = (right + 15) >> 4;
    for (cy = cy0; cy < cy1; cy++) {
        cx = cx0;
        while (cx < cx1) {
            if (row[cx]) {
                n = 16;
                x = (cx - cx0) << 4;
                for (cx++; cx < cx1 && row[cx]; cx++)
                    n += 16;
                BitBlt(dst, x + dx, dy + y, n, 8, src, left + x, top + y, SRCCOPY);
            } else
                for (cx++; cx < cx1 && !row[cx]; cx++)
                    ;
        }
        row += across;
        y += 8;
    }
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     10004af0  mov    eax,DWORD PTR [esp+0x14]               | mov    eax,DWORD PTR [esp+0x14]
     10004af4  sub    esp,0x8                                | sub    esp,0x8
     10004af7  sar    eax,0x4                                | sar    eax,0x4
     10004afa  push   ebx                                    | push   ebx
   * 10004afb  mov    edx,DWORD PTR [esp+0x2c]               | mov    ecx,DWORD PTR [esp+0x24]
     10004aff  push   esi                                    | push   esi
   * 10004b00  push   edi                                    | sar    ecx,0x3
   * 10004b01  xor    esi,esi                                | push   edi
   * 10004b03  push   ebp                                    | mov    ebx,DWORD PTR [esp+0x30]
   * 10004b04  add    edx,0x7                                | push   ebp
   * 10004b07  sar    edx,0x3                                | add    ebx,0xf
   * 10004b0a  mov    ebp,DWORD PTR [esp+0x30]               | mov    edx,DWORD PTR [esp+0x38]
   * 10004b0e  sar    ebp,0x3                                | sar    ebx,0x4
   * 10004b11  mov    ecx,DWORD PTR [esp+0x40]               | add    edx,0x7
   * 10004b15  mov    edi,DWORD PTR [esp+0x3c]               | sar    edx,0x3
   * 10004b19  mov    ebx,DWORD PTR [esp+0x34]               | mov    edi,DWORD PTR [esp+0x3c]
   * 10004b1d  add    ebx,0xf                                | mov    DWORD PTR [esp+0x10],eax
   * 10004b20  mov    DWORD PTR [esp+0x10],eax               | mov    eax,DWORD PTR [esp+0x40]
   * 10004b24  imul   ecx,ebp                                | imul   eax,ecx
   * 10004b27  sar    ebx,0x4                                | add    edi,eax
   * 10004b2a  add    edi,ecx                                | cmp    edx,ecx
   * 10004b2c  cmp    edx,ebp                                | jle    0x10004bc5
   * 10004b2e  jle    0x10004bc5                             | xor    esi,esi
   * 10004b34  sub    edx,ebp                                | sub    edx,ecx
     10004b36  mov    DWORD PTR [esp+0x14],edx               | mov    DWORD PTR [esp+0x14],edx
     10004b3a  mov    ebp,DWORD PTR [esp+0x10]               | mov    ebp,DWORD PTR [esp+0x10]
     10004b3e  cmp    ebx,ebp                                | cmp    ebx,ebp
     10004b40  jle    0x10004bb4                             | jle    0x10004bb4
     10004b42  cmp    BYTE PTR [ebp+edi*1+0x0],0x0           | cmp    BYTE PTR [ebp+edi*1+0x0],0x0
     10004b47  je     0x10004b9f                             | je     0x10004b9f
     10004b49  mov    eax,0x10                               | mov    eax,0x10
     10004b4e  mov    ecx,ebp                                | mov    ecx,ebp
     10004b50  sub    ecx,DWORD PTR [esp+0x10]               | sub    ecx,DWORD PTR [esp+0x10]
     10004b54  inc    ebp                                    | inc    ebp
     10004b55  shl    ecx,0x4                                | shl    ecx,0x4
     10004b58  cmp    ebp,ebx                                | cmp    ebp,ebx
     10004b5a  jge    0x10004b6b                             | jge    0x10004b6b
     10004b5c  cmp    BYTE PTR [ebp+edi*1+0x0],0x0           | cmp    BYTE PTR [ebp+edi*1+0x0],0x0
     10004b61  je     0x10004b6b                             | je     0x10004b6b
     10004b63  add    eax,0x10                               | add    eax,0x10
     10004b66  inc    ebp                                    | inc    ebp
     10004b67  cmp    ebp,ebx                                | cmp    ebp,ebx
     10004b69  jl     0x10004b5c                             | jl     0x10004b5c
     10004b6b  mov    edx,DWORD PTR [esp+0x30]               | mov    edx,DWORD PTR [esp+0x30]
     10004b6f  push   0xcc0020                               | push   0xcc0020
     10004b74  add    edx,esi                                | add    edx,esi
     10004b76  push   edx                                    | push   edx
     10004b77  mov    edx,DWORD PTR [esp+0x34]               | mov    edx,DWORD PTR [esp+0x34]
     10004b7b  add    edx,ecx                                | add    edx,ecx
     10004b7d  push   edx                                    | push   edx
     10004b7e  mov    edx,DWORD PTR [esp+0x34]               | mov    edx,DWORD PTR [esp+0x34]
     10004b82  add    ecx,DWORD PTR [esp+0x2c]               | add    ecx,DWORD PTR [esp+0x2c]
     10004b86  push   edx                                    | push   edx
     10004b87  push   0x8                                    | push   0x8
     10004b89  push   eax                                    | push   eax
     10004b8a  mov    eax,DWORD PTR [esp+0x3c]               | mov    eax,DWORD PTR [esp+0x3c]
     10004b8e  add    eax,esi                                | add    eax,esi
     10004b90  push   eax                                    | push   eax
     10004b91  push   ecx                                    | push   ecx
     10004b92  mov    eax,DWORD PTR [esp+0x3c]               | mov    eax,DWORD PTR [esp+0x3c]
     10004b96  push   eax                                    | push   eax
     10004b97  call   DWORD PTR ds:0x10016158                | call   DWORD PTR ds:0x10016158
     10004b9d  jmp    0x10004bb0                             | jmp    0x10004bb0
     10004b9f  inc    ebp                                    | inc    ebp
     10004ba0  cmp    ebp,ebx                                | cmp    ebp,ebx
     10004ba2  jge    0x10004bb4                             | jge    0x10004bb4
     10004ba4  cmp    BYTE PTR [ebp+edi*1+0x0],0x0           | cmp    BYTE PTR [ebp+edi*1+0x0],0x0
     10004ba9  jne    0x10004bb0                             | jne    0x10004bb0
     10004bab  inc    ebp                                    | inc    ebp
     10004bac  cmp    ebp,ebx                                | cmp    ebp,ebx
     10004bae  jl     0x10004ba4                             | jl     0x10004ba4
     10004bb0  cmp    ebp,ebx                                | cmp    ebp,ebx
     10004bb2  jl     0x10004b42                             | jl     0x10004b42
     10004bb4  add    edi,DWORD PTR [esp+0x40]               | add    edi,DWORD PTR [esp+0x40]
     10004bb8  add    esi,0x8                                | add    esi,0x8
     10004bbb  dec    DWORD PTR [esp+0x14]                   | dec    DWORD PTR [esp+0x14]
     10004bbf  jne    0x10004b3a                             | jne    0x10004b3a
     10004bc5  pop    ebp                                    | pop    ebp
     10004bc6  pop    edi                                    | pop    edi
     10004bc7  pop    esi                                    | pop    esi
     10004bc8  pop    ebx                                    | pop    ebx
     10004bc9  add    esp,0x8                                | add    esp,0x8
     10004bcc  ret                                           | ret
     10004bcd  lea    ecx,[ecx+0x0]                          | lea    ecx,[ecx+0x0]
```

## What HANDOFF.md already records (most recent last)
- - **Session 36 (2026-11-03) late: new tool + sister check.** `tools/msvc/search/permute.py MODULE SOURCE ADDR` = random semantic-preserving mutation hill-climb (decl swaps, adjacent call-free statement swaps, comparison flips, `A+B`/`A*B` operand swaps, `x += k` <-> `x = x + k`) scored on a TRUNCATED translation unit (`build/scratch/s36/tu1/render.c` = render.c cut before 0x100013a0 with `static` removed from prototypes, `tucs/render.c` = cut before 0x10004d50; ~1.6 s per compile instead of ~7 s). First CopyChanged run (about 3,000 candidates, seed 101): stays 13. Sister project (`/Users/daniel/Decomp-dev/lemmings-lemmings`, checked 11-03 evening): 349 'Match' commits total, 89 since 11-01 (
- - Session 31: StretchChanged with sy as function-static, plain local, or file global (3 forms, pads 0-15): 32-44, far worse than the volatile local (3); volatile stays. Running: Renderer_Render greedy setup-statement order (`s30_mdib/rr_greedy.log`), then 300 random decl orders each for Renderer_Render and CopyChanged (`s30_mdib/rd_rr.log`, `rd_cc.log`).
- - Session 31: Renderer_Render 7 -> 5 (applied): setup order now c, dx = 0, ... (greedy over the 6 setup lines, `s30_mdib/rr_greedy.log`); RENDER 74/77 intact; make OK. 300 random decl orders (old body) and CopyChanged 300 decl orders: no gain. levelsel joint padding (600): current is best. Backup `build/scratch/s30_mdib/render.c.before3`.
- - Session 31: Renderer_Render further greedy blocks (changed/across/down order, dx/dy if-body orders, r/s order): stays 5; 300 decl orders on the new body: stays 5. CopyChanged without bits/bmi parameters: wrong - the original Renderer_Render pushes them (function shrinks 896->864 and 30 functions shift), so the parameters are real. Not applied.
- - Optimisation-mode probe (`tools/msvc/optmode.py`, all modes incl. size) on Renderer_Render, CopyChanged, StretchChanged: every mode worse or equal; all three are /Ox functions. `#pragma optimize("l"/"p")` do not exist in VC 2.2 (no effect).
- - **CopyChanged (17 in production):** the original has StretchChanged's structure (cx0, cy0, cx1, cy1, `row = changed + across * cy0`, `if (cy0 < cy1) { y = 0; rows = cy1 - cy0; do {...} while (--rows); }`). Unused leftover declarations (sx/w/h/sy) change nothing. Statement order decides the loop's registers: order `cy0; row; cx1; cx0; cy1` gives the original's loop registers (row=edi, cx=ebp, cx1=ebx, y=esi) and with **27 pad lines the whole loop incl. `[ebp+edi]` matches: 15 insns left, all in the setup** (`build/scratch/s31_render/cc40.txt`, `render_cc40.c`). Running: cx0-first statement orders x pads 0-64 (`cc4.log`).
- - RENDER deep dive, continued: CopyChanged with the original statement order: decl-order permutations of the 4 loop vars x pads 0-40 (2952) best 26; declaration hill-climb (new tool `tools/msvc/search/declclimb.py`) 22 at pad 27; split `row = changed; ... row += across * cy0;` in every placement + decl-initialiser form x pads 0-40 (1066) best 24. The `cy0; row; cx1; cx0; cy1` order + 27 pads (15, loop fully matching) remains the best scratch result. Renderer_Render: hill-climb from the plain order stays 7 (production 5 kept). StretchChanged: sx/w/h volatile (5), address-taken sy (21) worse; statement-order greedy stays 3.
- - CopyChanged register study (`build/scratch/s31_render/cc_regs.py`/`.log`, all 120 statement orders at pad 0): `row` gets the original's edi ONLY when the first two statements are `cy0 = ...; row = ...;`; every order starting with cx0 or cy1 gives row=esi. The original computes cx0 first, so testing combined forms `row = changed + across * (cy0 = top >> 3);` in all positions x pads 0-40 (`cc8.txt`). Declaration climbs from the best (15, pad 27) and cross searches confirm 15 is the floor of the separate-statement forms. Renderer_Render declaration climb from production: stays 5. StretchChanged: `for` loops give the same counter code as the original (counter in cy0's slot) but keep the early 
-   - Common pattern in all three misses: the original puts a short-lived expression temp in **eax** (StretchChanged and CopyChanged: the `cy0 * across` row product; Renderer_Render: `left & 15`, with the dx/dy zero in ecx) where our VC 2.2 build picks ecx/edx. Ruled out as causes: optimisation pragmas (all modes), build flags (/Zd /Z7 /Zi /Gd /J same; /Gf /Gy /Ge /Gh worse), the VC 2.0 compiler (same results, 34/77 overall), unsigned/DWORD/long types, struct/array/address-taken/global/static forms of sy, extra volatiles, local names (no name-hash effect), unused leftover declarations.
-   - CopyChanged: structure = StretchChanged's (`cx0; cy0; cx1; cy1; row = changed + across * cy0; if (cy0 < cy1) { y = 0; rows = cy1 - cy0; do {...} while (--rows); }`); loop registers right only when `cy0; row` are the first two statements (`cc_regs.log`); best scratch 15 (`cc40.txt` + 27 pads; loop fully matching, setup differs). Not applied (needs the following functions re-padded for a fuzzy-only gain).
- - Session 33, CopyChanged insight: in the original the product goes to eax because it is *forced*: cy0=ecx and cy1=edx are register variables and eax holds cx0's pending store (flushed just before the product). With the natural order `cx0; cy0; cx1; cy1; row` ours gives cy0=edx, cy1=ebp, row=esi, y=ebx, product ecx (31). So CopyChanged is a global-allocator problem (get cy1 into edx, not a callee-saved register), not a product-spelling problem. Lab (`build/scratch/s33_sc/lab/l1.c`): volatile/plain/param/pointer/global/address-taken/struct/no-division sy all give the product in ecx when ecx is free, so StretchChanged's original probably also had ecx/edx occupied there. Nothing found yet that 
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33 real-name climbs (splitclimb.py, pads 0-3): StretchChanged 3 (same as production), CopyChanged production 17, natural-order 24, cc40 order 19, Renderer_Render split body 13 (production 5). ApplyOptions late-zero body, pads 0-40: 22 (swap). No match. The earlier variants.py results for RENDER were not wrong-context artefacts.
-   - `RENDER.DLL`: **75/77** (+1 match; only 2 functions remain: `Renderer_Render` 5, `CopyChanged` 13).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
