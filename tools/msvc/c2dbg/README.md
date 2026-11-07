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
