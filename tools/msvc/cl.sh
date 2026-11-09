#!/bin/zsh
# Run Visual C++ 2.2's CL.EXE (or another of its tools, TOOL=LINK.EXE):
#   tools/msvc/cl.sh /c /O2 file.c ...
# The work is done by msvc.py: the tool runs under Wine on macOS/Linux and
# directly on Windows. The compiler lives in build/vc22/MSVC20 (copied from
# your own Visual C++ 2.2; it is not part of this project), or set VCDIR.
exec python3 ${0:A:h}/msvc.py "$@"
