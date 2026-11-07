# C2.EXE register allocation: annotated map (VC++ 2.2, partial, partly traced)

Source: static reading of `build/vc22/MSVC20/BIN/C2.EXE` (sha256 38d0bbad...4233) with REA + Hopper, then
**dynamic checks with winedbg** (Claude session 37, 2026-11-07; recipe in `tools/msvc/c2dbg/README.md`).
Addresses are virtual addresses in C2.EXE (stripped; names are mine). **Observed** = read from disassembly or seen
in the debugger. **Inferred** = my reading, not confirmed. The traces ran the real compiler on `levelsel.cpp`
(`/Ox`, `-G4`, i.e. the project's normal flags) and on a reduced copy holding only `OpenLevelItem`.

## Where the code lives

Assert strings give the source files: `color.c` v1.101 (string 0x47a2bc), `regasg.c` v1.25 (0x47a2f8), also
`peeps.c`, `sched.c`, `schedp5u.c`, `range.c`, `nglobal.c`, `nglobreg.c`. `color.c` code is 0x417260-0x41e440.

## CORRECTION: which code actually runs  -- observed (debugger)

Breakpoints on these entry points during the `levelsel.cpp` compile (first 369 stops, then a full run):

| never hit (process ran to termination) | hit |
|---|---|
| sub_41daf0 (my earlier "picker"), sub_41e360, all of `regasg.c` (sub_41f3b0..41f600), sub_41f490, sub_41f410 | sub_418d50 (162x), sub_41d960 (102x), sub_41da20 (49x), sub_41e570, sub_41d140, sub_41d4e0, sub_41cea0, sub_41cd80, sub_41c1a0, sub_41c850, sub_41a100, sub_41a650, sub_41b940, sub_417c80 |

So the first version of this document (a per-block "lowest free register" picker in sub_41daf0/sub_41e360 steered by
regasg.c look-ahead) described code that is **not executed with the project's flags**. Do not rely on it.

## What the executed code is  -- observed unless marked

* **Driver sub_417260** (once per function): `[477280]` = 6 (7 if frame pointers are kept) is the number of colours `K`;
  retry loop (<= 10 rounds) of node-tree walk / sub_4179a0 / spill (sub_417c80); then sub_41c170 -> sub_41c1a0 (below).
* **Graph colouring with coalescing (inferred from sub_41a100):** nodes are virtual registers (ids > 0x1f) kept in 32 hash
  buckets at 0x477320 (lookup `sub_419870(id)`, key `id>>7`, bucket `id&0x1f`). sub_41a100 merges two nodes joined by a copy
  when the combined weight (`word [node+0x14]`, `word [node+0x16]`, bit 0x1000/0x800 flags, root id in `word [+0x1a]`)
  is not above `K`; sub_417c80 spills (flag 0x40) and rewrites uses. The colour of a node ends up in `[node+0x1c] & 0x3f`.
* **Apply colours: sub_418d50(ecx = instruction, edx = operand slot)**: replaces a virtual register id in an operand by its
  physical register via the node table (two `sub_419870` lookups, `& 0x3f`), validated by sub_424530 (register legal for
  the operand size; assert line 0x6b0) and sub_424370. Hit 162 times per compile (every operand).
* **sub_41d960(ecx = register) / sub_41da20(ecx = register): acquire / release.** Acquire sets the register's bit in
  the busy mask `dword_477318` and links the node into the per-register list at 0x4774a0+reg*4; release unlinks it and
  clears the bit.
* **sub_41c1a0 (recursive over blocks)**: at each block start `word_476f40 = word_4770b0 = 0x1a` (no latest-value
  register), `[477318] = 0`, clears the tables at 0x4774a0 and 0x476fe4, marks esp (`0x10`) and, without a frame pointer
  (`[47727c] == 0`), ebp (`0x20`) busy, plus registers live into the block. State is per block, not across functions.
* **Instruction-level register rewrite pass, sub_41c850 (per instruction) with sub_41cd80, sub_41cea0, sub_41d140**
  (the code at 0x41c95b-0x41caa8 is a *re-choice*, not the first assignment):
  * sub_41cd80(ecx = first node, edx = last node) returns a mask of the registers used or forced (via the opcode
    table sub_4111b0 and tables 0x47bf18/0x47a820/0x47b4d0) by the instructions between a value's definition and its use.
    Observed values at 0x41c95b in the `OpenLevelItem` trace: `ffffff11`, `ffffff12`, `ffffff13`, `ffffff16`, `ffffff17`,
    `ffffff19`, `ffffff51` (bit n = register n; 0 = eax, 1 = ecx, 2 = edx, 3 = ebx).
  * sub_41cea0 builds the ranked candidate list `word_477260[0..7]`: hints first (sub_41d140: a register the related operand
    or consumer already uses; registers in the 0x476fe4 table that hold the same value, via sub_41e570), then registers
    0..7 in ascending order excluding that clobber mask and the latest-value registers `word_476f40/4770b0`. Observed
    lists: `1,2,3,4,6,7` (eax excluded), `1,2,3,4,5,6,7`, `6,3,4,5,7` and `3,2,5,4,7,6,1,0` (hint-first).
  * 41c850 then takes the first candidate that is free in `[477318]` and acceptable to sub_424530 and rewrites the
    operand (`mov [node+0x14], reg`), releasing the old register with sub_41da20; assert line 0x162d when no candidate fits.
  * Inferred role: renaming a temporary to a different register to break a dependency before scheduling (the Pentium
    pairing passes sched.c/schedp5u.c run after it). This is the closest place I found to the "ecx/edx instead of eax"
    behaviour, but I have **not** shown that it makes the choice for any of the six known misses.

## Not understood yet

* Which function assigns the colour of a short-lived temporary the first time (candidate: the colouring order code around
  sub_417580 / sub_417690, called from sub_417260, and sub_41a650's large switch whose jump tables Hopper lost).
* How the colour index maps to the physical register (likely eax=0, ecx=1, edx=2, ebx=3, esp=4, ebp=5, esi=6, edi=7 from the
  masks above).
* Nothing here has been connected to a specific source construct yet.

## Next step that would settle it

Trace `OpenLevelItem` (reduced file `build/scratch/s37_dbg/red/levelsel.cpp`, 4 insns differ vs 2 in the full file, same
shape): break on sub_418d50 and dump the operand's old and new register for each hit; the hit whose result is `eax` for
`mov eax,[esp+0x34]` is the `sel` temporary. Then break on sub_41d960/41da20/41cea0 earlier in the same block to see which
register was marked busy or excluded when `sel` was coloured, and compare with the original (edx).

## UPDATE (same session): the colour assignment itself  -- observed (disassembly + debugger)

* **sub_417580 assigns the colour of a node.** For the node taken from the work list it ORs `1 << colour` of every already
  coloured neighbour (list at `[node+8]`, neighbour node in `[entry+4]`, colour in `[node+0x1c] & 0x3f`, 0x20 = uncoloured),
  then walks the preference table of dwords at **0x47a280 = eax, ecx, edx, esi, edi, ebx, ebp** (K = `[477280]` entries,
  7 with frame pointers omitted) and takes the first colour whose bit is clear. It records the colour in `[node+0x1c]`, sets
  bit `colour` in `[4770b8]` (registers used: callee-save accounting; ebp sets `[477270]`) and adds the node weight to
  `[4770b8+8+colour*4]`.
* **sub_417690 orders the nodes** (Chaitin/Briggs simplify): repeatedly removes a node whose degree `[node+0x14]` is below K,
  decrementing its neighbours' degrees; if none qualifies it picks a spill candidate by cost (`[node+0x16]` cost word,
  compared through `[node+ecx*4+0xc]`). Colouring happens in the reverse of the removal order (stack).
* So a temporary's register is **the first of eax, ecx, edx, esi, edi, ebx, ebp that none of its coloured neighbours
  has**. A temp gets edx instead of eax only when an eax-coloured and an ecx-coloured node interfere with it (are live at
  the same time) and were coloured first.
* **Trace of our `OpenLevelItem` `sel` temp** (reduced file, last round for that function): virtual register `4020`
  (the `mov v4020,[esp+0x34]` seen at sub_418d50 hit 411), cost 30000 (unspillable short-lived temp), degree 3,
  **neighbour colour mask `e0` = ebp|esi|edi only**, colour **eax**. Its three neighbours are the three register-variable
  locals; no eax/ecx-coloured value is live across its definition. The original (edx) therefore has both an eax-coloured and an
  ecx-coloured value live at that point (a live range that overlaps `sel`'s definition in c2's order *before* scheduling).
  Which source construct produces such live ranges is still unknown; this is the target to hit.
* Tools: `tools/msvc/c2dbg/colour_trace.py SRC.cpp` prints this table for any source (id, neighbour mask, colour, degree, cost).
* Variants tried against this reading (all scratch, `build/scratch/s36_claude/oli_store.py`): moving the `shown` store into
  the call arguments (`(store, sel)`, `(store, list)`, `(store, 9)`, `(store, &g_treeNext)`), store before/after the
  `switch(value)`, store after the call: 3 (with the switch) or 4 (without) or 9; production stays 2.

## UPDATE 3: what the node ids mean (decoding the 80xx nodes)  -- observed unless marked

Each interference-graph node has a 16-bit id; the class is in the top bits (`test bh,0xc0` = "virtual"):

| id | what it is | evidence |
|---|---|---|
| 0x00-0x0d | the machine-register nodes (sub_4178d0 creates ids 0..13); ids 0-7 are eax,ecx,edx,ebx,esp,ebp,esi,edi with a fixed colour | `colour_trace`, 56 refs to 0x3/0x4/0x5 in Builder |
| 0x18 | memory/frame pseudo-register (driver calls sub_4198c0(0x18)) | driver, sub_41c850 special cases |
| **0x4xxx** | **named virtual registers: the IL's variables and compiler variables** (locals, parameters, register candidates). Their node records carry a size byte at `+0x28`, words at `+0x24/+0x26` and pointers at `+0x2c..+0x34` (declaration data). In Builder: `4025 esi, 4028 edi, 402b ebx, 402d ebp` are the register variables, colour first (deg 10-23, cost 16-162). | node dumps (`dbg30`) |
| **0x8xxx** | **anonymous expression temporaries made during instruction selection**: every intermediate value (a load, a `lea`, a pushed operand, a zero-extended WORD) gets one. Node records are all zero (no declaration), created lazily by sub_4178f0 on first reference, tiny degree (2-4) and cost (0-15); they are coloured last and take what is left. Example in `OpenLevelItem`: `mov v80c9,[mem]; and v80c9,0xffff; push v80c9`. | 418d50 operand trace, node dumps |

Both classes are referenced the same way (sub_419b10 -> sub_4178f0 -> sub_4198c0 creates the node and adds the use weight to its
cost; 0x7530 = 30000 means "never spill"). sub_4198f0 (the node declarator, called from 0x439d70, 0x43b7a0, 0x43bd40, 0x43d260,
0x459100) was not executed in these compiles. Interference edges are added by sub_419f90 (called from sub_41a410, which sub_41a100
calls) and bump both nodes' degree `[+0x14]`.

**Ordering lever (inferred from sub_417690 and sub_419870, partly tested):** nodes live in hash buckets `[0x477320 + class_offset +
(id & 0x1f)*4]` (class offsets 0x00 / 0x80 / 0x100 / 0x180 for ids below 0x4000 / 0x4xxx / 0x8xxx / 0xCxxx). The simplify pass
(sub_417690) builds its candidate list by walking the 0x4xxx buckets first, then the 0x8xxx buckets, bucket 0..31 in order, each list
newest-first. So which temp is removed, and therefore coloured, early or late depends on `id & 0x1f`, and ids are assigned in IL
order. Anything that renumbers ids (unused locals, extra expression nodes, statement order) can reorder the colouring. Test: adding
k unused locals to `OpenLevelItem` (`build/scratch/s36_claude/oli_shift.py`, `oli_shift2.py`, 40 builds, k = 1..40) changes the output
non-monotonically between three states (2, 5 and 92-98 differing instructions; k = 1,6,7,8,14,22-26,30-33,38-40 stay at the
baseline 2; 9,10,15-18,34 give 5; 11-13,19-21,27-29,35-37 give 92-98), so numbering does matter, but none of these 40 values beats 2.
