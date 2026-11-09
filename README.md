# Lemmings for Windows 95 — Decompilation

A matching decompilation and portable source reconstruction of **Lemmings for Windows 95** (originally developed by Visual Sciences and published by Psygnosis in 1995).

The project achieves **dual-target compatibility**:
1. **1:1 Matching Target**: Generates byte-identical binaries when compiled with the original compiler (Microsoft Visual C++ 2.2 with `/Ox`).
2. **Portable Target**: Modern cross-compilation with llvm-mingw / GCC targeting 32-bit Windows using a minimal freestanding custom C runtime (`src/crt`), playable natively on modern Windows and under Wine / Proton.

---

## Progress & Status

Across the entire game (1 executable + 7 DLLs), **all 696 functions (100%)** are byte-matched:

| Module | Purpose | Functions Matched | Status |
|---|---|---|---|
| **`LEMMINGS.EXE`** | Main UI, windowing, menus, level selector | **334 / 334 (100.0%)** | **100% BYTE-IDENTICAL** |
| **`FILEHAND.DLL`** | File I/O and archive resource extraction | **7 / 7 (100.0%)** | **100% BYTE-IDENTICAL** |
| **`GAMEOVER.DLL`** | Game over animation and result handling | **3 / 3 (100.0%)** | **100% BYTE-IDENTICAL** |
| **`LOADLVL.DLL`** | Level file format parser and terrain unpacker | **41 / 41 (100.0%)** | **100% BYTE-IDENTICAL** |
| **`READDIB.DLL`** | DIB / BMP bitmap loading and palette parsing | **17 / 17 (100.0%)** | **100% BYTE-IDENTICAL** |
| **`VSMEM.DLL`** | Custom sub-allocator and memory manager | **11 / 11 (100.0%)** | **100% BYTE-IDENTICAL** |
| **`RENDER.DLL`** | High-performance 2D blitter and dirty-rect engine | **77 / 77 (100.0%)** | **100% BYTE-IDENTICAL** |
| **`PROCESS.DLL`** | Lemming physics, state machine, skill actions, terrain collision | **206 / 206 (100.0%)** | **100% BYTE-IDENTICAL** |
| **Total** | | **696 / 696 (100.0%)** | **8 / 8 Modules 100% Matched** |

"Byte-identical" here means each function's compiled code, checked with `tools/msvc/build.py --funcs`. The linked EXE and DLL files are not yet identical as whole files: headers, import tables and some relocated addresses still differ.

---

## Architecture & Module Breakdown

* **`LEMMINGS.EXE`**: The Win32 application entry point (`WinMain`). Manages the main game loop, high-resolution timer synchronization, sound mixer trigger calls, dialog boxes, and child dock windows (`MapView`, `IconView`, `CounterView`).
* **`RENDER.DLL`**: Software blitting engine designed by Visual Sciences. Supports full-screen scaling, clipped and masked RLE sprite blits, background terrain composition, and dirty-rectangle tracking (`MarkChanged`, `CopyChanged`, `StretchChanged`).
* **`PROCESS.DLL`**: Lemming gameplay simulation. Contains class implementations for each lemming skill (Walker, Faller, Floater, Blocker, Climber, Builder, Basher, Miner, Digger, Exploder), physics calculation, and destructive bitmap terrain deformation.
* **`LOADLVL.DLL`**: Reads `.LVL` binary level archives, extracts section chunks, decompresses level geometry, and instantiates trap/exit object instances.
* **`READDIB.DLL`**: Parses Win32 Device Independent Bitmaps (DIB), handles 8-bit palette conversion, and produces raw surface buffers.
* **`VSMEM.DLL`**: Visual Sciences custom pooled memory allocator tracking allocations by source file and line tag.
* **`FILEHAND.DLL`**: Low-level Win32 file streaming and handle abstraction layer.
* **`GAMEOVER.DLL`**: Handles win/loss end sequences and tally animations.

---

## Repository Structure

```
├── src/
│   ├── include/       # Shared headers and DLL interfaces
│   ├── crt/           # Lightweight freestanding C runtime stubs for MinGW
│   ├── lemmings/      # LEMMINGS.EXE source code and resources
│   ├── process/       # PROCESS.DLL source code (C++)
│   ├── render/        # RENDER.DLL source code (C)
│   ├── loadlvl/       # LOADLVL.DLL source code
│   ├── readdib/       # READDIB.DLL source code
│   ├── vsmem/         # VSMEM.DLL source code
│   ├── filehand/      # FILEHAND.DLL source code
│   └── gameover/      # GAMEOVER.DLL source code
├── orig/
│   └── WINLEMM/       # Original binary files (not tracked; user provided)
├── tools/
│   ├── match.py       # Instruction and relocation comparison engine
│   ├── msvc/          # Scripts for MSVC 2.2 compilation, padding sweeps & c2dbg tracers
│   └── impdef.py      # Import library generation helper
├── docs/
│   ├── DECOMP.md      # Detailed reverse-engineering methodology & compiler notes
│   └── C2_REGALLOC.md # Annotated map of C2.EXE graph-colouring register allocator
├── Makefile           # Portable MinGW build system
└── HANDOFF.md         # Detailed decompilation ledger & session logs
```

