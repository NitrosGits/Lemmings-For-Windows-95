#!/bin/zsh
# Run Visual C++ 2.0's CL.EXE (or another of its tools, TOOL=LINK.EXE) under Wine:
#   tools/msvc/cl.sh /c /O2 file.c ...
# The compiler lives in build/vc20/MSVC20 (extracted from the user's own ISO;
# it is not part of this project). Unix paths in the arguments are converted.
ROOT=${0:A:h:h:h}
VC=${VCDIR:-$ROOT/build/vc22/MSVC20}
export WINEPREFIX=${VCPREFIX:-$ROOT/build/wine-vc} WINEDEBUG=-all
wp() { winepath -w "$1" 2>/dev/null }
export INCLUDE="$(wp $VC/INCLUDE);$(wp $ROOT/src/include)"
export LIB="$(wp $VC/LIB)"
args=()
for a in "$@"; do
  if [[ $a == /* && -e ${a:h} && $a != /[A-Za-z]* ]] || [[ $a == /*/* && -e ${a:h} ]]; then args+=("$(wp $a)")
  elif [[ $a == /F[oaep]/* || $a == /Fo/* ]]; then args+=("${a[1,3]}$(wp ${a[4,-1]})")
  elif [[ -e $a && $a != /* ]]; then args+=("$(wp ${a:A})")
  else args+=("$a"); fi
done
exec wine $VC/BIN/${TOOL:-CL.EXE} "${args[@]}" 2>/dev/null
