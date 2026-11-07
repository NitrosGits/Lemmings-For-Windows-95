# Decompiling a module

Lemmings for Windows 95 (LEMMINGS.EXE + 7 DLLs, Visual Sciences 1995) is being
decompiled into source that builds two ways:

- **Matching**: Visual C++ 2.1 (the original compiler: CL 9.10, LINK 2.55)
  under Wine, `/Ox`, compared byte for byte with the original
  (`python3 tools/msvc/build.py [--funcs] MODULE`).
- **Portable**: MinGW GCC/Clang with our own tiny C runtime (`make`), tested by
  running the game in Wine (`MODS="VSMEM RENDER" tools/wine/run.sh NAME 300 ''`,
  then `python3 tools/cmplog.py build/runs/base-l1.txt build/runs/NAME.txt`).

The original game is in `orig/WINLEMM/`; Ghidra drafts of every function are in
`analysis/MODULE/` (`functions.tsv`, `all.c`, `decomp/ADDR.c`).

## Layout per module

- `src/<module>/*.c` (`.cpp` for C++ files), `objs.txt` (sources in link order:
  the order their code appears in the original), `<module>.def` (export names
  for MinGW), `*.rc` resources.
- `src/include/<module>.h`: the DLL's API. Exported with
  `__declspec(dllexport)` under MSVC, plain declarations for callers (the game
  calls DLLs through import thunks, never `dllimport`), `extern "C"` guards.
- `tools/names/<MODULE>.txt`: addresses of globals and runtime functions
  (`10004000 g_hinst`), so relocations are checked by name.

## The loop

1. `python3 tools/dis.py MODULE.DLL START END` (annotated disassembly) and the
   Ghidra draft; `tools/cstr.py MODULE ADDR` prints strings as C literals.
2. Write the function with its address on the line before it: `/* 0x10001040 */`.
3. `tools/msvc/try.sh MODULE.DLL src/<mod>/file.c [-v] [-f NAME]` compiles and
   compares every annotated function (`-v`: side by side).
   `tools/msvc/variant.py` tries an alternative body without touching the source.
   In `-v` output, the left column is our compiled source and the right column
   is the original; `match.py` prints relocations as `original` and `expected`.
4. When every function matches, the module's data (strings, tables, globals
   with `= 0` where the original had them in .data) and resources must too:
   `tools/msvc/build.py MODULE` reports what differs in the linked file.
   Timestamps and import-table order are expected differences.

## What we know about the original code

- Code after the game's own objects is Visual C++ 2.1's LIBC (not decompiled;
  linked from LIBC.LIB). Each DLL has an unused `DllEntryPoint` + `LibMain`.
- Loops are written with indices (`a[i].x`); the optimiser makes the pointers.
- Unused results were assigned anyway (calls are kept, stores dropped).
- Strings aren't pooled (`/Ox` without `/Gf`): duplicates stay duplicates.
- READDIB says READDIB.CPP but was compiled as C. PROCESS.DLL and the EXE have
  C++ member functions (`this` in ecx): those files must be `.cpp`.
- When a function won't match after a few honest tries, keep the closest
  version, note it in a comment, and move on.

## Toolchain facts (2026-10-18)

- The game was built with Visual C++ 2.2: its import libraries carry all 196 of
  the original's import hints. 2.1 and 2.2 have byte-identical compilers,
  linker and LIBC; only headers and import libraries differ. tools/msvc uses
  build/vc22 and the full `windows.h` (no WIN32_LEAN_AND_MEAN).
