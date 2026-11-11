# 64-bit Windows: the game on the real Win32, without the layer (PORTING.md,
# section 10). One program, Lemmings64.exe (LemmingsArm64.exe for ARM64): the
# game's modules, its resources (lemmings.rc, compiled as they are), and
# port/windows: the entry point and the few calls a 64-bit program on today's
# Windows cannot make as the game did (win64.c), the test harness
# (harness.c) and operator new on malloc (new.cpp). It needs only Windows
# itself and its C runtime (ucrt, part of Windows 10 and later). Included by
# CMakeLists.txt.
#
# The name is not the original's: the program can sit in the game's folder
# beside the 32-bit LEMMINGS.EXE and its DLLs, which it does not use.

enable_language(RC)

add_executable(lemmings WIN32
  ${CMAKE_SOURCE_DIR}/port/windows/win64.c
  ${CMAKE_SOURCE_DIR}/port/windows/harness.c
  ${CMAKE_SOURCE_DIR}/port/windows/new.cpp
  ${CMAKE_SOURCE_DIR}/src/lemmings/lemmings.rc)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|ARM64|arm64)$")
  set_target_properties(lemmings PROPERTIES OUTPUT_NAME LemmingsArm64)
else()
  set_target_properties(lemmings PROPERTIES OUTPUT_NAME Lemmings64)
endif()

set_target_properties(lemmings PROPERTIES C_STANDARD 11 C_EXTENSIONS ON CXX_STANDARD 98 CXX_EXTENSIONS ON)
# src/include and src/lemmings: what lemmings.rc includes (resource.h, res/)
target_include_directories(lemmings PRIVATE
  ${CMAKE_SOURCE_DIR}/port/windows ${CMAKE_SOURCE_DIR}/src/include ${CMAKE_SOURCE_DIR}/src/lemmings)
target_compile_options(lemmings PRIVATE
  $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Wno-unused-parameter>
  $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>)
target_link_libraries(lemmings PRIVATE ${GAME_MODULES} winmm version advapi32 gdi32 user32 shell32)
# no C++ runtime library (new.cpp has the game's operator new); the compiler's
# own support code linked in, so the program needs no DLL of the toolchain's
target_link_options(lemmings PRIVATE -nostdlib++ -static)
