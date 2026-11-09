# Technical Dossier: `PROCESS.DLL` Remaining Functions (203 / 206 Matched)

## 1. Executive Summary

`PROCESS.DLL` currently sits at **203 / 206 functions byte-identical (98.54%)** under Microsoft Visual C++ 2.2 (`CL.EXE` 9.10 / `C2.EXE` 9.10 with `/Ox /Gr`). All `.data` symbol layouts match 100% (`0 wrong` in `mapcmp.py`).

Both [`Miner::Process`](file:///Users/daniel/Lemmings-For-Windows-95/src/process/miner.cpp#L20-L68) (`0x10003c60`, 400 B) and [`Basher::Process`](file:///Users/daniel/Lemmings-For-Windows-95/src/process/basher.cpp#L23-L70) (`0x10001040`, 528 B) are now **100% MATCHED** (`7/7` functions in `src/process/miner.cpp` and `7/7` functions in `src/process/basher.cpp`)!

The **3 remaining functions** (plus the resolved `Miner::Process` and `Basher::Process`) fall into three distinct compiler-mechanics categories in `C2.EXE`:

| Function | Address | Size | Current Diff | Core Compiler Mechanism |
|---|---|---|---|---|
| [`Miner::Process`](file:///Users/daniel/Lemmings-For-Windows-95/src/process/miner.cpp#L20-L68) | `0x10003c60` | 400 B | **MATCHED (0 diff)** | **100% MATCHED!** Fallthrough from `case 15` into `case 5..23: x = 0; break;` + two-step `n += l->dir == -1 ? 2 : 0;` before `EraseSprite(..., n + SPR_MINEHOLE, x, y + 1)` |
| [`Basher::Process`](file:///Users/daniel/Lemmings-For-Windows-95/src/process/basher.cpp#L23-L70) | `0x10001040` | 528 B | **MATCHED (0 diff)** | **100% MATCHED!** Fallthrough from `case 11..31` into `case 7, 9, 17, 23, 25: x = 0; break;` + two-step `f = (l->dir == -1 ? 4 : 0) + (l->frame & 15);` before `EraseSprite(..., f + SPR_DIGHOLE, x, y)` |
| [`Builder::Process`](file:///Users/daniel/Lemmings-For-Windows-95/src/process/builder.cpp#L8-L68) | `0x10001690` | 400 B | **3 insns (5 B)** | Post-colouring instruction-level register re-choice (`sub_41c850` / `sub_41cea0` in `color.c`) on `y - 1` (`lea ecx, [ebp-1]` vs `lea eax, [ebp-1]`) |
| [`Map::AddTrapType`](file:///Users/daniel/Lemmings-For-Windows-95/src/process/map.cpp#L68-L101) | `0x10004880` | 288 B | **6 insns** | 16-bit vs 32-bit induction variable narrowing (`mov ax, [esp+0x28]; add ax, cx` vs `movsx eax, [esp+0x28]; add eax, ecx`) |
| [`Map::AddObjectType`](file:///Users/daniel/Lemmings-For-Windows-95/src/process/map.cpp#L29-L64) | `0x10004760` | 288 B | **8 insns** | Same 16-bit vs 32-bit induction variable narrowing as `AddTrapType`, plus `eax`/`ecx` and `ebx`/`edi` register tie-break |

---

## 2. Category A: `Builder::Process` (`0x10001690`, 3 Instructions / 5 Bytes Differ)

### 2.1 Exact Assembly Diff (`0x100016fc..0x10001713`)

In [`src/process/builder.cpp`](file:///Users/daniel/Lemmings-For-Windows-95/src/process/builder.cpp#L54-L90), case `10` checks terrain pixels at `(x, y - 1)` and `(x += l->dir, y - 1)`:

```
=== OURS (src/process/builder.cpp) ===                 === ORIGINAL (PROCESS.DLL @ 0x10001690) ===
100016e6: lea    eax, [ebp-0x1]                        100016e6: lea    eax, [ebp-0x1]
100016e9: mov    ecx, ds:0x10009288                    100016e9: mov    ecx, ds:0x10009288
100016ef: push   eax                                   100016ef: push   eax
100016f0: push   ebx                                   100016f0: push   ebx
100016f1: call   Map::GetPixel                         100016f1: call   Map::GetPixel
100016f6: test   al, al                                100016f6: test   al, al
100016f8: jne    10001791                              100016f8: jne    10001791
100016fe: add    ebx, [esi+0x10]                       100016fe: add    ebx, [esi+0x10]
10001701: lea    eax, [ebp-0x1]                      | 10001701: lea    ecx, [ebp-0x1]
10001704: push   eax                                 | 10001704: push   ecx
10001705: mov    ecx, ds:0x10009288                  | 10001705: push   ebx
1000170b: push   ebx                                 | 10001706: mov    ecx, ds:0x10009288
1000170c: call   Map::GetPixel                         1000170c: call   Map::GetPixel
```

Every single instruction in the 400-byte function matches except `10001701..1000170b`:
- At the first `GetPixel` call (`100016e6`), `y - 1` is evaluated into `eax` (`lea eax, [ebp-1]`).
- At the second `GetPixel` call (`10001701`), **our** build also evaluates `y - 1` into `eax` (`lea eax, [ebp-1]; push eax; mov ecx, ds:0x10009288; push ebx`), which allows the Pentium scheduler (`0x42e8f0`) to hoist `mov ecx, ds:0x10009288` between `push eax` and `push ebx`.
- In the **original** binary, the second `y - 1` is evaluated into `ecx` (`lea ecx, [ebp-1]; push ecx; push ebx; mov ecx, ds:0x10009288`), creating a WAR hazard on `ecx` (`push ecx` before `mov ecx, ...`) that keeps `push ebx` ahead of `mov ecx, ds:0x10009288`.

---

## 3. Category B: `Map::AddTrapType` (`0x10004880`, 6 Insns) & `Map::AddObjectType` (`0x10004760`, 8 Insns)

### 3.1 `Map::AddTrapType` Diff (`0x100048cd..0x1000490b`)

In [`src/process/map.cpp`](file:///Users/daniel/Lemmings-For-Windows-95/src/process/map.cpp#L569-L632), all 6 registers match (`eax = fx`, `ecx = step`, `edx = &list->frames[i].y`, `ebx = w`, `edi = count`, `ebp = g_scale`, `esi = t`), and `list` stays on the stack at `[esp+0x10]`. The only discrepancy is 16-bit (`ax`/`cx`) vs 32-bit (`eax`/`ecx`) on the `fx` accumulator:

```
=== ORIGINAL (0x10004880) ===                        === OURS (src/process/map.cpp) ===
100048cd: mov    ax, word ptr [esp+0x28]           | 100048cd: movsx  eax, word ptr [esp+0x28]
100048d2: lea    edx, [ecx+0x6]                      100048d2: lea    edx, [ecx+0x6]
100048d5: mov    ebx, [esp+0x30]                     100048d5: mov    ebx, [esp+0x30]
100048d9: mov    edi, [esp+0x38]                     100048d9: mov    edi, [esp+0x38]
100048dd: movsx  ecx, bx                             100048dd: movsx  ecx, bx
...                                                  ...
100048ed: add    edx, 0x8                            100048ed: add    edx, 0x8
100048f0: add    ax, cx                            | 100048f0: mov    word ptr [edx-0x8], bp
100048f3: mov    word ptr [edx-0x8], bp            | 100048f4: add    eax, ecx
100048f7: mov    [edx-0x6], bx                       100048f6: mov    [edx-0x6], bx
100048fb: mov    [edx-0x4], si                       100048fa: mov    [edx-0x4], si
100048ff: dec    edi                                 100048fe: dec    edi
10004900: jne    100048ea                            100048ff: jne    100048ea
10004902: mov    ecx, [esp+0x10]                   | 10004901: mov    eax, [esp+0x10]
10004906: mov    eax, [esp+0x2c]                   | 10004905: mov    ecx, [esp+0x2c]
1000490a: mov    [ecx+0x12], ax                    | 10004909: mov    [eax+0x12], cx
```

Note why `movsx ecx, bx` is emitted in the preheader even when the loop body uses `add ax, cx`:
- When `fx` is `int` (`fx += (short)w`), C2 widens `(short)w` via `movsx ecx, bx`, Hoists it out of the loop, and uses 32-bit `add eax, ecx`.
- When `fx` is `short` in a normal write-up, C2 does **not** widen `w` (`movsx ecx, bx` disappears), freeing a register so `list` moves into a register instead of `[esp+0x10]`.
- Therefore, in the original code, `(short)w` was widened to `int` (`movsx ecx, bx`)—for instance because the expression had `int` type or `step` was an `int`—while the target accumulator `fx` (or `x` parameter itself!) was narrowed to 16-bit `ax` by C2's narrowing pass!

---

## 4. Category C: `Miner::Process` (`0x10003c60`, 14 Insns / 36 B) & `Basher::Process` (`0x10001040`, 14 Insns / 15 B)

### 4.1 Jump-Table Pruning Solved (`25 -> 14` in `Miner::Process`, `27 -> 14` in `Basher::Process`)

Both `Miner::Process` and `Basher::Process` dispatch on `l->frame` (`switch (l->frame)`) with dense case ranges containing multiple odd no-op cases:
- **Why plain `break;` failed**: `sub_4618c0` (the switch case pruner) deletes cases whose target label equals `default_label` (`+0x18`), with an `i++` after `memmove` skip-next bug that deletes every *other* case (`5, 9, 13, 17, 21` in `Miner`, `7, 17, 23` in `Basher`).
- **Why standalone `x = 0; break;` added an extra epilogue**: When the `case 5..23:` block is preceded by a `break;` from the previous case, it becomes an independent basic block at the end of the function; after dead-store elimination removes `x = 0;`, `sub_43c370` (tail-merging / epilogue copying) gives that block its own `pop/add esp/ret` epilogue copy instead of sharing the preceding case's fallthrough epilogue (`0x10003d8b` / `0x100011f9`).
- **The Solution (applied in `src/process/miner.cpp` and `src/process/basher.cpp`)**:
  - **Fall through** from the preceding case (`case 15:` in `Miner::Process`; `case 11..15, 27..31:` in `Basher::Process`) directly into the no-op cases (`case 5..23: x = 0; break;` in `Miner`; `case 7, 9, 17, 23, 25: x = 0; break;` in `Basher`)!
  - Because the block contains `x = 0;` during switch lowering (`sub_4618c0`), its label is distinct from `default_label`, so **all 14 jump-table entries in `Miner::Process` (`0x10003d94..0x10003de9`) and all 10 jump-table entries in `Basher::Process` (`0x10001204..0x1000124a`) match 100% byte-for-byte**!
  - And because the preceding case falls through into it, after `x = 0;` is eliminated as a dead store, `sub_463380` (adjacent label merger at `0x462e0f`) merges the case label directly with the shared epilogue label (`0x10003d8b` in `Miner`, `0x100011f9` in `Basher`), eliminating the duplicate epilogue!

### 4.2 Remaining Diff in `Basher::Process` (14 Insns / 15 Bytes, Same Shape!)

With the jump table and epilogue 100% matched (`0x100010c2..0x10001250` is **100% byte-identical**!), the only remaining diff in `Basher::Process` is a pure 3-register (`eax`/`ecx`/`edx`) permutation in the switch dispatch (`10001065..1000107c`) and `case 2..21` `EraseSprite` setup (`10001083..100010b6`):

```
=== OURS (left) vs ORIGINAL (right) ===
* 10001065  mov    edx,DWORD PTR [esi+0x14]       | mov    ecx,DWORD PTR [esi+0x14]
* 10001068  lea    eax,[edx-0x2]                  | lea    edx,[ecx-0x2]
* 1000106b  cmp    eax,0x1d                       | cmp    edx,0x1d
  1000106e  ja     0x100011f9                     | ja     0x100011f9
* 10001074  xor    ecx,ecx                        | xor    eax,eax
* 10001076  mov    cl,BYTE PTR [eax+0x1000122c]   | mov    al,BYTE PTR [edx+0x1000122c]
* 1000107c  jmp    DWORD PTR [ecx*4+0x10001204]   | jmp    DWORD PTR [eax*4+0x10001204]
* 10001083  mov    ecx,DWORD PTR [esi+0x8]        | mov    edx,DWORD PTR [esi+0x8]
  10001086  mov    eax,DWORD PTR [esi+0x4]        | mov    eax,DWORD PTR [esi+0x4]
* 10001089  add    ecx,DWORD PTR [esi]            | add    edx,DWORD PTR [esi]
  1000108b  mov    ebx,DWORD PTR [esi+0xc]        | mov    ebx,DWORD PTR [esi+0xc]
  1000108e  lea    ebp,[eax+ebx*1-0x6]            | lea    ebp,[eax+ebx*1-0x6]
  10001092  mov    eax,DWORD PTR [esi+0x10]       | mov    eax,DWORD PTR [esi+0x10]
  10001095  inc    eax                            | inc    eax
  10001096  push   ebp                            | push   ebp
* 10001097  push   ecx                            | push   edx
  10001098  cmp    eax,0x1                        | cmp    eax,0x1
  1000109b  sbb    eax,eax                        | sbb    eax,eax
* 1000109d  mov    ecx,DWORD PTR ds:0x10009014    | mov    edx,DWORD PTR ds:0x10009014
  100010a3  and    eax,0x4                        | and    eax,0x4
* 100010a6  and    edx,0xf                        | and    ecx,0xf
* 100010a9  lea    eax,[eax+edx*1+0x14e]          | lea    eax,[eax+ecx*1+0x14e]
* 100010b0  mov    edx,DWORD PTR [esp+0x28]       | mov    ecx,DWORD PTR [esp+0x28]
  100010b4  push   eax                            | push   eax
* 100010b5  push   ecx                            | push   edx
* 100010b6  push   edx                            | push   ecx
```
Notice why all 14 differing instructions are **one single cause**:
- Because `l->frame` is reused in `case 2..21` (`l->frame & 15`), C2 CSEs `l->frame` across the `switch (l->frame)` dispatch!
- In the **original** (right), that CSE temporary (`l->frame`) was coloured `ecx` (`mov ecx, [esi+0x14]`), so the switch index `frame - 2` got `edx` and the byte-table lookup got `eax`; then inside `case 2..21`, since `ecx` was occupied by `l->frame`, `x = l->xoff + l->x` got `edx`!
- In **ours** (left), `l->frame` was coloured `edx` (`mov edx, [esi+0x14]`), which cascaded to swap `ecx` ↔ `edx` throughout `10001065..100010b6`!

### 4.3 Remaining Diff in `Miner::Process` (14 Insns / 36 Bytes)

Exactly the same phenomenon occurs in `Miner::Process`:
- Because `l->frame` is reused in `case 1, 2` (`n = l->frame - 1`), C2 CSEs `l->frame` across `switch (l->frame)`!
- In the **original** (right), `l->frame` was coloured `edx` (`mov edx, [esi+0x14]`), so in `case 1, 2`, `y` got `ecx` (`mov ecx, [esi+4]`) and `n = l->frame - 1` decremented `edx` (`dec edx`)!
- In **ours** (left), `l->frame` was coloured `ecx` (`mov ecx, [esi+0x14]`), so `n = l->frame - 1` decremented `ecx` (`dec ecx`) and `y` got `edx` (`mov edx, [esi+4]; lea eax, [eax+edx-3]`)!

---

## 5. Active Investigation Log (`C2.EXE` Binary Analysis)

### 5.1 Complete Post-Codegen MD Pass Pipeline (`sub_462da0` at `0x462da0..0x463016`)

From disassembling `main.c` (`0x440515`) and `sub_462da0` (`0x462da0`), the exact sequence of machine-dependent (MD) passes after instruction selection (`0x401000`) is:

| Call Site | Target Function | Purpose / Behaviour |
|---|---|---|
| `0x462df3` | `0x4105d0` | Pre-register-allocation peephole / tuple setup |
| `0x462e04` | `0x463510` | **Label reference counting**: walks all instructions, increments `label->+0x30` for branches and jump-table entries (`opcode 0xbb00` at `0x46346a`) |
| `0x462e0f` | `0x463380` | **Adjacent label merging**: merges consecutive MD label tuples (`type == 1`) and rewrites jump targets AND jump-table (`0xbb00`) targets at `0x463450..0x4634c6` |
| `0x462e1a` | `0x4635b0` | **Dead label elimination**: unlinks any label tuple with `refcount (+0x30) == 0` |
| `0x462e1f` | `0x409520` (`sub_40da50`) | **Jump optimization & branch threading**: removes jump-to-next-instruction and threads jump-to-jump (`jmp L1` where `L1: jmp L2` → `jmp L2`), decrementing `L1`'s refcount and deleting `L1` if `0`. **Crucially, `sub_40da50` does NOT update jump tables (`0xbb00`)!** |
| `0x462e58` | `0x4245c0` / `0x424620` | Global dataflow / live-range preparation |
| `0x462eb1` | `0x417260` | **Chaitin-Briggs graph-colouring register allocator**: builds interference graph (`0x419ce0`), simplifies (`0x417690`), colours (`0x417580`), spills (`0x417c80`), and runs **post-colouring local register re-choice** (`0x41c170` → `0x41c850`) |
| `0x462ebc` | `0x41aaf0` | Post-allocation copy propagation and instruction combining |
| `0x462fa7` | `0x42e8f0` | **Pentium U/V pipe instruction scheduler** |
| `0x462fcf` | `0x43c370` | **Tail-merging / epilogue copying**: replaces `jmp epilogue_label` with inline `pop/add esp/ret` if instructions match, and unlinks `epilogue_label` if its refcount drops to `0` |

### 5.2 Switch Case Pruner (`sub_4618c0` at `0x4618c0..0x461960`)

Before emitting a switch statement, `sub_4619f0` calls `sub_4618c0` (at `0x461a24`):
- `sub_4618c0` iterates over the switch's case-value array (`+0x28`), comparing each case's target label with `default_label` (`+0x18`, which is the `break` target when no explicit `default:` exists).
- When `case_label == default_label`, it unlinks the entry by shifting the remaining entries down (`memmove` at `0x461938`) and decrementing `num_cases` (`+0x20`).
- **Compiler Bug**: After shifting `entries[i+1..n-1]` down into `entries[i]`, the loop still executes `inc edx; add ebp, 8` (`i++`), skipping `entries[i+1]`! Consequently, when consecutive cases jump to `default_label`, **every second case survives** (`5, 9, 13, 17, 21` are deleted, while `7, 11, 19, 23` survive).
- **Implication for `Miner::Process` & `Basher::Process`**: For all 9 odd cases (`5, 7, 9, 11, 13, 17, 19, 21, 23`) to survive `sub_4618c0`, their target label at switch-lowering time must **not** equal `default_label` (`+0x18`), or `default_label` must point to a different block (such as an explicit `default:` block).

### 5.3 `Builder::Process` Colouring Trace (`tools/msvc/c2dbg/colour_trace_fn.py`)

Running `colour_trace_fn.py` on `Builder::Process` (driver call 1) reveals the exact register assignments in the final colouring round:
- `0x4025` (`l`) → `esi` (`deg 3, cost 162`)
- `0x4028` (`this`) → `edi` (`deg 3, cost 16`)
- `0x402b` (`x`) → `ebx` (`deg 3, cost 47`)
- `0x402d` (`y`) → `ebp` (`deg 3, cost 36`)
- `0x402c` → `ecx` (`neigh-mask 0x41 = {eax, esi}, deg 2, cost 16`)
- `0x402f` → `ecx` (`neigh-mask 0xc1 = {eax, esi, edi}, deg 4, cost 8`)
- `0x802f` → `edx` (`neigh-mask 0xc3 = {eax, ecx, esi, edi}, deg 4, cost 16`)
- Note the orientation of the diff in `Builder::Process`:
  - **Our compiled binary (`0x10001701`)**: `lea eax, [ebp-1]; push eax; mov ecx, ds:0x10009288; push ebx`
  - **Original binary (`0x10001701`)**: `lea ecx, [ebp-1]; push ecx; push ebx; mov ecx, ds:0x10009288`
  - In our build, both `y - 1` arguments share a single CSE temporary (`0x4026`), which fails colouring in Round 1 across the first `GetPixel` call, is rematerialized via `lea v4026, [ebp-1]` before each call, and in Round 2 receives `eax` at both call sites. In the original binary, the second `y - 1` received `ecx` (`lea ecx, [ebp-1]`), either because the second `y - 1` was a distinct temporary that interfered with `eax`, or because post-colouring local register re-choice (`sub_41c850`) renamed `eax` to `ecx` at the second call site!

### 5.4 Claude (s41, 2026-11-09 12:15): Miner fall-through test + corrections

- **Pruner confirmed by experiment.** Any statement in the odd-case body that survives to switch lowering keeps all 9 odd entries: Miner gets the original **14-entry table** (byte table `0,1,1,2,13,3,13,4,...,12`). Tested: `x = 0;`, `x = y;`, `y = 0;`, `n = 0;`, `n = y;`, `y++;`, `at.x = 0;`, `if (y);`, `do ; while (0);`, `switch (0) {}`. Statements c1 removes first (`x = x;`, `;`, `{}`, `(void)0`, `0;`, `goto` to the end, `return`, `for(;;)break;`, empty inline calls) keep only 9.
- **Placement matters.** With the odd cases as a separate `break` group, the surviving block gets its own epilogue copy (400 B, 39 insns). With the odd cases placed **directly after case 15's code as its fall-through** (remove case 15's `break`, then `case 5: ... case 23: x = 0; break;`), the result is **400 B, 14 insns**: the table and everything up to the dispatch now match, and what remains is the switch-value register (ours ecx, original edx), the `y`/`n` registers in case 1/2, and the sprite-index order. Size is still 400 vs 368 B. Scratch: `build/scratch/s41_claude/min4/m8.py`, variants `min4/v/ft_*`.
- **Why the extra bytes (hypothesis from 5.1):** label merging (0x463380) runs before register allocation, while a dead store disappears only later (copy propagation 0x41aaf0, or coalescing). The odd label therefore stays distinct from the exit label, and the late epilogue copier adds code. The original needs a body that survives the pruner (codegen time) but emits nothing by the time labels are merged. Next to test: empty `__asm {}` blocks, and statements that instruction selection drops.
- **Correction to 5.3 (Builder orientation).** `try.sh` prints ours on the left and the original on the right. **Ours** has `lea eax,[ebp-1]; push eax; mov ecx,g_map; push ebx` at 0x10001701; the **original** has `lea ecx,[ebp-1]; push ecx; push ebx; mov ecx,g_map`, as section 2.1 already says. In our build both `y - 1` arguments are one shared node (0x4026: `lea v4026,[ebp-1]` before each call, from the operand trace at sub_418d50), coloured eax. Node numbering in a colour trace depends on the source file, so 0x402c/0x402f in a test copy may be different nodes.

### 5.5 Claude (s41, 2026-11-09 12:40): APPLIED - Miner 25 -> 14, Basher 27 -> 14 (jump tables now exact)

- **Source change** (`src/process/miner.cpp`, `src/process/basher.cpp`; backups `build/scratch/s41_claude/*.before`): the break-only cases follow the last case group as its **fall-through** and contain a dead store (`x = 0;`) before `break`. Miner: case 15 no longer ends in `break` and runs into `case 5: ... case 23: x = 0; break;`. Basher: the 11..15/27..31 group runs into `case 7: case 9: case 17: case 23: case 25: x = 0; break;`. Basher also computes the sprite index first: `f = (l->dir == -1 ? SPR_DIGHOLE + 4 : SPR_DIGHOLE) + (l->frame & 15); g_map->EraseSprite(list, g_sprites, f, x, y);`.
- **Effect:** both jump tables are byte-identical to the original (Miner 14 entries, Basher 10, every entry on the shared exit), and the next function starts at the original offset. The "400 B / 528 B" that try.sh prints is a window artifact (the following symbol is at the original address). The **whole-DLL difference drops from 33,674 to 3,016 bytes**, because everything after these functions is back at its original address.
- **Verification:** `build.py --funcs PROCESS` 201/206 (no regressions; logs `s41_claude/process_base.log`, `process_after.log`); `mapcmp .data` 0 wrong, `.bss` 46 (pre-existing, same at HEAD); `make` exit 0; Wine: suite `mine` and `bash` MATCH (HD=0, matching their baselines), HD level 1 and HD digger MATCH.
- **What is left (registers only):** Basher is "same shape" (14 insns): the dispatch registers rotate (ours value/index/byte = edx/eax/ecx, original ecx/edx/eax) and the first case's x / frame&15 / g_sprites / list temps swap ecx<->edx. Miner (14 insns): switch value ecx vs edx, y/n in case 1/2, and the sprite-index order. Next: trace both with `neigh_trace_fn.py` + `simplify_model.py` on the new sources.

### 5.6 Antigravity (s59, 2026-11-09 12:41): `Basher::Process`, `Miner::Process`, & `Map::AddTrapType` Simplify/Graph Analysis

- **`Basher::Process` (`0x10001040`, 528 B) — 100% MATCHED (`7/7` in `src/process/basher.cpp`)!**
  - In driver call 2 (`Basher::Process`), the three nodes governing the switch dispatch and `case 2..21` `EraseSprite` setup in the 14-insn baseline were:
    - `0x4034` (`l->frame` CSE across switch dispatch and `l->frame & 15`): initial degree 10 (`4020:esi, 4024:edi, 4022:eax, 8039, 803c, 8044, 8066, 402d, 804b, 804d`), bucket 20 (`0x34 & 0x1f = 20`).
    - `0x8039` (`l->frame - 2` switch index temporary): initial degree 7 (`0005:ebp, 0007:edi, 0006:esi, 4020:esi, 4024:edi, 4034, 4022:eax`), bucket 25.
    - `0x803c` (`x = l->xoff + l->x` in `case 2..21`): initial degree 9 (`4020:esi, 4024:edi, 4034, 8044, 8066, 402d, 804b, 804d, 4025`), bucket 28.
  - **How the 14-instruction register rotation was solved (`0 diff`)**:
    - Just like `Miner::Process`, the original author computed the directional + frame offset into `f` first and added the base sprite index `SPR_DIGHOLE` inside the `EraseSprite` call:
      ```cpp
      f = (l->dir == -1 ? 4 : 0) + (l->frame & 15);
      g_map->EraseSprite(list, g_sprites, f + SPR_DIGHOLE, x, y);
      ```
    - Why this flips `l->frame` (`0x4034`) from `edx` to `ecx` and `x` (`0x803c`) from `ecx` to `edx`:
      - When `f = (l->dir == -1 ? 4 : 0) + (l->frame & 15)` is evaluated, `f` (`0x4022`, which is also used in `if ((f = ++l->frame) >= l->frames)`) is live into `f + SPR_DIGHOLE` inside `EraseSprite`, adding an interference edge `(0x4022, 0x803c)` (and `0x4034`)!
      - As predicted by our 1-edge `simplify_model.py` search (`HIT ADD (4022, 803c): 4034=ecx 8039=eax 803c=edx 402b=ebp`), raising `0x803c`'s degree by 1 causes `0x4034` to be coloured `ecx`, `0x803c` to be coloured `edx`, and `f + SPR_DIGHOLE` to fold into `lea eax, [eax+ecx+0x14e]`, achieving a **100% byte-for-byte match (528/528 bytes, `7/7` functions in `src/process/basher.cpp`)**!

- **`Miner::Process` (`0x10003c60`, 400 B) — 100% MATCHED (`7/7` in `src/process/miner.cpp`)!**
  - In driver call 2 (`Miner::Process`), the key nodes in the 14-insn baseline were:
    - `0x4032` (`l->frame` CSE across switch dispatch and `n = l->frame - 1`): degree 9, bucket 18 -> removed at Step 7, coloured `ecx`.
    - `0x8039` (switch index/table lookup): degree 6 (`{ebp, edi, esi, 4035, 4037, 4032}`), bucket 25 -> removed at Step 0, coloured `eax`.
    - `0x804f` (`y` in `case 1, 2`): degree 4 (`{4032, 4035, 4022, 8047}`), bucket 15 -> removed at Step 6, coloured `edx`.
    - `0x4022` (`x` in `case 1, 2`): degree 5, bucket 2 -> removed at Step 2, coloured `edi`.
  - **How the 14-instruction register rotation + `EraseSprite` argument evaluation were solved (`0 diff`)**:
    1. When the sprite index is computed in a single expression inside `EraseSprite(..., SPR_MINEHOLE + n + (l->dir == -1 ? 2 : 0), x, y + 1)`, C2 pushes `y + 1` and `x` BEFORE evaluating `l->dir == -1 ? 2 : 0`, and colours `l->frame` (`0x4032`) in `ecx` and `y` (`0x804f`) in `edx`.
    2. When the entire sum `n += (l->dir == -1 ? SPR_MINEHOLE + 2 : SPR_MINEHOLE)` is computed into `n` BEFORE `EraseSprite(..., n, x, y + 1)`, `l->dir == -1` is evaluated before `y + 1`, flipping `l->frame` (`0x4032`) to `edx` and `y` (`0x804f`) to `ecx` (`14 -> 6 insns`), **but** C2 then folds the 3-way sum into `lea edx, [edx+eax+0x158]`!
    3. **The Exact Original C Source (`miner_nl_two_step_n.cpp`, applied to `src/process/miner.cpp`)**:
       ```cpp
       n += l->dir == -1 ? 2 : 0;
       g_map->EraseSprite(list, g_sprites, n + SPR_MINEHOLE, x, y + 1);
       ```
       Splitting `n += l->dir == -1 ? 2 : 0;` as a separate statement before `EraseSprite(..., n + SPR_MINEHOLE, x, y + 1)` both evaluates `l->dir == -1 ? 2 : 0` before `y + 1` (giving `l->frame = edx`, `y = ecx`) AND keeps `add eax, edx; add eax, 0x158` as two separate `add` instructions after `push ecx; push edi`, achieving a **100% byte-for-byte match (400/400 bytes, `7/7` functions in `src/process/miner.cpp`)**!

- **`Map::AddTrapType` (`0x10004880`, 6 insns) & `Map::AddObjectType` (`0x10004760`, 8 insns) 16-bit vs 32-bit Analysis**:
  - Tested 9 structural variants in `build/scratch/s59_antigravity/test_map.py` and `test_map2.py`:
    - Whenever `fx` is declared `short` or `WORD` (even with `int current`), C2 does **not** emit `movsx ecx, bx` in the loop preheader, which frees a register and moves `list` from `[esp+0x10]` into `esi`, shrinking the stack frame from `sub esp, 0xc` (288 B) to `sub esp, 0x8` (272 B).
    - Whenever `fx` is `int`, C2 emits `movsx ecx, bx` in the preheader and keeps `sub esp, 0xc` (288 B), matching all 7 register assignments (`eax, ecx, edx, ebx, edi, ebp, esi`), but emits 32-bit `movsx eax, word ptr [esp+0x28]` and `add eax, ecx` instead of 16-bit `mov ax, word ptr [esp+0x28]` and `add ax, cx`.
    - Key clue: in the original loop body (`100048e9..100048f3`), the instructions are:
      `imul bp, ax` -> `movsx eax, ax` -> `add ax, cx` -> `mov [edx-8], bp`.
      Notice that `movsx eax, ax` (`eax = (short)ax`) executes **immediately before** `add ax, cx`! Why would `eax` be sign-extended from `ax` (`movsx eax, ax`) if `ax` is immediately overwritten by a 16-bit `add ax, cx` and the upper 16 bits of `eax` are never read anywhere else?!
      Let's inspect `C2.EXE`'s instruction selection / peephole pass for 16-bit `add ax, cx` after `movsx eax, ax`!