---

## Building the Project

The project builds on Windows, macOS and Linux. On Windows the game and the original compiler run directly. macOS and Linux use Wine for both.

| | Windows | macOS | Linux |
|---|---|---|---|
| Portable build (`make`) | MSYS2 MINGW32 shell | MinGW-w64 cross compiler | MinGW-w64 cross compiler |
| Running the game | directly | Wine | Wine |
| Matching build (Visual C++ 2.2) | directly | Wine | Wine |

### 1. Prerequisites

- **Windows**: [MSYS2](https://www.msys2.org). In its **MINGW32** shell, install the 32-bit GCC, make and Python:
  ```bash
  pacman -S make mingw-w64-i686-gcc python
  ```
  A python.org Python on the `PATH` works too. [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) also works: put its `bin` folder on the `PATH` in an MSYS2 shell. No Wine is needed.
- **macOS**: a MinGW-w64 cross compiler (`brew install mingw-w64`, or [llvm-mingw](https://github.com/mstorsjo/llvm-mingw)), GNU `make` and Python 3. Wine (for example Homebrew's `wine-stable` cask) is needed to run the game and the matching compiler.
- **Linux** (Debian/Ubuntu names): `make`, `python3`, `gcc-mingw-w64-i686`, `g++-mingw-w64-i686`, and a Wine that can run 32-bit programs.

The matching tools disassemble with a **GNU** `objdump`. MinGW-w64 GCC, MSYS2 and Linux binutils all provide one. llvm-mingw's `objdump` is LLVM's and does not work, so with llvm-mingw alone, install binutils or set `OBJDUMP` to a GNU objdump.

### 2. Portable Build (Modern Toolchain)

The portable build links against our freestanding CRT (`src/crt`) and needs no MSVC runtime DLLs:
```bash
# Build all DLLs and LEMMINGS.EXE into build/out/
make -j4
```
On macOS and Linux the Makefile uses the `i686-w64-mingw32-` cross tools. On Windows it picks MSYS2's plain `gcc`/`dlltool` names automatically. Use `make CROSS=<prefix>` for other tool names.

**Prebuilt binaries:** every push to `main` runs the [Windows build workflow](.github/workflows/windows-build.yml) on GitHub Actions. It builds the binaries with llvm-mingw and checks that the native MSYS2 MINGW32 build still works. Download `LEMMINGS.EXE` and the 7 DLLs from the `lemmings-win32` artifact of a run on the repository's **Actions** tab. They still need the original game's data files: copy them over an original installation.

### 3. Playing the Game

To run the game, the original game files (`.DAT`, `.WAV`, `.MID`, `.LVL`) are required.

1. Place the original game directory in `orig/WINLEMM/` (or copy the built files over an existing installation).
2. Copy the original game data, with the freshly compiled binaries over it, into `build/game/`:
   ```bash
   make game
   ```
3. Run it:
   - **Windows**: run `build/game/LEMMINGS.EXE` directly.
   - **macOS / Linux**:
     ```bash
     wine build/game/LEMMINGS.EXE
     ```

### 4. Matching Build Verification (Visual C++ 2.2)

The matching build compiles the source with the original compiler and compares the result with the 1995 binaries in `orig/WINLEMM/`.

**Setup:** the compiler is not part of this repository. Copy the `BIN`, `INCLUDE` and `LIB` folders of your own Visual C++ 2.2 into `build/vc22/MSVC20/`, or set `VCDIR` to the folder that holds them.
- **Windows**: `CL.EXE`, `LINK.EXE` and `RC.EXE` run directly.
- **macOS / Linux**: they run under Wine, in their own prefix `build/wine-vc` (created on first use; `VCPREFIX` changes it).

```bash
# Every module: compile, link and compare the whole files
python3 tools/msvc/build.py

# One module, also comparing each function
python3 tools/msvc/build.py --funcs PROCESS

# One file
python3 tools/msvc/try.py PROCESS.DLL src/process/map.cpp

# One function, with a side-by-side assembly diff
python3 tools/msvc/try.py PROCESS.DLL src/process/map.cpp -v -f Map::AddTrapType
```
On Windows, use `python` or `py -3` if `python3` is not available. On macOS and Linux, `tools/msvc/try.sh` is the same command as `try.py`.

Tested on Windows 11 (ARM64, in a VM) with a python.org Python and llvm-mingw: the matching build gives the same results as on macOS, and `make` produces the same binaries apart from their timestamps. The MSYS2 MINGW32 GCC and MSYS2-Python routes have not been tried on Windows yet. The research scripts are still macOS/Linux only (zsh and Wine): the search and sweep scripts in `tools/msvc/`, the `tools/msvc/c2dbg/` debugger tools and the gameplay test harness `tools/wine/run.sh`.

---

## Decompilation Tooling & AI Methodology

This matching decompilation was achieved using a hybrid workflow combining industry-standard reverse engineering software, a custom Python-based verification and search harness, and autonomous AI-assisted pair programming.

### 1. Reverse Engineering & Disassembly Tooling
- **Ghidra**: Used for initial binary control-flow analysis, structure and type recovery, calling convention detection (`__thiscall` vs `__cdecl` / `__stdcall`), and generating initial draft C pseudocode.
- **Hopper Disassembler & REA**: Used for basic-block slicing, control flow graph validation, and cross-referencing global variable access patterns.
- **GNU Binutils (`objdump`, `nm`) & Capstone Engine**: Used for low-level inspection of COFF object relocations, symbol tables, and 16/32-bit instruction decoding.

### 2. Custom Python Automation Toolchain (`tools/`)
Because 1995 Visual C++ 2.2 (`c2.dll`) code generation is highly sensitive to Pentium U/V-pipeline scheduling, AST shape, and symbol table layout, a specialized suite of Python tools was created:
- **`tools/match.py`**: Relocation-aware disassembly matching engine that compares compiled COFF objects directly against original PE sections, highlighting instruction, register, and displacement mismatches side by side.
- **`tools/msvc/msvc.py`, `try.py` & `build.py`**: Compilation harness executing the authentic 1995 Visual C++ 2.2 compiler (`CL.EXE 9.10`) with `/Ox` optimization: directly on Windows, under Wine on macOS and Linux (`try.sh` and `cl.sh` are zsh front ends for them).
- **`tools/msvc/declsearch.py` & `runperm.py`**: Automated combinatorial search scripts that test local variable declaration order permutations to resolve register allocation tie-breaking in `c2.dll`.
- **`tools/msvc/padplan.py` & `padsweep.py`**: Symbol-table padding sweep tools (`extern int pad_...;`) that reconstruct unexported symbol table hashes and align internal compiler symbol counters without altering executable code semantics.
- **`tools/msvc/mapcmp.py`**: Symbol map comparator ensuring that 100% of `.bss` and `.data` global variables land on their exact byte-level addresses in the original binary layout.
- **`tools/msvc/c2dbg/` (`colour_trace_fn.py`, `neigh_trace_fn.py`, `simplify_model.py`)**: Winedbg debugger harness and exact Python Chaitin-Briggs graph-colouring simulator for `C2.EXE` (`sub_417690` / `sub_417580`).
- **`tools/dis.py` & `cstr.py`**: Automated tools for dumping annotated target assembly and extracting null-terminated string literals.

### 3. AI Assistance & Autonomous Agents
This repository was developed with AI assistance under human architecture direction and verification:
- **Agents Involved**: **Google Antigravity**, **Anthropic Claude**, and **OpenAI Codex CLI**.
- **Agent Roles**:
  - Analyzing complex x86 assembly idioms and translating Ghidra pseudocode into idiomatic 1990s C/C++.
  - Formulating hypotheses on Pentium U/V dual-pipe instruction pairings and AGI (Address Generation Interlock) stalls.
  - Designing automated Python search specs to explore localized syntax permutations.
  - Maintaining dual compatibility with both MSVC 2.2 byte-matching and modern `llvm-mingw` portable compilation.
- **Deterministic Verification**: No AI-generated code is accepted on trust. Every function is strictly validated against deterministic ground truth:
  - **Byte-Identical Matching**: Compiled under original Microsoft Visual C++ 2.2 (`/Ox`) under Wine and compared byte-for-byte against the 1995 retail binaries (`tools/match.py`, `tools/msvc/try.sh`).
  - **Memory Layout & Symbol Placement**: Validated with `tools/msvc/mapcmp.py` to guarantee exact `.bss` and `.data` symbol address correspondence.
  - **Portable Compilation**: Tested via clean compilation under modern `llvm-mingw` (`make -j4`) with custom freestanding CRT (`src/crt`), running natively and in Wine.
- **Collaboration Protocol & Ledger**: Multi-agent collaboration boundaries and safety rules are governed by [`AGENTS.md`](AGENTS.md). For detailed disclosure on AI methodology and verification protocols, see [`AI_DISCLOSURE.md`](AI_DISCLOSURE.md). The complete technical log of all sessions, investigations, and breakthroughs is documented in [`HANDOFF.md`](HANDOFF.md).

---

## Licensing & Legal Disclaimer

### Code License
The reverse-engineered C and C++ source code in this repository is licensed under the **[MIT License](LICENSE)**.

### Game Assets & Trademarks
- **No Assets Included**: This repository contains only decompiled source code and build recipes. It does **not** distribute any proprietary game assets (sprites, textures, sounds, music, or level data). A legally acquired copy of the original retail game is required to play.
- **Trademarks**: *Lemmings* is a registered trademark of Sony Interactive Entertainment Europe / Psygnosis / DMA Design. Visual Sciences is the original developer of the Windows 95 port. All trademarks and registered trademarks belong to their respective owners.
- **Purpose**: This project is maintained strictly for educational, research, preservation, and historical study purposes.
