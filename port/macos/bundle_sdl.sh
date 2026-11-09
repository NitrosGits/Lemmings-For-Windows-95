#!/bin/sh
# Lemmings.app carries its own SDL3: copies the library the build linked into
# Contents/Frameworks, points the executable at that copy, and signs both
# (ad hoc: arm64 macOS runs only signed code). Run by macos.cmake after a
# bundle build:  bundle_sdl.sh <app's executable> <libSDL3.0.dylib>
set -e
exe=$1
lib=$2
fw=$(dirname "$exe")/../Frameworks
name=$(basename "$lib")
mkdir -p "$fw"
cp -f "$lib" "$fw/$name"
chmod u+w "$fw/$name"
install_name_tool -id "@rpath/$name" "$fw/$name"
old=$(otool -L "$exe" | awk -v n="$name" '$1 ~ n {print $1; exit}')
[ -n "$old" ] && [ "$old" != "@rpath/$name" ] && install_name_tool -change "$old" "@rpath/$name" "$exe"
otool -l "$exe" | grep -q "@executable_path/../Frameworks" || install_name_tool -add_rpath "@executable_path/../Frameworks" "$exe"
codesign --force --sign - "$fw/$name"
codesign --force --sign - "$exe"
