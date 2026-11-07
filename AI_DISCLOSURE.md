# AI Assistance & Attribution Disclosure

## Overview

This matching decompilation and portable source reconstruction of **Lemmings for Windows 95** was developed with pair-programming assistance from autonomous artificial intelligence models working in partnership with human reverse engineers.

---

## Participating AI Models & Roles

Three AI systems collaborated across the codebase under human architectural direction and verification:

1. **Google Antigravity** (Google DeepMind):
   - U/V-pipe instruction scheduling and Pentium latency optimization.
   - Register allocation tie-breaking and AST structural formulation for Microsoft Visual C++ 2.2 (`CL.EXE 9.10`).
   - Symbol padding planning (`tools/msvc/padplan.py`) to reproduce compiler internal symbol table counters.
   - Freestanding C runtime portability (`src/crt`) and dual-target build system coordination (`Makefile`).
   - Maintaining the inter-agent collaboration ledger ([`HANDOFF.md`](HANDOFF.md)).

2. **Anthropic Claude** (Claude 3.5 Sonnet / Claude Code):
   - Slicing and functional decompilation of simulation modules (`PROCESS.DLL`, `LOADLVL.DLL`, `READDIB.DLL`).
   - Boundary condition reconstruction, steel clipping logic, and complex state machine translation from Ghidra pseudocode to clean 1990s C/C++.
   - Verification log analysis and hypothesis testing across combinatorial permutation spaces.

3. **OpenAI Codex** (Codex CLI):
   - Exploration of isolated game logic functions and scratch sweeps (`tools/spawn_codex.sh`).
   - Candidate AST shape permutation generation and register lifetime analysis.

---

## Deterministic Verification Protocol

No code generated or suggested by any AI model is accepted on trust. Every function undergoes strict, automated, deterministic verification:

1. **Byte-for-Byte Exact Binary Matching**:
   - Compiled with the original 1995 retail compiler: Microsoft Visual C++ 2.2 (`/Ox`) executed under Wine.
   - Relocations and disassembly compared against original retail binaries via `tools/match.py` and `tools/msvc/try.sh`.
   - Result: **688 of 696 functions (98.85%)** across all 8 modules match the original 1995 release byte-for-byte.

2. **Symbol Table & BSS Placement**:
   - Validated via `python3 tools/msvc/mapcmp.py <MODULE> --sec .bss / --sec .data` to guarantee that 100% of global variables land on their exact byte addresses in memory.

3. **Portable Target Compilation & Wine Execution**:
   - Must compile with zero errors or warnings under modern `llvm-mingw` (`make -j4`).
   - Verified running natively and under Wine without runtime crashes or memory leaks.

---

## Inter-Agent Governance & Collaboration Protocol

To ensure safety, prevent race conditions, and preserve documentation integrity when multiple autonomous agents operate in the repository:
- **Strict File Ownership**: Documented in [`AGENTS.md`](AGENTS.md).
- **Single Source of Truth**: All progress, hypotheses, and session logs are appended sequentially to [`HANDOFF.md`](HANDOFF.md).
- **Dual Build Target Mandate**: Every commit must build cleanly under modern MinGW (`make`) and maintain or improve matching scores under MSVC 2.2 (`tools/msvc/try.sh`).