- Register allocation depends on the symbol table: how many symbols (globals,
  locals, strings, header declarations) precede a function, and the order its
  locals are declared in. tools/msvc/permute.py searches declaration orders and
  a "symbol padding" count (written as one commented `extern int pad_...;` line:
  the original had more declarations there that we can't know). It scores the
  whole file, so fixing one function can't silently break another.
- The effect of padding is only roughly periodic: cycles of 17 and 32 show up,
  with a slower drift on top. EraseTileTopDown came within 4 instructions at
  15 lines of padding, 2 at 47, and matched only at 79. So every choice is
  checked with a real compile, and searches should cover about 128 lines
  (`--pads 0-127/8` for a coarse pass).
  Workflow:
  1. Find a body that matches at some padding: `tools/msvc/variants.py`
     (alternative bodies, `[[ a || b ]]` choices, `--pads 0-40`, `--apply K`).
  2. Find a local declaration order: `tools/msvc/declsearch.py` (every order, or
     `--sample N`, or `--climb N`).
  3. Plan the padding for the whole file last: `tools/msvc/padplan.py MODULE
     SOURCE --max 128 --write`. It goes through the functions top-down. For
     each, it keeps everything above fixed and adds the smallest padding just
     before it that makes it match.
- A function's code also depends on where the symbols it uses were declared:
  the globals it reads, and the prototypes of the functions it calls. The
  originals had a prototype block at the top of each file (LOADLVL's
  LoaderWrite and StyleGroundFiles only match with one). Padding between the
  headers, the globals and the prototypes (`pad_top...` lines, which padplan
  leaves alone) moves those relative to each other. Find it with a
  two-dimensional sweep: padding before the declaration × padding before the
  function.
- A shared-header declaration can cause a stable compare-operand mismatch even
  when the function's own padding and locals have been exhausted. Confirm the
  global's source uses/xrefs first. Replacing an unnecessary header extern by
  one padding symbol (preserving the header's symbol count), keeping its actual
  file definition, then replanning that file can fix it. This matched PROCESS
  FindLemmings in session 21 and preserved all other functions. Xrefs support
  this experiment; they do not prove the original header's contents.
- Habits of the original author that the compiler shows:
  - Row addresses are computed by reassigning the coordinate first, as in
    `y = dstH - y - 1; d = dst + dstW * y + x;`.
  - Loops are `for (j = 0; j < h; j++)` around `for (i = 0; i < w; i++, dp++, sp++)`.
  - Wrappers adjust their own parameters before passing them on, as in
    `srcX += skipX;` followed by the call.
  - Intermediate values go in their own variables, as in
    `size = ...; p = Alloc(size, 0);`.
  - Arrays are indexed (`ReadLong(lvl->terrain[i])`) rather than walked with
    a pointer the source steps itself.
  - Some locals are declared inside loop blocks.
  - Some values read from WORD fields are held in `int` locals, as in
    `int canvas = s->canvas;`.
  - Slots and canvases are mostly indexed directly (`SLOT_OF(r, slot).width`,
    `g_canvases[canvas].bits`). A pointer is used only where the original
    keeps one in a register.
- `tools/match.py` also reports "N insns differ", counted after aligning the
  two listings. Use it to rank near-misses: once an instruction's length
  differs, every later byte differs too, so byte counts stop being useful.
- Loops: the original often steps a separate pointer (`p += 4`) with a
  down-counter instead of indexing; `if (x) {...} return x;` rather than early
  returns; statement order inside a block matters (e.g. which pointer is set up
  first).
- An intermediate pointer copy can change VC 2.2's register lifetimes even
  when the copy itself is optimized away. RestoreChanged matched after
  `BYTE *canvasBits = g_canvases[s->canvas].bits;`, then the existing `down`
  initializer, then `BYTE *bits = canvasBits;`. This made the bits load use
  `edx` before the stride spill, instead of reusing `eax` after the spill.
  An immediate copy did not help; the later copy's position mattered.
- A small aggregate conversion temporary can leave a stack slot after its
  loads/stores disappear. EXE TreeLoadBitmaps matched by converting SwapRGB's
  COLORREF result through a local `RGBQUAD colour`, then copying four bytes
  into the palette with `memcpy`. This supplies the original extra four-byte
  frame slot and changes the palette-index/bitmap-info register allocation.
  Simply reserving a larger palette buffer reproduced the frame size but
  placed the buffer four bytes too early. This is a matching source shape;
  the original local type cannot be recovered conclusively from the bytes.
- Naming a constant call argument can affect an inlined body even when the
  argument itself folds away. EXE IconWnd::OnCreate matched after passing a
  scoped `int pressed = 1` to `m_skills[0].SetPressed(pressed)`. The inlined
  InvalidateRect HWND load changed from ecx to the original eax; no header or
  padding edit was needed, and the file's previous matches were preserved.
- Explicit local function pointers can reconstruct a compiler's cached-import
  choice when ordinary call/loop spellings cannot. WinMain matched with
  uninitialized pointers declared dispatch/wait/peek before MSG, then assigned
  DispatchMessageA/WaitMessage/PeekMessageA after LoadAccelerators. The message
  loop calls those locals; TranslateMessage stays direct. Both declaration and
  initialization order mattered: an earlier initialized-pointer form left the
  accelerator handle and PeekMessage in swapped registers. The suffix needed
  padding repair. This establishes a matching reconstruction, not proof that
  the original source explicitly declared pointers.
- A file-local forward extern before the shared application headers can
  change compare operand selection without altering the shared header or
  global data order. SetViewCursor matched by declaring g_lastCursor just
  after windows.h, adding 33 padding declarations, then declaring
  g_hSmaBoxCursorGameNew. Seven pads before SetViewCursor and one before
  OnHScroll preserved all earlier matches. Both cursor comparisons now have
  the original operand order. Ordinary local aliases and block-scope externs
  had not fixed them; first declaration position mattered.
- Intermediate declaration order can determine how far a loop initializer
  is hoisted across calls. EXE AboutDlgProc matched with `WORD length =
  (WORD)lstrlen(key); id = 0x400; end = key + length; for (; ...)`.
  Merely naming the length while keeping id in the for initializer did not
  help; initializing id before the lstrlen call also produced the wrong
  schedule. The split call, loop initializer, then pointer addition mattered.
- EXE InitTextBuffer matched with `int i; BYTE *buffer; DWORD offset;`,
  allocating into buffer before assigning the global, then initializing
  offset only after the magic/count stores. The combination plus seven
  preceding padding declarations matched; individual lifetime/type changes
  had not. Replanning the suffix preserved its existing matches.
- A sign-extended character comparison followed by the non-special branch
  and then a special-character branch can be a switch. DrawFrontText matched
  with `switch (*p) { default: ...; break; case '\n': ...; break; }`;
  the if/else form narrowed the comparison to a byte and laid branches out
  differently. This fixed a structural mismatch without a synthetic cast.
- Import order: the compiler lists a file's imports per function, in reverse
  order of first use, and LINK follows that. The originals differ from our
  links only by one swap of the first KERNEL32 slot with a runtime import
  (GetACP/GetCPInfo), probably because they were linked incrementally over an
  earlier build. Treated as a known link-level difference, like timestamps.

## Data layout and the whole-module link (2026-10-20, from LEMMINGS.EXE)

- `python3 tools/msvc/mapcmp.py MODULE [--sec .data|.bss|.text|all]` compares
  the link map's symbol addresses with `tools/names/<MODULE>.txt`, per object.
  `build/scratch/datadiff.py LO HI` shows the original's and our .data side by
  side.
- Every .data aggregate of 8 bytes or more (arrays, struct arrays, structs) is
  8-aligned, whatever its element type. A table that sits 4-aligned in the
  original is a scalar followed by an array (`int g_unused2fc = 0;
  int g_unusedTable[23] = {...};`).
- A file's .data holds its globals in definition order, then each function's
  string literals in function order. A function's static locals are emitted
  with its literals: `static char *key = "Visual";` in CheckSerial put
  "Visual" and the pointer after the previous function's literals, where a
  global of the same name would have come before them all. A file-scope
  `char *const p = "..."` does not fold into the code (it loads the pointer).
- Uninitialised globals (.bss) are ordered within a file by a hash bucket of
  their name (later-defined first within a bucket), not by definition order.
  The hash is only partly understood (see HANDOFF.md).
- Resources: `tools/rsrc2rc.py MODULE.EXE OUT.rc [RESDIR]` decompiles a
  module's resources into an .rc file plus .cur/.ico/.bmp files, in the
  original's statement order; `--cmp ORIGINAL BUILT` compares them. RC 2.x
  rebuilds LEMMINGS.EXE's resources byte for byte (only the directory
  timestamps differ). build.py gives RC the source directory as an include path.

## Checking behaviour, not just bytes

- `tools/msvc/try.sh MODULE SRC --sem` lists, for every unmatched function,
  instructions that differ beyond register allocation and scheduling (field
  offsets, immediates, globals). A "same shape" near miss can be a real bug:
  IconWnd::OnCreate advanced `r.left` instead of `r.top` (3 bytes), which
  left the game with one skill button. The Wine runs found it; `--sem` would
  have too.
- The MinGW build of LEMMINGS.EXE uses `src/crt/exemain.c` (entry point, C++
  constructors and destructors, heap, operator new/delete) and
  `src/crt/stdio.c` (whole-file fopen/fread/fwrite, _fileno, _filelength,
  _unlink, itoa). `MODS="... LEMMINGS" tools/wine/run.sh` runs it.
- More source shapes the compiler shows: a stack frame bigger than ours by a
  RECT means the original measured into a second RECT (CentreWindow,
  OnViewDestroy); WNDCLASS fields are assigned in a fixed order (usually the
  struct's own); display-list entries are added with
  `int n = list->count++; RLEDATA *d; DLENTRY *e = &list->entries[n];`
  and filled through `d = &e->u.rle`; `if (strlen(s)) ok = 1; else ok = 0;`
  keeps the constant's load after the call where `ok = 1; if (!strlen(s))
  ok = 0;` hoists it.

- `tools/msvc/runperm.py MODULE SRC 'signature' [--pads A-B] [--apply]`
  tries every order of each run of consecutive simple statements, with every
  padding. padplan only pads the code as written, so an order that matches only
  at some padding is invisible to it; several functions matched this way
  (EnumUsers, RewindReplay, AnimateTitle). A byte match is the original's
  code, so any matching order is correct.
- `tools/wine/suite.sh` runs level-1 scenarios (each skill, the release
  rate) with the original binaries and with `$MODS` rebuilt, and compares the
  lemming logs. Never while compile searches run (timing).
- **Switch jump-table slots (VC 2.2, probe `build/scratch/s24_intx/probe/`):**
  a group of case labels gets one table slot per *run* of its values (case 5,
  7, 9 … each get a slot; 1 and 2 share one) only if the group's code differs
  from the default's when the table is built. A group whose body is just
  `break` (or `return`, `;`, `if (0) …`, an empty inline call, `x = x`) is
  folded into the default slot, and VC then keeps slots only for some of its
  values. A dead block (`{ int t = …; }`) or a conditional `break` keeps one
  slot per run, but the group then gets its own copy of the epilogue.
  Read a compiled table with `build/scratch/s24_intx/jt2.py OBJ`.

## Register-tie and compare-order facts found in sessions 40-66

- **An intermediate copy of a call result** (`T tmp = f(); T v = tmp;`) or a **cached global in a local** (`int rate = g.rate;` used later) changes the allocator's choices without changing the emitted code shape (RestoreChanged, StartEndScreen, OnStartLevel).
- **Types and declaration order of unrelated locals decide allocation tie-breaks:** CreateFrontDib needed `DWORD w` (unsigned) with `int size` and a non-adjacent reorder of the declarations; the original order and adjacent swaps stayed 3 instructions off.
- **cmp operand order:** with a 16-bit `WORD room` *local* VC emits `cmp room,n`; when the room value is an inline expression (or an int temp cast to WORD at the compare) it emits `cmp n,room; jb` (TreeEnsureVisible). Compares of a global against a register local are always `cmp reg,mem` in the lab, but the original sometimes has `cmp mem,reg` (ApplyOptions): unsolved.
- **Narrowing:** VC narrows an arithmetic expression to 16 bits whenever its destination is a `short`, even an `int` multiply; the original sometimes keeps a 32-bit multiply feeding 16-bit dec/not (ScrollTo): no source form found.
- **Local function pointers** (`LRESULT (WINAPI *p)(...) = SendMessage;` assigned after a given call) pin which import pointers get cached in registers (WinMain).
- **An unused extra local** can reshuffle the whole function's register assignment (IconWnd::WndProc 115 -> 44): the real original probably has another *used* variable.
- A DIFF line with no "N insns differ" in `try.sh` output is a relocation-only difference (identical code): check the names file (`tools/names/LEMMINGS.EXE.txt`) before discarding it.
- Lab method: standalone replica + `tools/msvc/cl.sh /c /Ox` + `i686-w64-mingw32-objdump -d -M intel`.

## Size-mode functions (session 32)

A few originals are size-mode code (VC `/Os` behaviour) inside an `/Ox` build: SoundInit and DockWnd::DockWnd. Signs: a one-instruction-group permutation of the preheader (the compiler's schedule reorder is absent) or an unused `push edi/pop edi`. Test with `tools/msvc/optmode.py MODULE SOURCE ADDR` (`#pragma optimize("t", off)` is enough; `("s", on)` is equivalent). A size-mode function gets no trailing `npad`, so follow it with an empty `__declspec(naked)` function of `_emit` bytes that reproduces the original padding, then re-run padplan for the functions after it (the extra symbol shifts register roles). `#pragma optimize` is an error inside a function body, and pragmas apply to the next function only. Guard it all with `#ifdef _MSC_VER`.

## Block-scoped temporaries (session 32)

Wrapping a single statement of a repeated sequence in `{ BYTE b = expr; *d |= b; }` can change VC's byte-register allocation for the WHOLE function (both arms of an `if` share the allocator state). BuildCollision's scale-1 arm went from a spill (32 insns off) to 2 off with one such block on its last bit. Search by sweeping which positions get a block temporary (shapes.py with generated alternatives, 2^n variants), then re-run padplan for the functions after it (the extra identifier shifts them).

- **Inline member bodies are expanded at the end of the translation unit** (session 34/33): their
  register choices depend on the file-wide symbol count and on the locals declared inside each inline
  body. Tune per file without disturbing other files by giving the shared inline a macro hook
  (`#ifdef LAYOUT_PAD / LAYOUT_PAD / #endif` in window.h's `Dock::Layout`) that only one .cpp
  defines, as dummy `int` locals. `tools/msvc/search/inlinepad.py` sweeps dummy-local count x
  padding before a function (DockWnd::Drag matched with 3 locals + 13 pads).

## Expression-context lowering of `?:`, and retained dead uses (session 36, WaveTestVolume)

How VC 2.2 compiles a clamp whose result lives in a callee-saved register:

- `n = c ? a : b;` (or `if (c) n = a;`) assigned DIRECTLY to a register variable is lowered per arm:
  `cmp eax,100; mov esi,eax; jl L; mov esi,100`. The same ternary used as an expression VALUE (the
  controlling expression of a `switch`, e.g. `switch (n = devs >= 100 ? 100 : devs) { case 1: break; }`)
  is computed in eax and stored once at the join: `cmp eax,100; jl L; mov eax,100; L: mov esi,eax`.
  That second shape is what the original WaveTestVolume has.
- Whether `n` keeps its register at all depends on a later use surviving until register allocation.
  An empty `switch (n) { case 1: break; }` survives (the dispatch is removed by the late jump-to-next
  peephole); `if (n) ;`, `n;` and `(void)n` are removed in the front end; `if (n > 100) ...` after a
  clamp is range-folded late for `if`-form clamps but early for ternary ones. A ternary whose result is
  unused keeps its COMPARE/branch/`mov eax,imm` code but loses the store; an unused `if`-form clamp is
  deleted entirely.
- A ternary assigned to a separate temp and then copied (`t = ...; n = t;`) is coalesced back into
  per-arm stores; types (short, WORD, unsigned...) and block-scoped temps do not prevent it, inline
  helpers expand their ternary at the end of the function (lazily, at the use), and
  `#pragma optimize` modes do not change the choice.
- Method that found it: `tools/msvc/asmshapes.py MODULE SOURCE ADDR NAME SPEC.py START_RE END_RE`
  compiles every combination of a spec and groups them by the exact instructions OUR build emits
  between two markers, so the set of shapes the compiler CAN produce for a construct is visible and you
  can see which construct family is close to the original's shape (the original's is listed last).
  `tools/msvc/showvariant.py` prints one combination's full side-by-side. A group with no rows at all
  means that combination MATCHES (try.sh prints no diff rows for a matched function).
- When the stripped original code is unknowable (the neighbouring `WaveSetVolume` is an empty body),
  placeholders like these empty switches are acceptable; keep them commented as stand-ins.

## Preventing `if/else` constant-branch hoisting and empty-switch evaluation ordering (session 37)

- **Preventing `if/else` constant-branch hoisting via common statement hoisting (`Bitmap::BuildRows`)**:
  When one arm of `if (cond)` is a single constant assignment (`offset = 0;`), VC 2.2's early
  if-conversion pass turns `if (cond) { ... } else offset = 0;` into `offset = 0; if (cond) { ... }`
  and hoists `mov reg, 0` above earlier calls. Placing a preceding assignment (`pitch = m_width;`)
  inside BOTH arms of the `if/else` makes both arms multi-statement during if-conversion (blocking
  the constant hoist), and a later common-head hoisting pass hoists `pitch = m_width;` out of both
  arms to right before the `cmp`, leaving `xor edi, edi` in the `else` branch.
- **Empty-switch (`switch (expr) { case 1: break; }`) live-range and basic-block splitting**:
  Each empty `switch` adds **5** internal `$L` label symbols to VC 2.2's symbol table and is stripped
  at the very end by the jump-to-next peephole:
  - **Multi-variable evaluation before a call (`BuildCollision`)**: `switch (w - h) { case 1: break; }`
    at the top of a block forces VC 2.2 to load both `w` (`ebp`) and `h` (`edi`) into their
    callee-saved registers before pushing the arguments of a following call (`VSMEM_Allocate`),
    using only 1 switch (+5 symbols instead of +10 for two switches).
  - **Extending a local's call-crossing live range (`ApplyOptions`)**: placing `switch (size) { case 1: break; }`
    before `return 1;` at the end of a function extends `size`'s live range across trailing calls
    (`InGame()`, `SetZoom()`), raising its call-crossing weight above a cached import pointer
    (`IsDlgButtonChecked`) so `size` gets `edi` and the import pointer gets `ebx` even when `size = 0;`
    is initialized late.
  - **Splitting a basic block to avoid struct spills (`ReadIniSettings`)**: when two `PtInRect(&r, pt)`
    blocks both assign `pt.x` before `pt.y` in the same basic block as the call, VC 2.2 spills `pt.y`
    to the stack in both blocks. Inserting `switch (pt.x) { case 1: break; }` between `pt.x = ...; pt.y = ...;`
    and `if (... && !PtInRect(&r, pt))` splits the basic block and keeps `pt.x` (`eax`) and `pt.y` (`ecx`)
    in registers with zero spills.
  - **Forcing subexpression CSE into `eax` after a division (`StretchChanged`)**: when `row = changed + cy0 * across;`
    immediately follows `sy = (h / 2 + height) / h;`, VC 2.2 allocates `ecx` for `cy0 * across` and schedules
    it into the `idiv` sequence. Placing `switch (cy0 * across) { case 1: break; }` between `sy = ...;` and
    `row = changed + cy0 * across;` makes `cy0 * across` a common subexpression evaluated in `eax` after the
    `sy` store, which `row = changed + cy0 * across;` then adds to `ebx` (`add ebx, eax`).
