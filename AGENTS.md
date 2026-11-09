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
  - `src/process/miner.cpp` (`Miner::Process` 0x10003c60, MATCHED 7/7 functions).
  - `src/process/basher.cpp` (`Basher::Process` 0x10001040, MATCHED 7/7 functions).
  - `src/process/builder.cpp` (`Builder::Process` 0x10001690, 3 instructions differ).
  - `src/process/map.cpp` (`Map::AddTrapType` 0x10004880, 6; `Map::AddObjectType` 0x10004760, 8).
  - `tools/asset_gui.py` / `tools/asset_tool.py` (Asset Inspector, Level Viewer, Level Player tooling).

* **Codex Focus / Concurrent Agent**:
  - `src/lemmings/levelsel.cpp` (`ShowLevelDetails` 0x40bbe0, 2 instructions differ; `OpenLevelItem` 0x40b3e0, 27 instructions differ).
  - `src/lemmings/gameview.cpp` (`ScrollTo` 0x406490, 18 instructions differ; `SetViewScale` 0x4073e0, 46 instructions differ).

* **Claude (Claude Code) Focus**:
  - `src/process/basher.cpp` (`Basher::Process` 0x10001040, 27 instructions differ).
  - `src/process/miner.cpp` (`Miner::Process` 0x10003c60, 25 instructions differ).
  - `src/process/map.cpp` (`Map::AddTrapType` 0x10004880, 6; `Map::AddObjectType` 0x10004760, 11): repaired on the user's request on 2026-11-07 (see HANDOFF "Claude session 35"); hand back by noting it in HANDOFF.
  - (released 2026-11-07, no edits) `src/lemmings/levelsel.cpp` OpenLevelItem: tried on the user's request, see HANDOFF Claude s35.
  - `src/render/render.c` `Renderer_Render` (0x10001020): matched 2026-11-07 on the user's request by Claude s37 (17 unused locals + 17 fewer `pad_newcanvas` externs, see HANDOFF "Claude session 37"); hand back by noting it in HANDOFF.
  - (2026-11-09, user request "work on all remaining functions in PROCESS", Claude s41) `src/process/builder.cpp` `Builder::Process` taken over from Antigravity's list for this session; plus basher.cpp, miner.cpp, map.cpp above. Hand back by noting it in HANDOFF.
  - (2026-11-09, user request "keep working on OpenLevelItem", Claude s42) `src/lemmings/levelsel.cpp` `OpenLevelItem` (0x40b3e0): matched 2026-11-09 (unsigned `t`, prev set in both branches, +8 `pad_openlevelitem` externs; see HANDOFF "Claude session 42 (continued)"); handed back.
  - `src/render/render.c` `CopyChanged` (0x10004af0): matched 2026-11-09 on the user's request by Claude s40 (`changed` used as the row pointer, sibling statement order, declaration order, +1 `pad_stretchchanged` extern; see HANDOFF "Claude session 40"); hand back by noting it in HANDOFF.

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

---

## 6. Running Wine Tools Without Repeated Security Stops (Antigravity)

Every matching/test tool runs the compiler or the game under Wine, and Wine cannot run inside Antigravity's terminal sandbox (its IPC is blocked). These commands therefore always need `BypassSandbox: true`, which always needs the user's approval. The user can click **Always allow**; Antigravity then remembers the approval **by command prefix** (binary + first argument), but only for simple commands. To keep those approvals working:

1. **Run Wine tools as single commands with one fixed prefix**, exactly in these forms (arguments after the prefix may vary):
   - `zsh tools/msvc/try.sh MODULE FILE ...`
   - `python3 tools/msvc/build.py --funcs MODULE`
   - `python3 tools/msvc/padplan.py ...`, `python3 tools/msvc/showvariant.py ...`, `python3 tools/msvc/mapcmp.py ...`
   - `zsh tools/wine/run.sh NAME UPDATES 'commands'`
   - `make`
   Do not run them sandboxed first (they always fail there); use `BypassSandbox: true` directly, for that single command only.
2. **Never wrap these tools in compound shell commands.** No heredocs (`cat << EOF > x.py ... EOF; python3 x.py`), no `&&`/`;`/`|` chains, no `$(...)`, no shell variables, no `env`/`timeout`/`xargs` wrappers: any of these makes Antigravity ask again for every new command. Write files (sweep scripts, variants) with the file-writing tool, which runs sandboxed and needs no approval, then run the script as its own command.
3. **Sweeps:** put sweep scripts in a stable place and call them in one fixed form, e.g. `python3 build/scratch/<agent>/sweep_<name>.py`. Each new script path is a new prefix (one approval); reuse one script with arguments instead of writing a new one per idea.
4. **Do not paste raw disassembly into replies.** A reply containing pages of raw x86 from `c2.exe` was blocked by Gemini's response safety filter (false positive). Write disassembly to files under `build/scratch/` and report only short excerpts and conclusions in terms of compiler passes (register allocation, scheduling, switch lowering).
