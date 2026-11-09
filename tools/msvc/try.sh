#!/bin/zsh
# Compile SOURCE with Visual C++ 2.2 and compare with the original module:
#   tools/msvc/try.sh MODULE SOURCE.c [match.py options, e.g. -v -f NAME]
# The work is done by try.py (which also runs on Windows, without Wine).
exec python3 ${0:A:h}/try.py "$@"
