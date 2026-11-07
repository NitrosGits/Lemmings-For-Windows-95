# Standard Operating Procedure: Multi-Agent Collaboration (Antigravity & Codex)

This document establishes the collaboration protocol, division of responsibilities, and operational safety rules when multiple autonomous AI agents (e.g. Google Antigravity and OpenAI Codex CLI) work together in the *Lemmings for Windows 95* decompilation codebase.

---

## 1. Core Principles

1. **Strict File Ownership & Isolation**:
   - No two agents may edit the same source file simultaneously.
   - Before editing any file in `src/`, an agent must verify that the target is not currently being edited by another agent.
   - Each agent must work in its own dedicated scratch subfolder under `build/scratch/` (e.g. `build/scratch/s40_antigravity/`, `build/scratch/codex/`).

2. **Source Integrity & Two-Way Build Target**:
   - All C/C++ source code must maintain dual compatibility:
     - **Matching target**: Byte-identical under Visual C++ 2.2 (`tools/msvc/try.sh` / `build.py` with `/Ox` under Wine).
     - **Portable target**: llvm-mingw (`make`) with custom CRT (`src/crt`), running without crashes in Wine.
   - Never commit or stage compiler binaries (`build/vc2*`) or game assets.

3. **Single Source of Truth (`HANDOFF.md`)**:
   - `HANDOFF.md` is the shared ledger between all sessions and agents.
   - Any agent beginning work must read `HANDOFF.md` to identify active locks, reservations, and baselines.
   - Any agent completing a phase or making progress must append its session notes to `HANDOFF.md` immediately.

---

## 2. Division of Responsibilities

| Domain | Primary Agent | Scope & Description |
|---|---|---|
| **VC++ 2.2 Matching & Instruction Scheduling** | **Antigravity** | Disassembly matching via `tools/msvc/try.sh`, Pentium U/V pipe scheduling, register tie-breaking, symbol padding sweeps (`padplan.py`), and compiler AST shape analysis. |
| **Isolated Module Decompilation & Scratch Sweeps** | **Codex** | Independent exploration of self-contained modules/functions, automated scratch script generation (`spec.py` permutation runs), and verification log analysis. |
| **Build System & Portable MinGW Runtime** | **Shared / Antigravity** | `Makefile`, import libraries, Win32 API stubs, and CRT portability checks (`make`). |
| **Documentation & Handoff Coordination** | **Antigravity** | Maintaining `HANDOFF.md`, tracking global match counts across all 8 modules (EXE + 7 DLLs), and merging verified diffs. |

---

## 3. Active Work Assignment

To avoid race conditions and merge conflicts, tasks are assigned to distinct files:

* **Antigravity Focus**:
  - `src/process/builder.cpp` (`Builder::Process` 0x10001690, 3 instructions differ).
  - `src/render/render.c` (`Renderer_Render` 0x10001020, 6 instructions differ; `CopyChanged` 0x10004af0, 13 instructions differ).
  - `tools/asset_gui.py` / `tools/asset_tool.py` (Asset Inspector, Level Viewer, Level Player tooling).

* **Codex Focus / Concurrent Agent**:
  - `src/lemmings/levelsel.cpp` (`ShowLevelDetails` 0x40bbe0, 2 instructions differ; `OpenLevelItem` 0x40b3e0, 27 instructions differ).
  - `src/lemmings/gameview.cpp` (`ScrollTo` 0x406490, 18 instructions differ; `SetViewScale` 0x4073e0, 46 instructions differ).

* **Claude (Claude Code) Focus**:
  - `src/process/basher.cpp` (`Basher::Process` 0x10001040, 27 instructions differ).
  - `src/process/miner.cpp` (`Miner::Process` 0x10003c60, 25 instructions differ).
  - `src/process/map.cpp` (`Map::AddTrapType` 0x10004880, 6; `Map::AddObjectType` 0x10004760, 11): repaired on the user's request on 2026-11-07 (see HANDOFF "Claude session 35"); hand back by noting it in HANDOFF.
  - (released 2026-11-07, no edits) `src/lemmings/levelsel.cpp` OpenLevelItem: tried on the user's request, see HANDOFF Claude s35.

* **Naming rule for every agent**: start each HANDOFF.md heading with the agent's name (`### Claude session N`, `### Codex Session N`, `### Antigravity Session N`), only append to HANDOFF.md (never rewrite another agent's lines), and update your own Focus list here before editing a file.

---

## 4. Spawning Codex from Antigravity

Antigravity can launch Codex CLI non-interactively using the wrapper `tools/spawn_codex.sh` or direct CLI execution:

```bash
# Non-interactive Codex execution outside git repo:
codex exec --skip-git-repo-check -s workspace-write -C . "Your prompt here"
```

### Reactive Wakeup Pattern
1. Antigravity invokes Codex as a background command (`run_command` with async task management).
2. Antigravity terminates its turn without spinning in an active polling loop.
3. Upon task completion, the system delivers the result notification, re-activating Antigravity.
4. Antigravity reviews the generated scratch files, validates with `make` and `tools/msvc/try.sh`, and updates `HANDOFF.md`.

---

## 5. Verification Protocol Before Retaining Changes

Before committing or retaining any source change:
1. `tools/msvc/try.sh <MODULE> <SOURCE>` must show equal or fewer differing instructions.
2. `make` must exit with code 0 (zero compiler or linker errors).
3. `python3 tools/msvc/mapcmp.py <MODULE> --sec .bss / --sec .data` must report 0 wrong symbols.
4. `HANDOFF.md` must be updated with the exact instruction difference and session notes.
