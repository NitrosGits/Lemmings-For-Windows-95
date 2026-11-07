#!/bin/zsh
# usage: run_c2dbg.sh SCRIPT ILBASE SRC OBJ  -> runs c2 under winedbg with the command file
R=/Users/daniel/Lemmings-For-Windows-95
export WINEPREFIX=$R/build/wine-vc
VCW='Z:\Users\daniel\Lemmings-For-Windows-95\build\vc22\MSVC20'
il=$(winepath -w $PWD/$2 2>/dev/null); src=$(winepath -w $3 2>/dev/null); obj=$(winepath -w $PWD/$4 2>/dev/null)
wine winedbg --file $1 "$VCW\\BIN\\c2.exe" -ef "$VCW\\BIN\\c23.err" -il "$il" -f "$src" -W 1 -G4 -dos -ML -Gs4096 -Fo"$obj" 2>&1 | grep -v -E "mvk|VK_|Vulkan|^\s*$|^\s+(model|type|vendor|device|pipeline|GPU|Metal|supports|Read)|fixme" ; true
