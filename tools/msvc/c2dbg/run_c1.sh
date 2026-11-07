#!/bin/zsh
# usage: run_c1.sh SRC ILBASE   (runs c1xx or c1 depending on extension, leaves ILBASE.il* files in cwd)
R=/Users/daniel/Lemmings-For-Windows-95
export WINEPREFIX=$R/build/wine-vc WINEDEBUG=-all
VCW='Z:\Users\daniel\Lemmings-For-Windows-95\build\vc22\MSVC20'
INC="$VCW\\INCLUDE;Z:\\Users\\daniel\\Lemmings-For-Windows-95\\src\\include"
export INCLUDE="$INC" LIB="$VCW\\LIB"
src=$(winepath -w $1 2>/dev/null); il=$(winepath -w $PWD/$2 2>/dev/null)
case $1 in *.cpp) exe=c1xx.exe;; *) exe=c1.exe;; esac
wine "$VCW\\BIN\\$exe" -ef "$VCW\\BIN\\c1.err" -il "$il" -f "$src" -W 1 -Ze -Zp8 -ZB64 -D_INTEGRAL_MAX_BITS=64 -D_M_IX86=400 -G4 -pc '\:/' -D_MSC_VER=900 -D_WIN32 -nologo -Gs -Og -Oi -Ot -Oy -Ob1 -Fo"$(winepath -w $PWD/$2.obj)" -I "$VCW\\INCLUDE" -I 'Z:\Users\daniel\Lemmings-For-Windows-95\src\include' 2>&1 | grep -v -E "mvk|VK_|Vulkan|^\s*$|^\s+(model|type|vendor|device|pipeline|GPU|Metal|supports|Read)" ; true
