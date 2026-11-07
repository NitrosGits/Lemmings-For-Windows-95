# Builder::Process hand-off

> Generated 2026-11-04. Project `/Users/daniel/Lemmings-For-Windows-95` (not a git repo). Module **PROCESS.DLL**, function at **0x10001690**, source `src/process/builder.cpp`.
> Status: `0x10001690 Builder::Process: DIFF (400 bytes) 5 bytes differ, first at +0x72, 3 insns differ; +0x77 ?g_map@@3PAVMap@@A: original 0x92880d, expected 0x10009288`

## Goal
Make `Builder::Process` compile with VC++ 2.2 (`/Ox`, via `tools/msvc/try.sh PROCESS.DLL src/process/builder.cpp -v -f Builder::Process`) to bytes identical to the original, without breaking the other functions in `builder.cpp` or the MinGW build (`make`).

## How to test
```
zsh tools/msvc/try.sh PROCESS.DLL src/process/builder.cpp -v -f Builder::Process
python3 tools/msvc/realname.py PROCESS.DLL src/process/builder.cpp 0x10001690 SPEC.py --jobs 3   # sweep source variants under the real file name
```
`realname.py`'s insn count can hide byte growth: always confirm with `try.sh`. See `docs/DECOMP.md` (padding externs, size-mode functions, block-scoped temporaries, inline-member expansion) and HANDOFF.md.

## Current source
```cpp
/* 0x10001690 */
void Builder::Process(LEMMING *l, DLENTRY **list)
{
    int x, y;
    PNT at;
    int n;
    int f;

    f = ++l->frame;
    n = l->frames;
    if (f >= n)
        l->frame = l->loop - n + f;
    switch (l->frame) {
    case 0:
        x = l->x + l->dir;
        y = l->y - 1;
        l->x = x;
        l->y = y;
        if (g_map->GetPixel(x, y - 1) || g_map->GetPixel(x += l->dir, y - 1)) {
            l->x = x;
            l->y = y;
            l->dir = -l->dir;
            Leave(l);
            g_walker.Enter(l);
            return;
        }
        if (--l->count == 0) {
            l->x = x;
            l->y = y;
            Leave(l);
            g_shrugger.Enter(l);
            return;
        }
        if (g_map->GetPixel(x + l->dir * 2, y - 9)) {
            l->x = x;
            l->y = y;
            l->dir = -l->dir;
            Leave(l);
            g_walker.Enter(l);
            return;
        }
        l->x = x;
        l->y = y;
        break;
    case 9:
        x = l->xoff + l->x;
        y = l->y - 1;
        if (y >= 0) {
            if (l->dir == -1)
                x -= 4;
            g_map->AddSprite(list, g_sprites, SPR_BRICK, x, y);
        }
        break;
    case 10:
        if (l->count <= 3) {
            at.x = l->x;
            at.y = l->y + l->yoff;
            SoundPlayFX(at, 0);
        }
        break;
    }
}
```

