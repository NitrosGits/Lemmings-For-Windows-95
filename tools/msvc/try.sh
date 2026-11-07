#!/bin/zsh
# Compile SOURCE with Visual C++ 2.0 and compare with the original module:
#   tools/msvc/try.sh MODULE SOURCE.c [match.py options, e.g. -v -f NAME]
ROOT=${0:A:h:h:h}
mod=$1 src=${2:A}; shift 2
out=$ROOT/build/msvc/try/${src:h:t}/${src:t:r}.obj
mkdir -p ${out:h}
rm -f $out
cd ${out:h} && $ROOT/tools/msvc/cl.sh /nologo /c ${=MSVCFLAGS:-/Ox} /Fo${out} $src | tr -d "\r" | grep -v -i -E "^Z:[^(]*\.(c|cpp)$"
[[ -f $out ]] || { echo "compile failed"; exit 1; }
python3 $ROOT/tools/match.py $mod $src $out "$@"
