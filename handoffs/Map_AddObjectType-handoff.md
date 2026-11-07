# Map::AddObjectType hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **PROCESS.DLL**, function at **0x10004760**, source `src/process/map.cpp`.
> Status: `0x10004760 Map::AddObjectType: DIFF (288 bytes) 202 bytes differ, first at +0x2f, 32 insns differ; +0x6b ?g_scale@@3HA: original 0xc10366c0, expected 0x100112d8; +0x77 ?g_scale@@3HA: original 0xf661001, expected 0x100112d8`

## Goal
Make `Map::AddObjectType` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh PROCESS.DLL src/process/map.cpp -v -f Map::AddObjectType`) to bytes identical to the original, without breaking the other functions in `map.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh PROCESS.DLL src/process/map.cpp -v -f Map::AddObjectType
python3 tools/msvc/realname.py PROCESS.DLL src/process/map.cpp 0x10004760 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x10004760: a kind of object, its frames side by side in a slot */
int Map::AddObjectType(WORD renderer, WORD slot, int x, int y, int w, int h, int count)
{
    SPRITELIST *list;
    int i;
    int n = numTypes;
    OBJTYPE *t = &types[n];
    int fx;
    int current;

    list = NewSpriteList(count);
    list->slot = slot;
    list->count = (WORD)count;
    for (i = 0, fx = x; i < count; i++, fx = current + (short)w) {
        list->frames[i].x = (WORD)((current = (short)fx) * g_scale);
        list->frames[i].y = (WORD)(y * g_scale);
        list->frames[i].width = (WORD)(w * g_scale);
        list->frames[i].height = (WORD)(h * g_scale);
    }
    t->width = w;
    t->height = h;
    t->count = count;
    t->list = Renderer_SpriteList(renderer, list);
    t->frames = GetGraphics(renderer, slot, x, y, w, h, count);
    FreeSpriteList(list);
    numTypes++;
    return n;
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     10004760  sub    esp,0xc                                | sub    esp,0xc
     10004763  mov    DWORD PTR [esp+0x4],ecx                | mov    DWORD PTR [esp+0x4],ecx
     10004767  push   ebx                                    | push   ebx
     10004768  push   esi                                    | push   esi
     10004769  mov    ecx,DWORD PTR [ecx+0xd8]               | mov    ecx,DWORD PTR [ecx+0xd8]
     1000476f  mov    eax,DWORD PTR [esp+0xc]                | mov    eax,DWORD PTR [esp+0xc]
     10004773  push   edi                                    | push   edi
     10004774  mov    DWORD PTR [esp+0x14],ecx               | mov    DWORD PTR [esp+0x14],ecx
     10004778  push   ebp                                    | push   ebp
     10004779  shl    ecx,0x2                                | shl    ecx,0x2
     1000477c  lea    edx,[ecx+ecx*4]                        | lea    edx,[ecx+ecx*4]
     1000477f  mov    ecx,DWORD PTR [esp+0x38]               | mov    ecx,DWORD PTR [esp+0x38]
     10004783  push   ecx                                    | push   ecx
     10004784  lea    esi,[edx+eax*1+0x10]                   | lea    esi,[edx+eax*1+0x10]
     10004788  call   0x10008d32                             | call   0x10008d32
   * 1000478d  mov    dx,WORD PTR [esp+0x38]                 | mov    cx,WORD PTR [esp+0x38]
     10004792  mov    DWORD PTR [esp+0x10],eax               | mov    DWORD PTR [esp+0x10],eax
   * 10004796  mov    ax,WORD PTR [esp+0x24]                 | cmp    DWORD PTR [esp+0x38],0x0
   * 1000479b  mov    ecx,DWORD PTR [esp+0x10]               | mov    dx,WORD PTR [esp+0x24]
   * 1000479f  cmp    DWORD PTR [esp+0x38],0x0               | mov    WORD PTR [eax],cx
   * 100047a4  mov    WORD PTR [ecx+0x2],ax                  | mov    WORD PTR [eax+0x2],dx
   * 100047a8  mov    WORD PTR [ecx],dx                      | jle    0x10004806
   * 100047ab  mov    edx,DWORD PTR [esp+0x28]               | lea    edx,[eax+0x6]
   * 100047af  jle    0x10004808                             | mov    edi,DWORD PTR [esp+0x30]
   * 100047b1  lea    eax,[ecx+0x6]                          | mov    ax,WORD PTR [esp+0x28]
   * 100047b4  mov    ebx,DWORD PTR [esp+0x30]               | mov    ebx,DWORD PTR [esp+0x38]
   * 100047b8  movsx  edi,bx                                 | movsx  ecx,di
   * 100047bb  mov    ecx,DWORD PTR [esp+0x38]               | mov    ebp,DWORD PTR ds:0x100112d8
   * 100047bf  movsx  edx,dx                                 | add    edx,0x8
   * 100047c2  mov    ebp,edx                                | imul   bp,ax
   * 100047c4  add    eax,0x8                                | movsx  eax,ax
   * 100047c7  imul   bp,WORD PTR ds:0x100112d8              | add    ax,cx
   * 100047ce                                                | mov    WORD PTR [edx-0x8],bp
   * 100047cf  mov    WORD PTR [eax-0x8],bp                  | mov    ebp,DWORD PTR ds:0x100112d8
   * 100047d3  add    edx,edi                                | imul   bp,WORD PTR [esp+0x2c]
   * 100047d5  mov    ebp,DWORD PTR ds:0x100112d8            | mov    WORD PTR [edx-0x6],bp
   * 100047db  imul   bp,WORD PTR [esp+0x2c]                 | mov    ebp,DWORD PTR ds:0x100112d8
   * 100047e1  mov    WORD PTR [eax-0x6],bp                  | imul   bp,di
   * 100047e5  mov    ebp,DWORD PTR ds:0x100112d8            | mov    WORD PTR [edx-0x4],bp
   * 100047eb  imul   bp,bx                                  | mov    ebp,DWORD PTR ds:0x100112d8
   * 100047ef  mov    WORD PTR [eax-0x4],bp                  | imul   bp,WORD PTR [esp+0x34]
   * 100047f3  mov    ebp,DWORD PTR ds:0x100112d8            | mov    WORD PTR [edx-0x2],bp
   * 100047f9  imul   bp,WORD PTR [esp+0x34]                 | dec    ebx
   * 100047ff  mov    WORD PTR [eax-0x2],bp                  | jne    0x100047bc
   * 10004803  dec    ecx                                    | jmp    0x1000480a
   * 10004804  jne    0x100047bf                             | mov    edi,DWORD PTR [esp+0x30]
   * 10004806  jmp    0x1000480c                             | mov    eax,DWORD PTR [esp+0x34]
   * 10004808  mov    ebx,DWORD PTR [esp+0x30]               | mov    ecx,DWORD PTR [esp+0x38]
   * 1000480c  mov    eax,DWORD PTR [esp+0x34]               | mov    bx,WORD PTR [esp+0x20]
   * 10004810  mov    ecx,DWORD PTR [esp+0x38]               | mov    edx,DWORD PTR [esp+0x10]
   * 10004814  mov    di,WORD PTR [esp+0x20]                 | push   edx
   * 10004819  mov    edx,DWORD PTR [esp+0x10]               | mov    DWORD PTR [esi+0x8],edi
   * 1000481d  push   edx                                    | push   ebx
   * 1000481e  mov    DWORD PTR [esi+0x8],ebx                | mov    DWORD PTR [esi+0xc],eax
   * 10004821  push   edi                                    | mov    DWORD PTR [esi+0x4],ecx
   * 10004822  mov    DWORD PTR [esi+0xc],eax                | call   0x10008d2c
   * 10004825  mov    DWORD PTR [esi+0x4],ecx                | mov    WORD PTR [esi],ax
   * 10004828  call   0x9966482d                             | mov    ecx,DWORD PTR [esp+0x38]
   * 1000482d  mov    WORD PTR [esi],ax                      | mov    eax,DWORD PTR [esp+0x34]
   * 10004830  mov    ecx,DWORD PTR [esp+0x38]               | push   ecx
   * 10004834  mov    eax,DWORD PTR [esp+0x34]               | mov    edx,DWORD PTR [esp+0x30]
   * 10004838  push   ecx                                    | push   eax
   * 10004839  mov    edx,DWORD PTR [esp+0x30]               | mov    ecx,DWORD PTR [esp+0x30]
   * 1000483d  push   eax                                    | push   edi
   * 1000483e  mov    ecx,DWORD PTR [esp+0x30]               | mov    eax,DWORD PTR [esp+0x30]
   * 10004842  push   ebx                                    | push   edx
   * 10004843  mov    eax,DWORD PTR [esp+0x30]               | push   ecx
   * 10004847  push   edx                                    | push   eax
   * 10004848  push   ecx                                    | push   ebx
   * 10004849  push   eax                                    | call   0x100046d0
   * 1000484a  push   edi                                    | mov    ecx,DWORD PTR [esp+0x2c]
   * 1000484b  call   0x100046d0                             | add    esp,0x1c
   * 10004850  mov    ecx,DWORD PTR [esp+0x2c]               | mov    DWORD PTR [esi+0x10],eax
   * 10004854  add    esp,0x1c                               | push   ecx
   * 10004857  mov    DWORD PTR [esi+0x10],eax               | call   0x10008d26
   * 1000485a  push   ecx                                    | mov    ecx,DWORD PTR [esp+0x14]
   * 1000485b  call   0x5c8b4860                             | mov    eax,DWORD PTR [esp+0x18]
   * 10004860  mov    ecx,DWORD PTR [esp+0x14]               | pop    ebp
   * 10004864  mov    eax,DWORD PTR [esp+0x18]               | pop    edi
   * 10004868  pop    ebp                                    | pop    esi
   * 10004869  pop    edi                                    | inc    DWORD PTR [ecx+0xd8]
   * 1000486a  pop    esi                                    | pop    ebx
   * 1000486b  inc    DWORD PTR [ecx+0xd8]                   | add    esp,0xc
   * 10004871  pop    ebx                                    | ret    0x1c
   * 10004872  add    esp,0xc                                | lea    ecx,[ecx+0x0]
   * 10004875  ret    0x1c                                   | lea    esp,[esp+0x0]
   * 10004878  lea    esp,[esp+0x0]                          | sub    esp,0xc
   * 1000487c  lea    esp,[esp+0x0]                          | mov    DWORD PTR [esp+0x4],ecx
```

## What HANDOFF.md already records (most recent last)
- - **General lesson (try on other register-tie misses: AddTrapType/AddObjectType, Miner/Basher::Process, DrawLemmings, BuildRows...):** wrap individual repeated expression statements in `{ T tmp = expr; use(tmp); }` blocks, one position at a time (2^n sweep), instead of only reordering declarations.
- ### Session 32: block-temp sweep on Map::AddTrapType / AddObjectType (not applied)
- - Sweeping `{ WORD v = expr; list->frames[i].f = v; }` over the 6 statements (63 variants each, specs `build/scratch/s32/at_spec.py`, `ao3_spec.py`): **AddTrapType 17 -> 11** (best: all four frame stores wrapped, `build/scratch/s32/at_c15.cpp`), **AddObjectType 21 -> 14** (best combos c31/c47/c55/c59/c61/c62). Neither is exact, and each drops map.cpp from 19/23 to 10-11 until padplan re-plans the file (not done, so live source unchanged).
- - So the block-temp sweep helps where a repeated store sequence fixes the byte-register pattern (BuildCollision) or frame-store expressions (AddTrapType/AddObjectType 17->11 / 21->14), but not these switch-heavy functions.
- - Remaining: PROCESS Basher 27, Builder 3, Miner 25, AddObjectType 32 (mid-experiment; was 21), AddTrapType 17, BuildCollision 2, WaveTestVolume 13; EXE ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4; RENDER Renderer_Render, CopyChanged, StretchChanged.
- - **Session 36 end state (verified 2026-11-02):** **WaveTestVolume MATCHES** (src/process/wave.cpp 14/14, `build.py --funcs PROCESS` = **200/206**, `build/scratch/s36/process_wavematch.log`; `.bss` unchanged (46 pre-existing wrong); `make` OK; Wine HD level 1 and digger runs MATCH the baselines, `build/runs/s36-l1`, `s36-dig`). Totals **679/696**: PROCESS 6 misses (Basher 27, Builder 3, Miner 25, AddObjectType 32, AddTrapType 17, BuildCollision 2), LEMMINGS 8 (ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4), RENDER 3 = 17 left. **The fix:** `devs = waveOutGetNumDevs(); switch (n = devs >= 100 ? 100 : devs) {
- - **Session 36, AddTrapType/AddObjectType (17 / 32, unchanged):** both share the same two residuals. (1) the original schedules `cmp [count],0` BEFORE the `list->count`/`list->slot` stores (ours after); (2) the original's fx update is 16-bit (`movsx eax,ax; add ax,cx`, fx in eax, pointer in edx, counter `dec edi`) while ours is `add eax,edi` (32-bit). Tried: `short`/`WORD`/`unsigned short` fx x 5 update spellings (30-46 insns, fx then lives in dx/si), `(short)(fx + w)`, `fx + (short)w`, `(short)fx + w`, double casts, `(short)x` init (16, but shifts the rest of map.cpp: file 15/23 vs 20), `(short)` casts on the x store and on g_scale, count/slot store order. WaveTestVolume's lesson (expressio
-   * **Lab on the AddTrapType/AddObjectType fx update** (`build/scratch/s36/lab/`): with `int fx` every spelling gives `movsx eax,ax; add eax,edx` (32-bit); `fx = (short)((short)fx + (short)w)` gives a 16-bit `add ax,si` but with the `movsx` AFTER the add, the original has it before; `short`/`WORD` fx drop the movsx entirely. No spelling reproduces `movsx eax,ax; add ax,cx`.
- - **Session 36 (2026-11-03) APPLIED: ScrollTo 23 -> 18** (`gameview.cpp`: one more pad symbol `pad_scrollto_x0` before it, plus `padplan.py --start 0x406560` for OnHScroll/LevelHeight/... and 92 symbols `pad_setviewscale0..91` before SetViewScale to keep it at 46 (it went to 60), then `padplan --start 0x406810`). EXE still 326/334 (`build/scratch/s36/exe_scroll.log`), `.bss` 0 wrong, `make` OK, Wine HD level 1 + digger MATCH (`s36-l1f`, `s36-digf`); backup `build/scratch/s36/gameview.cpp.before`. Pad sweeps 0-140 over the other near-misses (`sweepall.log`): ApplyOptions 7 at pads 11-13 (not applied: 1 insn, the hoisted `xor edi,edi` and `cmp [g_zoom],esi` remain), BuildRows/AddTrapType/AddOb
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - **Session 33: Map::FillObjects MATCHED (PROCESS 199/206, total 676/696).** All locals declared at the top in the order `int i; int y1; int x0; int y0; OBJGRID *g = grid; int x1; int j;` (x1/y1 assigned as statements), plus 3 `pad_fillobjects` lines. Found by `tools/msvc/search/declclimb.py` in round 0 (body `build/scratch/s33_bp/fo_top.txt`). The compare-operand orders follow declaration order, as in MarkChanged. Side effect: AddObjectType (unmatched, earlier in map.cpp) went from 21 to 32 insns; no match was lost. Verified: `build.py --funcs PROCESS` 199/206 (`build/scratch/s33_bp/proc_fo.log`), `make`, muted HD Wine `s33b-l1` and `s33b-dig` MATCH. Backup `build/scratch/s33_bp/pmap.cpp.be
- - Session 33, inline/padding hunt (no new match): `Message` (lemming.h, a plain inline function, not a class member) dummy locals 0-8 x padding for Miner (pad_top 0-40) and Basher (pad_basherprocess 0-40): no effect (hook removed again, lemming.h unchanged). End-of-file padding (file-wide symbol count) does not move non-inline functions (Builder, WaveTestVolume, Miner, Basher, AddObjectType all unchanged), so the count effect is specific to class-member inlines. pad_top 0-64 for AddObjectType/AddTrapType: no gain. BuildCollision with all declarations at the top + declclimb: 2. Renderer_Render: 8 best setup orders x pads 0-64 and 5 canvas-pointer spellings: 5.
- - Session 33, split-and-climb batch (new tool `tools/msvc/search/splitclimb.py MODULE SOURCE ADDR [--body F] [--pads A-B] [--apply]`: the FillObjects recipe, i.e. every initialised/mid-block declaration becomes a top declaration plus an assignment in place, then a pairwise declaration climb; compiles every variant under the REAL file name in `build/scratch/splitclimb/<func>/<k>/`, whose baseline reproduces try.sh on the real file, unlike variants.py, which gave Miner 32 instead of 25). Results, best (insns, unmatched in file): ScrollTo (short x body) 21 (pads 0-3), 18 at pad 19 but costs a file match; SetViewScale 46; OpenLevelItem 27; ShowLevelDetails 37; DrawPreview 36; AddTrapType 17; Add
-   - `PROCESS.DLL`: **201/206** (+1 match; only 5 functions remain: `Basher::Process` 27, `Builder::Process` 3, `Miner::Process` 25, `Map::AddObjectType` 32, `Map::AddTrapType` 17).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