## Side-by-side (ours | original; `*` marks differing rows)
```
     10001690  sub    esp,0x8                                | sub    esp,0x8
     10001693  push   ebx                                    | push   ebx
     10001694  push   esi                                    | push   esi
     10001695  mov    esi,DWORD PTR [esp+0x14]               | mov    esi,DWORD PTR [esp+0x14]
     10001699  push   edi                                    | push   edi
     1000169a  push   ebp                                    | push   ebp
     1000169b  mov    edi,ecx                                | mov    edi,ecx
     1000169d  mov    eax,DWORD PTR [esi+0x14]               | mov    eax,DWORD PTR [esi+0x14]
     100016a0  mov    ecx,DWORD PTR [esi+0x1c]               | mov    ecx,DWORD PTR [esi+0x1c]
     100016a3  inc    eax                                    | inc    eax
     100016a4  mov    DWORD PTR [esi+0x14],eax               | mov    DWORD PTR [esi+0x14],eax
     100016a7  cmp    eax,ecx                                | cmp    eax,ecx
     100016a9  jl     0x100016b5                             | jl     0x100016b5
     100016ab  mov    edx,DWORD PTR [esi+0x20]               | mov    edx,DWORD PTR [esi+0x20]
     100016ae  sub    edx,ecx                                | sub    edx,ecx
     100016b0  add    edx,eax                                | add    edx,eax
     100016b2  mov    DWORD PTR [esi+0x14],edx               | mov    DWORD PTR [esi+0x14],edx
     100016b5  mov    eax,DWORD PTR [esi+0x14]               | mov    eax,DWORD PTR [esi+0x14]
     100016b8  test   eax,eax                                | test   eax,eax
     100016ba  je     0x100016d8                             | je     0x100016d8
     100016bc  cmp    eax,0x9                                | cmp    eax,0x9
     100016bf  je     0x100017bb                             | je     0x100017bb
     100016c5  cmp    eax,0xa                                | cmp    eax,0xa
     100016c8  je     0x100017f6                             | je     0x100017f6
     100016ce  pop    ebp                                    | pop    ebp
     100016cf  pop    edi                                    | pop    edi
     100016d0  pop    esi                                    | pop    esi
     100016d1  pop    ebx                                    | pop    ebx
     100016d2  add    esp,0x8                                | add    esp,0x8
     100016d5  ret    0x8                                    | ret    0x8
     100016d8  mov    ebx,DWORD PTR [esi]                    | mov    ebx,DWORD PTR [esi]
     100016da  mov    ebp,DWORD PTR [esi+0x4]                | mov    ebp,DWORD PTR [esi+0x4]
     100016dd  add    ebx,DWORD PTR [esi+0x10]               | add    ebx,DWORD PTR [esi+0x10]
     100016e0  dec    ebp                                    | dec    ebp
     100016e1  mov    DWORD PTR [esi],ebx                    | mov    DWORD PTR [esi],ebx
     100016e3  mov    DWORD PTR [esi+0x4],ebp                | mov    DWORD PTR [esi+0x4],ebp
     100016e6  lea    eax,[ebp-0x1]                          | lea    eax,[ebp-0x1]
     100016e9  mov    ecx,DWORD PTR ds:0x10009288            | mov    ecx,DWORD PTR ds:0x10009288
     100016ef  push   eax                                    | push   eax
     100016f0  push   ebx                                    | push   ebx
     100016f1  call   0x100054c0                             | call   0x100054c0
     100016f6  test   al,al                                  | test   al,al
     100016f8  jne    0x10001791                             | jne    0x10001791
     100016fe  add    ebx,DWORD PTR [esi+0x10]               | add    ebx,DWORD PTR [esi+0x10]
   * 10001701  lea    eax,[ebp-0x1]                          | lea    ecx,[ebp-0x1]
   * 10001704  push   eax                                    | push   ecx
   * 10001705  mov    ecx,DWORD PTR ds:0x10009288            | push   ebx
   * 1000170b  push   ebx                                    | mov    ecx,DWORD PTR ds:0x10009288
     1000170c  call   0x100054c0                             | call   0x100054c0
     10001711  test   al,al                                  | test   al,al
     10001713  jne    0x10001791                             | jne    0x10001791
     10001715  mov    ax,WORD PTR [esi+0x34]                 | mov    ax,WORD PTR [esi+0x34]
     10001719  dec    ax                                     | dec    ax
     1000171b  mov    WORD PTR [esi+0x34],ax                 | mov    WORD PTR [esi+0x34],ax
     1000171f  jne    0x10001743                             | jne    0x10001743
     10001721  push   esi                                    | push   esi
     10001722  mov    eax,DWORD PTR [edi]                    | mov    eax,DWORD PTR [edi]
     10001724  mov    ecx,edi                                | mov    ecx,edi
     10001726  mov    DWORD PTR [esi],ebx                    | mov    DWORD PTR [esi],ebx
     10001728  mov    DWORD PTR [esi+0x4],ebp                | mov    DWORD PTR [esi+0x4],ebp
     1000172b  call   DWORD PTR [eax+0x8]                    | call   DWORD PTR [eax+0x8]
     1000172e  push   esi                                    | push   esi
     1000172f  mov    ecx,0x100091a8                         | mov    ecx,0x100091a8
     10001734  call   0x100041f0                             | call   0x100041f0
     10001739  pop    ebp                                    | pop    ebp
     1000173a  pop    edi                                    | pop    edi
     1000173b  pop    esi                                    | pop    esi
     1000173c  pop    ebx                                    | pop    ebx
     1000173d  add    esp,0x8                                | add    esp,0x8
     10001740  ret    0x8                                    | ret    0x8
     10001743  lea    eax,[ebp-0x9]                          | lea    eax,[ebp-0x9]
     10001746  mov    ecx,DWORD PTR [esi+0x10]               | mov    ecx,DWORD PTR [esi+0x10]
     10001749  push   eax                                    | push   eax
     1000174a  lea    edx,[ebx+ecx*2]                        | lea    edx,[ebx+ecx*2]
     1000174d  push   edx                                    | push   edx
     1000174e  mov    ecx,DWORD PTR ds:0x10009288            | mov    ecx,DWORD PTR ds:0x10009288
     10001754  call   0x100054c0                             | call   0x100054c0
     10001759  test   al,al                                  | test   al,al
     1000175b  mov    DWORD PTR [esi],ebx                    | mov    DWORD PTR [esi],ebx
     1000175d  mov    DWORD PTR [esi+0x4],ebp                | mov    DWORD PTR [esi+0x4],ebp
     10001760  je     0x10001787                             | je     0x10001787
     10001762  mov    eax,DWORD PTR [esi+0x10]               | mov    eax,DWORD PTR [esi+0x10]
     10001765  push   esi                                    | push   esi
     10001766  mov    ebx,DWORD PTR [edi]                    | mov    ebx,DWORD PTR [edi]
     10001768  mov    ecx,edi                                | mov    ecx,edi
     1000176a  neg    eax                                    | neg    eax
     1000176c  mov    DWORD PTR [esi+0x10],eax               | mov    DWORD PTR [esi+0x10],eax
     1000176f  call   DWORD PTR [ebx+0x8]                    | call   DWORD PTR [ebx+0x8]
     10001772  push   esi                                    | push   esi
     10001773  mov    ecx,0x10009008                         | mov    ecx,0x10009008
     10001778  call   0x10004630                             | call   0x10004630
     1000177d  pop    ebp                                    | pop    ebp
     1000177e  pop    edi                                    | pop    edi
     1000177f  pop    esi                                    | pop    esi
     10001780  pop    ebx                                    | pop    ebx
     10001781  add    esp,0x8                                | add    esp,0x8
     10001784  ret    0x8                                    | ret    0x8
     10001787  pop    ebp                                    | pop    ebp
     10001788  pop    edi                                    | pop    edi
     10001789  pop    esi                                    | pop    esi
     1000178a  pop    ebx                                    | pop    ebx
     1000178b  add    esp,0x8                                | add    esp,0x8
     1000178e  ret    0x8                                    | ret    0x8
     10001791  mov    eax,DWORD PTR [esi+0x10]               | mov    eax,DWORD PTR [esi+0x10]
     10001794  push   esi                                    | push   esi
     10001795  mov    ecx,edi                                | mov    ecx,edi
     10001797  mov    DWORD PTR [esi],ebx                    | mov    DWORD PTR [esi],ebx
     10001799  mov    ebx,DWORD PTR [edi]                    | mov    ebx,DWORD PTR [edi]
     1000179b  mov    DWORD PTR [esi+0x4],ebp                | mov    DWORD PTR [esi+0x4],ebp
     1000179e  neg    eax                                    | neg    eax
     100017a0  mov    DWORD PTR [esi+0x10],eax               | mov    DWORD PTR [esi+0x10],eax
     100017a3  call   DWORD PTR [ebx+0x8]                    | call   DWORD PTR [ebx+0x8]
     100017a6  push   esi                                    | push   esi
     100017a7  mov    ecx,0x10009008                         | mov    ecx,0x10009008
     100017ac  call   0x10004630                             | call   0x10004630
     100017b1  pop    ebp                                    | pop    ebp
     100017b2  pop    edi                                    | pop    edi
     100017b3  pop    esi                                    | pop    esi
     100017b4  pop    ebx                                    | pop    ebx
     100017b5  add    esp,0x8                                | add    esp,0x8
     100017b8  ret    0x8                                    | ret    0x8
     100017bb  mov    eax,DWORD PTR [esi+0x8]                | mov    eax,DWORD PTR [esi+0x8]
     100017be  mov    ecx,DWORD PTR [esi+0x4]                | mov    ecx,DWORD PTR [esi+0x4]
     100017c1  add    eax,DWORD PTR [esi]                    | add    eax,DWORD PTR [esi]
     100017c3  dec    ecx                                    | dec    ecx
     100017c4  js     0x10001816                             | js     0x10001816
     100017c6  cmp    DWORD PTR [esi+0x10],0xffffffff        | cmp    DWORD PTR [esi+0x10],0xffffffff
     100017ca  jne    0x100017cf                             | jne    0x100017cf
     100017cc  sub    eax,0x4                                | sub    eax,0x4
     100017cf  push   ecx                                    | push   ecx
     100017d0  push   eax                                    | push   eax
     100017d1  push   0x15c                                  | push   0x15c
     100017d6  mov    eax,ds:0x10009014                      | mov    eax,ds:0x10009014
     100017db  mov    ecx,DWORD PTR [esp+0x2c]               | mov    ecx,DWORD PTR [esp+0x2c]
     100017df  push   eax                                    | push   eax
     100017e0  push   ecx                                    | push   ecx
     100017e1  mov    ecx,DWORD PTR ds:0x10009288            | mov    ecx,DWORD PTR ds:0x10009288
     100017e7  call   0x100050e0                             | call   0x100050e0
     100017ec  pop    ebp                                    | pop    ebp
     100017ed  pop    edi                                    | pop    edi
     100017ee  pop    esi                                    | pop    esi
     100017ef  pop    ebx                                    | pop    ebx
     100017f0  add    esp,0x8                                | add    esp,0x8
     100017f3  ret    0x8                                    | ret    0x8
     100017f6  cmp    WORD PTR [esi+0x34],0x3                | cmp    WORD PTR [esi+0x34],0x3
     100017fb  jg     0x10001816                             | jg     0x10001816
     100017fd  mov    eax,DWORD PTR [esi]                    | mov    eax,DWORD PTR [esi]
     100017ff  push   0x0                                    | push   0x0
     10001801  mov    DWORD PTR [esp+0x14],eax               | mov    DWORD PTR [esp+0x14],eax
     10001805  mov    eax,DWORD PTR [esi+0x4]                | mov    eax,DWORD PTR [esi+0x4]
     10001808  add    eax,DWORD PTR [esi+0xc]                | add    eax,DWORD PTR [esi+0xc]
     1000180b  mov    edx,DWORD PTR [esp+0x14]               | mov    edx,DWORD PTR [esp+0x14]
     1000180f  push   eax                                    | push   eax
     10001810  push   edx                                    | push   edx
     10001811  call   0x10005e30                             | call   0x10005e30
     10001816  pop    ebp                                    | pop    ebp
     10001817  pop    edi                                    | pop    edi
     10001818  pop    esi                                    | pop    esi
     10001819  pop    ebx                                    | pop    ebx
     1000181a  add    esp,0x8                                | add    esp,0x8
     1000181d  ret    0x8                                    | ret    0x8
```

