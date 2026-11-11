# CMake toolchain file for the 64-bit Windows build with llvm-mingw
# (https://github.com/mstorsjo/llvm-mingw), the compiler the 32-bit build
# uses too. The same on macOS, Linux and Windows: llvm-mingw's bin folder on
# the PATH, or LLVM_MINGW set to its folder.
#
#   cmake -S . -B build/win64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/windows/llvm-mingw.cmake
#   cmake -S . -B build/winarm64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/windows/llvm-mingw.cmake -DLEMMINGS_ARCH=aarch64
#
# LEMMINGS_ARCH: x86_64 (the default) or aarch64 (Windows on ARM).

if(NOT LEMMINGS_ARCH)
  set(LEMMINGS_ARCH x86_64)
endif()
if(NOT LEMMINGS_ARCH MATCHES "^(x86_64|aarch64)$")
  message(FATAL_ERROR "LEMMINGS_ARCH is x86_64 or aarch64, not '${LEMMINGS_ARCH}'")
endif()
# CMake configures test projects with this file again: they need the choice too
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES LEMMINGS_ARCH)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ${LEMMINGS_ARCH})

set(_triple ${LEMMINGS_ARCH}-w64-mingw32)
if(DEFINED ENV{LLVM_MINGW})
  set(_bin "$ENV{LLVM_MINGW}/bin/")
else()
  set(_bin "")
endif()
set(CMAKE_C_COMPILER ${_bin}${_triple}-clang)
set(CMAKE_CXX_COMPILER ${_bin}${_triple}-clang++)
set(CMAKE_RC_COMPILER ${_bin}${_triple}-windres)

# look for libraries and headers in the toolchain only, for programs on the host
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
