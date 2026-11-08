# Debugging VC++ 2.2's code generator (C2.EXE) under Wine

Needs the compiler in `build/vc22/MSVC20` and the Wine prefix `build/wine-vc` (same as `tools/msvc/cl.sh`).

1. Generate the IL files for a source with the same flags `cl` uses (`cl /Bd` shows them), in your scratch dir:
   `cd SCRATCH && /path/tools/msvc/c2dbg/run_c1.sh /abs/src.cpp NAME`  (leaves `NAMEex NAMEgl NAMEin NAMEsy`)
2. Write a winedbg command file, then run c2 under it:
   `tools/msvc/c2dbg/run_c2dbg.sh SCRIPT NAME /abs/src.cpp out.obj`
   (the output object is identical to cl's apart from the timestamp, so the debug run does not change the code).

Findings about winedbg here (Wine 11, arm64 Mac, x86_64 wine, 32-bit PE):
* `break *ADDR` at process start says "Invalid address" and `hbreak` never fires. First `break *0x401000`, `cont`,
  `delete 1` (0x401000 is a frequently called function), THEN set the real breakpoints and `cont`.
* `x/N x ADDR` prints N dwords; the `x/NwX` forms fail ("No symbols found for w"). `info reg` dumps registers.
* A script with N `cont`/`info reg` pairs works as a sampling trace; `quit` at the end.
* Breakpoints that never fire end in "Process ... has terminated" (that is how to prove a function is dead).

## One-function tools (Claude s40)

* `colour_trace_fn.py SRC FUNC_INDEX [N]`: colour trace of one function only. FUNC_INDEX = 1-based number of the call of
  the allocator driver sub_417260 (one per compiled function, in file order). Cut the file after the target function
  (stub later static functions) so the target is easy to find; CopyChanged is call 70 of `render.c` cut that way.
* `neigh_trace_fn.py SRC FUNC_INDEX [N]`: for every colouring decision, the node and its neighbours with their colours
  (breaks only where every dump is valid: a failing `x/` makes winedbg drop the commands after it).
* `rounds_fn.py SRC FUNC_INDEX [N]`: outer iterations, heuristic h, colour success / FAIL per node, spills.
* `simplify_model.py NB_TXT`: Python model of simplify + colour (rules in its docstring) rebuilt from a
  `neigh_trace_fn.py` output; reproduces the traced colours, so graph changes can be tried before hunting for source.
