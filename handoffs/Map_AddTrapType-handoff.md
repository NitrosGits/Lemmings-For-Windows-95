# Map::AddTrapType hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **PROCESS.DLL**, function at **0x10004880**, source `src/process/map.cpp`.
> Status: `0x10004880 Map::AddTrapType: DIFF (288 bytes) 201 bytes differ, first at +0x36, 17 insns differ; +0x61 ?g_scale@@3HA: original 0x112d82d, expected 0x100112d8; +0x77 ?g_scale@@3HA: original 0x12d82d8b, expected 0x100112d8`

## Goal
Make `Map::AddTrapType` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh PROCESS.DLL src/process/map.cpp -v -f Map::AddTrapType`) to bytes identical to the original, without breaking the other functions in `map.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh PROCESS.DLL src/process/map.cpp -v -f Map::AddTrapType
python3 tools/msvc/realname.py PROCESS.DLL src/process/map.cpp 0x10004880 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x10004880 */
int Map::AddTrapType(WORD renderer, WORD slot, int x, int y, int w, int h, int count)
{
    SPRITELIST *list;
    int i;
    int n = numTypes;
    OBJTYPE *t = &types[n];
    int fx;

    list = NewSpriteList(count);
    list->count = (WORD)count;
    list->slot = slot;
    for (i = 0, fx = x; i < count; i++, fx = (short)fx + (short)w) {
        list->frames[i].x = (WORD)(fx * g_scale);
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
     10004880  sub    esp,0xc                                | sub    esp,0xc
     10004883  mov    DWORD PTR [esp+0x4],ecx                | mov    DWORD PTR [esp+0x4],ecx
     10004887  push   ebx                                    | push   ebx
     10004888  push   esi                                    | push   esi
     10004889  mov    ecx,DWORD PTR [ecx+0xd8]               | mov    ecx,DWORD PTR [ecx+0xd8]
     1000488f  mov    eax,DWORD PTR [esp+0xc]                | mov    eax,DWORD PTR [esp+0xc]
     10004893  push   edi                                    | push   edi
     10004894  mov    DWORD PTR [esp+0x14],ecx               | mov    DWORD PTR [esp+0x14],ecx
     10004898  push   ebp                                    | push   ebp
     10004899  shl    ecx,0x2                                | shl    ecx,0x2
     1000489c  lea    edx,[ecx+ecx*4]                        | lea    edx,[ecx+ecx*4]
     1000489f  mov    ecx,DWORD PTR [esp+0x38]               | mov    ecx,DWORD PTR [esp+0x38]
     100048a3  push   ecx                                    | push   ecx
     100048a4  lea    esi,[edx+eax*1+0x10]                   | lea    esi,[edx+eax*1+0x10]
     100048a8  call   0x10008d32                             | call   0x10008d32
     100048ad  mov    dx,WORD PTR [esp+0x24]                 | mov    dx,WORD PTR [esp+0x24]
     100048b2  mov    DWORD PTR [esp+0x10],eax               | mov    DWORD PTR [esp+0x10],eax
   * 100048b6  mov    ax,WORD PTR [esp+0x38]                 | cmp    DWORD PTR [esp+0x38],0x0
   * 100048bb  mov    ecx,DWORD PTR [esp+0x10]               | mov    ax,WORD PTR [esp+0x38]
   * 100048bf  cmp    DWORD PTR [esp+0x38],0x0               | mov    ecx,DWORD PTR [esp+0x10]
     100048c4  mov    WORD PTR [ecx],ax                      | mov    WORD PTR [ecx],ax
   * 100048c7  mov    eax,DWORD PTR [esp+0x28]               | mov    WORD PTR [ecx+0x2],dx
   * 100048cb  mov    WORD PTR [ecx+0x2],dx                  | jle    0x1000492a
   * 100048cf  jle    0x10004928                             | mov    ax,WORD PTR [esp+0x28]
   * 100048d1  add    ecx,0x6                                | lea    edx,[ecx+0x6]
     100048d4  mov    ebx,DWORD PTR [esp+0x30]               | mov    ebx,DWORD PTR [esp+0x30]
   * 100048d8  movsx  edi,bx                                 | mov    edi,DWORD PTR [esp+0x38]
   * 100048db  mov    edx,DWORD PTR [esp+0x38]               | movsx  ecx,bx
     100048df  mov    ebp,DWORD PTR ds:0x100112d8            | mov    ebp,DWORD PTR ds:0x100112d8
   * 100048e5  add    ecx,0x8                                | add    edx,0x8
     100048e8  imul   bp,ax                                  | imul   bp,ax
     100048ec  movsx  eax,ax                                 | movsx  eax,ax
   * 100048ef  mov    WORD PTR [ecx-0x8],bp                  | add    ax,cx
   * 100048f3  add    eax,edi                                | mov    WORD PTR [edx-0x8],bp
     100048f5  mov    ebp,DWORD PTR ds:0x100112d8            | mov    ebp,DWORD PTR ds:0x100112d8
     100048fb  imul   bp,WORD PTR [esp+0x2c]                 | imul   bp,WORD PTR [esp+0x2c]
   * 10004901  mov    WORD PTR [ecx-0x6],bp                  | mov    WORD PTR [edx-0x6],bp
     10004905  mov    ebp,DWORD PTR ds:0x100112d8            | mov    ebp,DWORD PTR ds:0x100112d8
     1000490b  imul   bp,bx                                  | imul   bp,bx
   * 1000490f  mov    WORD PTR [ecx-0x4],bp                  | mov    WORD PTR [edx-0x4],bp
     10004913  mov    ebp,DWORD PTR ds:0x100112d8            | mov    ebp,DWORD PTR ds:0x100112d8
     10004919  imul   bp,WORD PTR [esp+0x34]                 | imul   bp,WORD PTR [esp+0x34]
   * 1000491f  mov    WORD PTR [ecx-0x2],bp                  | mov    WORD PTR [edx-0x2],bp
   * 10004923  dec    edx                                    | dec    edi
   * 10004924  jne    0x100048df                             | jne    0x100048e0
   * 10004926  jmp    0x1000492c                             | jmp    0x1000492e
     10004928  mov    ebx,DWORD PTR [esp+0x30]               | mov    ebx,DWORD PTR [esp+0x30]
     1000492c  mov    eax,DWORD PTR [esp+0x34]               | mov    eax,DWORD PTR [esp+0x34]
     10004930  mov    ecx,DWORD PTR [esp+0x38]               | mov    ecx,DWORD PTR [esp+0x38]
     10004934  mov    di,WORD PTR [esp+0x20]                 | mov    di,WORD PTR [esp+0x20]
     10004939  mov    edx,DWORD PTR [esp+0x10]               | mov    edx,DWORD PTR [esp+0x10]
     1000493d  push   edx                                    | push   edx
     1000493e  mov    DWORD PTR [esi+0x8],ebx                | mov    DWORD PTR [esi+0x8],ebx
     10004941  push   edi                                    | push   edi
     10004942  mov    DWORD PTR [esi+0xc],eax                | mov    DWORD PTR [esi+0xc],eax
     10004945  mov    DWORD PTR [esi+0x4],ecx                | mov    DWORD PTR [esi+0x4],ecx
   * 10004948  call   0x53de3151                             | call   0x10008d2c
     1000494d  mov    WORD PTR [esi],ax                      | mov    WORD PTR [esi],ax
     10004950  mov    ecx,DWORD PTR [esp+0x38]               | mov    ecx,DWORD PTR [esp+0x38]
     10004954  mov    eax,DWORD PTR [esp+0x34]               | mov    eax,DWORD PTR [esp+0x34]
     10004958  push   ecx                                    | push   ecx
     10004959  mov    edx,DWORD PTR [esp+0x30]               | mov    edx,DWORD PTR [esp+0x30]
     1000495d  push   eax                                    | push   eax
     1000495e  mov    ecx,DWORD PTR [esp+0x30]               | mov    ecx,DWORD PTR [esp+0x30]
     10004962  push   ebx                                    | push   ebx
     10004963  mov    eax,DWORD PTR [esp+0x30]               | mov    eax,DWORD PTR [esp+0x30]
     10004967  push   edx                                    | push   edx
     10004968  push   ecx                                    | push   ecx
     10004969  push   eax                                    | push   eax
     1000496a  push   edi                                    | push   edi
     1000496b  call   0x100046d0                             | call   0x100046d0
     10004970  mov    ecx,DWORD PTR [esp+0x2c]               | mov    ecx,DWORD PTR [esp+0x2c]
     10004974  add    esp,0x1c                               | add    esp,0x1c
     10004977  mov    DWORD PTR [esi+0x10],eax               | mov    DWORD PTR [esi+0x10],eax
     1000497a  push   ecx                                    | push   ecx
   * 1000497b  call   0x53a531d1                             | call   0x10008d26
     10004980  mov    ecx,DWORD PTR [esp+0x14]               | mov    ecx,DWORD PTR [esp+0x14]
     10004984  mov    eax,DWORD PTR [esp+0x18]               | mov    eax,DWORD PTR [esp+0x18]
     10004988  pop    ebp                                    | pop    ebp
     10004989  pop    edi                                    | pop    edi
     1000498a  pop    esi                                    | pop    esi
     1000498b  inc    DWORD PTR [ecx+0xd8]                   | inc    DWORD PTR [ecx+0xd8]
     10004991  pop    ebx                                    | pop    ebx
     10004992  add    esp,0xc                                | add    esp,0xc
     10004995  ret    0x1c                                   | ret    0x1c
   * 10004998  lea    esp,[esp+0x0]                          | lea    ebx,[ebx+0x0]
   * 1000499c  lea    esp,[esp+0x0]                          | mov    edx,DWORD PTR [esp+0x4]
```

## What HANDOFF.md already records (most recent last)
- - Loop-form sweep done (`at4_spec.py`: do-while with guard, while, pointer-advance `f++` x3 forms, update-before/after, each plain and block-temp): best stays 11 (while/for variants compile to the same code as the block-temp `for`; pointer forms are no better). The 16-bit `ax` induction variable in the original is not reached by loop shape either. Remaining untried ideas for AddTrapType: make `fx`/`y` WORD-typed *parameters* (changing the signature: `short x, short y` etc.), or pass them through a local struct.
- - So the block-temp sweep helps where a repeated store sequence fixes the byte-register pattern (BuildCollision) or frame-store expressions (AddTrapType/AddObjectType 17->11 / 21->14), but not these switch-heavy functions.
- - Remaining: PROCESS Basher 27, Builder 3, Miner 25, AddObjectType 32 (mid-experiment; was 21), AddTrapType 17, BuildCollision 2, WaveTestVolume 13; EXE ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4; RENDER Renderer_Render, CopyChanged, StretchChanged.
- - Session 35 further negatives: Builder::Process with GetPixel's return type BYTE/char/int/unsigned/BOOL/short/void* (BYTE and char keep 3, others worse); WaveTestVolume with an inline no-op taking the clamped volume (4 forms: still 13; REA/Hopper asm confirms `mov esi,eax` after the clamp is dead and esi is only saved/restored); AddTrapType block-temp (c15) re-checked under the real name = 11, and with `pad_mapaddtraptype` 0-90 still 11. No source changed.
- - **Session 36 end state (verified 2026-11-02):** **WaveTestVolume MATCHES** (src/process/wave.cpp 14/14, `build.py --funcs PROCESS` = **200/206**, `build/scratch/s36/process_wavematch.log`; `.bss` unchanged (46 pre-existing wrong); `make` OK; Wine HD level 1 and digger runs MATCH the baselines, `build/runs/s36-l1`, `s36-dig`). Totals **679/696**: PROCESS 6 misses (Basher 27, Builder 3, Miner 25, AddObjectType 32, AddTrapType 17, BuildCollision 2), LEMMINGS 8 (ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4), RENDER 3 = 17 left. **The fix:** `devs = waveOutGetNumDevs(); switch (n = devs >= 100 ? 100 : devs) {
- - **Session 36, AddTrapType/AddObjectType (17 / 32, unchanged):** both share the same two residuals. (1) the original schedules `cmp [count],0` BEFORE the `list->count`/`list->slot` stores (ours after); (2) the original's fx update is 16-bit (`movsx eax,ax; add ax,cx`, fx in eax, pointer in edx, counter `dec edi`) while ours is `add eax,edi` (32-bit). Tried: `short`/`WORD`/`unsigned short` fx x 5 update spellings (30-46 insns, fx then lives in dx/si), `(short)(fx + w)`, `fx + (short)w`, `(short)fx + w`, double casts, `(short)x` init (16, but shifts the rest of map.cpp: file 15/23 vs 20), `(short)` casts on the x store and on g_scale, count/slot store order. WaveTestVolume's lesson (expressio
-   * **Lab on the AddTrapType/AddObjectType fx update** (`build/scratch/s36/lab/`): with `int fx` every spelling gives `movsx eax,ax; add eax,edx` (32-bit); `fx = (short)((short)fx + (short)w)` gives a 16-bit `add ax,si` but with the `movsx` AFTER the add, the original has it before; `short`/`WORD` fx drop the movsx entirely. No spelling reproduces `movsx eax,ax; add ax,cx`.
- - **Session 36 (2026-11-03) APPLIED: ScrollTo 23 -> 18** (`gameview.cpp`: one more pad symbol `pad_scrollto_x0` before it, plus `padplan.py --start 0x406560` for OnHScroll/LevelHeight/... and 92 symbols `pad_setviewscale0..91` before SetViewScale to keep it at 46 (it went to 60), then `padplan --start 0x406810`). EXE still 326/334 (`build/scratch/s36/exe_scroll.log`), `.bss` 0 wrong, `make` OK, Wine HD level 1 + digger MATCH (`s36-l1f`, `s36-digf`); backup `build/scratch/s36/gameview.cpp.before`. Pad sweeps 0-140 over the other near-misses (`sweepall.log`): ApplyOptions 7 at pads 11-13 (not applied: 1 insn, the hoisted `xor edi,edi` and `cmp [g_zoom],esi` remain), BuildRows/AddTrapType/AddOb
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33 declaration climbs (`tools/msvc/search/declclimb.py`, pairwise swaps + move-to-front, pads 0-3/0-6; logs `build/scratch/s33_bp/*_climb.log`): no new matches. Builder::Process 3, DrawPreview 36, DockWnd::Drag 17, ScrollTo 22 (from 23, pad 0, not applied), SetViewScale 45 (from 46, pad 2, not applied), ShowLevelDetails 37, Miner::Process 25, Basher::Process 27, Map::AddTrapType 17. None of these is a declaration-order tie; each needs a source-shape change. WaveTestVolume skipped: the original keeps the clamped volume in esi though it is never used again (a dead value in a callee-saved register), which no declaration order explains.
- - Session 33 Map::AddTrapType: the original's loop advances fx with a 16-bit `add ax,cx`. A `short fx` is far worse (46); int fx with `fx = (short)x` init gives 16 (from 17). Not applied (`att2.txt`).
- - Session 33, inline/padding hunt (no new match): `Message` (lemming.h, a plain inline function, not a class member) dummy locals 0-8 x padding for Miner (pad_top 0-40) and Basher (pad_basherprocess 0-40): no effect (hook removed again, lemming.h unchanged). End-of-file padding (file-wide symbol count) does not move non-inline functions (Builder, WaveTestVolume, Miner, Basher, AddObjectType all unchanged), so the count effect is specific to class-member inlines. pad_top 0-64 for AddObjectType/AddTrapType: no gain. BuildCollision with all declarations at the top + declclimb: 2. Renderer_Render: 8 best setup orders x pads 0-64 and 5 canvas-pointer spellings: 5.
- - Session 33, split-and-climb batch (new tool `tools/msvc/search/splitclimb.py MODULE SOURCE ADDR [--body F] [--pads A-B] [--apply]`: the FillObjects recipe, i.e. every initialised/mid-block declaration becomes a top declaration plus an assignment in place, then a pairwise declaration climb; compiles every variant under the REAL file name in `build/scratch/splitclimb/<func>/<k>/`, whose baseline reproduces try.sh on the real file, unlike variants.py, which gave Miner 32 instead of 25). Results, best (insns, unmatched in file): ScrollTo (short x body) 21 (pads 0-3), 18 at pad 19 but costs a file match; SetViewScale 46; OpenLevelItem 27; ShowLevelDetails 37; DrawPreview 36; AddTrapType 17; Add
-   - `PROCESS.DLL`: **201/206** (+1 match; only 5 functions remain: `Basher::Process` 27, `Builder::Process` 3, `Miner::Process` 25, `Map::AddObjectType` 32, `Map::AddTrapType` 17).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