## What HANDOFF.md already records (most recent last)
- - **Builder::Process (3):** second GetPixel call: original `lea ecx,[ebp-1]; push ecx; push ebx; mov ecx,g_map`, ours `lea eax; push eax; mov ecx,g_map; push ebx`. 10 spellings of `x += l->dir` call/condition (spec `build/scratch/s32/bp_spec.py`) all stay at 3.
- - Remaining: PROCESS Basher 27, Builder 3, Miner 25, AddObjectType 32 (mid-experiment; was 21), AddTrapType 17, BuildCollision 2, WaveTestVolume 13; EXE ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4; RENDER Renderer_Render, CopyChanged, StretchChanged.
- - Tried this session (negative): BuildCollision real-name pad sweep 0-63 (stays 2: the prologue order is not pad-sensitive); inline `Message()` locals (hook `MESSAGE_PAD`, k 0-31) on Miner and Basher: no gain (hook reverted); Builder block temps (7 variants): stays 3.
- - Session 35 further negatives: Builder::Process with GetPixel's return type BYTE/char/int/unsigned/BOOL/short/void* (BYTE and char keep 3, others worse); WaveTestVolume with an inline no-op taking the clamped volume (4 forms: still 13; REA/Hopper asm confirms `mov esi,eax` after the clamp is dead and esi is only saved/restored); AddTrapType block-temp (c15) re-checked under the real name = 11, and with `pad_mapaddtraptype` 0-90 still 11. No source changed.
- - Builder::Process (3) again: 5 more shapes of the first two GetPixel tests (`&&` mix, goto, pre-decrement y, `--y`): best stays 3 (others 9-17). The `lea ecx,[ebp-1]` is y-1 with y in ebp; the residual is still one temp-register choice. Sister: committed the DockWnd pad stub (97aafbe); the size-mode pragma gives nothing there (ctor 87.2 either way, the original also pushes edi).
- - Session 36 (this turn; counts unchanged: EXE 324/334, PROCESS 198/206, RENDER 74/77 = 675/696; source unchanged): handoffs/ now has one hand-off per remaining function (18) + README (`python3 tools/make_handoffs.py` regenerates). Negatives: BuildRows (inverted biHeight test, 10 zero-forms of the else branch, offset init forms: the `mov edi,0` hoist stays); ApplyOptions (original has `xor edi,edi` just before the 0x184 check: `size = 0;` there with/without top-level init, 8 decl orders, dummy early inits: 23 insns because fn-pointer/size registers swap ebx<->edi; base with `int size = 0;` at top stays 8, only the hoisted xor + `cmp [g_zoom],esi` operand order differ); ReadIniSettings (natur
- - **Session 36 end state (verified 2026-11-02):** **WaveTestVolume MATCHES** (src/process/wave.cpp 14/14, `build.py --funcs PROCESS` = **200/206**, `build/scratch/s36/process_wavematch.log`; `.bss` unchanged (46 pre-existing wrong); `make` OK; Wine HD level 1 and digger runs MATCH the baselines, `build/runs/s36-l1`, `s36-dig`). Totals **679/696**: PROCESS 6 misses (Basher 27, Builder 3, Miner 25, AddObjectType 32, AddTrapType 17, BuildCollision 2), LEMMINGS 8 (ApplyOptions 8, ScrollTo 23, SetViewScale 46, ReadIniSettings 3, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, BuildRows 4), RENDER 3 = 17 left. **The fix:** `devs = waveOutGetNumDevs(); switch (n = devs >= 100 ? 100 : devs) {
- - **Session 36 (2026-11-03) more negatives, RENDER state unchanged (Renderer_Render 5, StretchChanged 3, CopyChanged 13; 74/77):** optmode (s, t-off, i-off, y-off, a, w, e) on the three: no gain; StretchChanged: volatile on sx/w/h (3 -> 5+), fuzz.py 600 samples, 220 random valid prologue orders (only the current order gives 3), 6 spellings of the row expression; Renderer_Render: 5 spellings of the across/down computations, 6 more climbs; CopyChanged Stretch-style: pads 0-140 flat at 30 (a pure register rotation), another 90-round climb reached 21. **Pad sweeps 0-140 (previous sweeps stopped at 63) for Builder::Process (3), BuildCollision (2), ReadIniSettings (3): no pad count changes the res
- - **Session 36 (2026-11-03) APPLIED: ScrollTo 23 -> 18** (`gameview.cpp`: one more pad symbol `pad_scrollto_x0` before it, plus `padplan.py --start 0x406560` for OnHScroll/LevelHeight/... and 92 symbols `pad_setviewscale0..91` before SetViewScale to keep it at 46 (it went to 60), then `padplan --start 0x406810`). EXE still 326/334 (`build/scratch/s36/exe_scroll.log`), `.bss` 0 wrong, `make` OK, Wine HD level 1 + digger MATCH (`s36-l1f`, `s36-digf`); backup `build/scratch/s36/gameview.cpp.before`. Pad sweeps 0-140 over the other near-misses (`sweepall.log`): ApplyOptions 7 at pads 11-13 (not applied: 1 insn, the hoisted `xor edi,edi` and `cmp [g_zoom],esi` remain), BuildRows/AddTrapType/AddOb
- - Session 33, EXE/PROCESS (user switched from RENDER): Builder::Process, 5 control-flow forms of the two GetPixel tests (goto, duplicated blocks, negated nested ifs) x pads 0-6: all 3. A dummy function placed before it does not flip the `lea eax/ecx,[ebp-1]` temp (so it is not compiler state carried across functions). ReadIniSettings: swapping the `g_mapXUsed, g_usedMapY` order in the extern and/or the definition: still 3. A second POINT (4 declaration orders x 2 assignment orders, plus block-scoped) x pads 0-3: 13, worse. `pt.x` first makes VC spill pt. Both residuals, like RENDER's three, are a single temp-register choice. No source changed (`build/scratch/s33_bp/`).
- - **Re-verified 2026-11-02 (fresh `build.py --funcs` per module, `build/scratch/s33_bp/verify2.log`, `verify3.log`, `verify_RENDER.log`):** EXE 324/334, PROCESS 198/206, RENDER 74/77 = **675/696**, which agrees with the header. StartGame and SaveLayout now MATCH (another session's work; this supersedes the 'SaveLayout not applied' note above). SoundInit MATCH. Current misses: EXE (10): ReadIniSettings 3, Bitmap::BuildRows 4, ApplyOptions 7, MapView::DrawLemmings 7, DockWnd::Drag 17, ScrollTo 23, OpenLevelItem 27, DrawPreview 36, ShowLevelDetails 37, SetViewScale 46. PROCESS (8): BuildCollision 2, Builder::Process 3, Map::FillObjects 4, WaveTestVolume 13, Map::AddTrapType 17, Map::AddObjectTy
- - Session 33 declaration climbs (`tools/msvc/search/declclimb.py`, pairwise swaps + move-to-front, pads 0-3/0-6; logs `build/scratch/s33_bp/*_climb.log`): no new matches. Builder::Process 3, DrawPreview 36, DockWnd::Drag 17, ScrollTo 22 (from 23, pad 0, not applied), SetViewScale 45 (from 46, pad 2, not applied), ShowLevelDetails 37, Miner::Process 25, Basher::Process 27, Map::AddTrapType 17. None of these is a declaration-order tie; each needs a source-shape change. WaveTestVolume skipped: the original keeps the clamped volume in esi though it is never used again (a dead value in a callee-saved register), which no declaration order explains.
- - Session 33, inline/padding hunt (no new match): `Message` (lemming.h, a plain inline function, not a class member) dummy locals 0-8 x padding for Miner (pad_top 0-40) and Basher (pad_basherprocess 0-40): no effect (hook removed again, lemming.h unchanged). End-of-file padding (file-wide symbol count) does not move non-inline functions (Builder, WaveTestVolume, Miner, Basher, AddObjectType all unchanged), so the count effect is specific to class-member inlines. pad_top 0-64 for AddObjectType/AddTrapType: no gain. BuildCollision with all declarations at the top + declclimb: 2. Renderer_Render: 8 best setup orders x pads 0-64 and 5 canvas-pointer spellings: 5.
-   - `PROCESS.DLL`: **201/206** (+1 match; only 5 functions remain: `Basher::Process` 27, `Builder::Process` 3, `Miner::Process` 25, `Map::AddObjectType` 32, `Map::AddTrapType` 17).

## Hints
- Differences are almost always a register/ordering choice by the compiler. Levers that have worked elsewhere: `extern int pad_xxxN;` symbol padding, declaration order and locals, block-scoped temporaries `{ T t = expr; v = t; }`, deriving a field from a just-stored field, `#pragma optimize("t", off)` plus a naked `_emit` padding stub for size-mode functions.
- Write new reusable scripts under `tools/`, record negatives in HANDOFF.md.
