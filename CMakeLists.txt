# The native 64-bit build of Lemmings for Windows 95 (PORTING.md).
#
#   cmake -S . -B build/native -G Ninja
#   cmake --build build/native
#   build/native/lemmings --data orig/WINLEMM
#
# The 7 DLLs become static libraries linked into one executable with the
# Win32 layer (port/src) and the platform's backend: AppKit and Core Audio on
# macOS (port/macos), SDL3 elsewhere for now. The 32-bit builds (Makefile,
# tools/msvc) are separate and unchanged.

cmake_minimum_required(VERSION 3.21)
project(Lemmings VERSION 1.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

if(APPLE)
  set(LEMMINGS_BACKEND cocoa CACHE STRING "The platform backend: cocoa or sdl")
else()
  set(LEMMINGS_BACKEND sdl CACHE STRING "The platform backend: cocoa or sdl")
endif()
if(LEMMINGS_BACKEND STREQUAL "cocoa")
  enable_language(OBJC)
else()
  find_package(SDL3 3.2 REQUIRED CONFIG)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# ---- The resources: lemmings.rc as C tables ----
file(GLOB RES_FILES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/lemmings/res/*)
set(RES_C ${CMAKE_BINARY_DIR}/gen/lemmings_res.c)
add_custom_command(
  OUTPUT ${RES_C}
  COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/port/rc2c.py
          ${CMAKE_SOURCE_DIR}/src/lemmings/lemmings.rc ${RES_C}
  DEPENDS ${CMAKE_SOURCE_DIR}/tools/port/rc2c.py ${CMAKE_SOURCE_DIR}/src/lemmings/lemmings.rc ${RES_FILES}
  COMMENT "Converting lemmings.rc")

# ---- The Win32 layer ----
add_library(winsdl OBJECT
  port/src/core.c port/src/file.c port/src/reg.c port/src/gdi.c port/src/font.c
  port/src/wnd.c port/src/msg.c port/src/res.c port/src/menu.c port/src/dlg.c
  port/src/controls.c port/src/peer.c port/src/wave.c port/src/mci.c port/src/harness.c
  ${RES_C})
set_target_properties(winsdl PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS ON)
target_include_directories(winsdl PUBLIC port/include PRIVATE port/src)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(winsdl PRIVATE -Wall -Wno-unused-function -Wno-unused-parameter)
endif()
if(NOT LEMMINGS_BACKEND STREQUAL "cocoa")
  target_sources(winsdl PRIVATE port/src/plat_sdl.c)
  target_link_libraries(winsdl PUBLIC SDL3::SDL3)
endif()

# The backend, sound and MIDI synthesizer: the platform's own (PORTING.md, section 3)
if(APPLE)
  include(port/macos/macos.cmake)
else()
  target_sources(winsdl PRIVATE port/src/midi_none.c)
endif()

# ---- The game ----
# What every game source gets: the layer's Win32 headers first, the original
# headers, and port_prefix.h forced in (the C runtime pieces).
set(GAME_FLAGS -include port_prefix.h -fno-strict-aliasing)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  list(APPEND GAME_FLAGS -w)    # the decompiled code is kept as it is
endif()

function(game_module name)
  cmake_parse_arguments(M "" "" "SOURCES;DEFINES" ${ARGN})
  add_library(${name} STATIC ${M_SOURCES})
  set_target_properties(${name} PROPERTIES C_STANDARD 99 C_EXTENSIONS ON CXX_STANDARD 98 CXX_EXTENSIONS ON)
  target_include_directories(${name} PRIVATE port/include src/include)
  target_compile_definitions(${name} PRIVATE PORT_SDL ${M_DEFINES})
  target_compile_options(${name} PRIVATE ${GAME_FLAGS}
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti -fno-threadsafe-statics>)
endfunction()

# Names defined in more than one DLL get a module prefix (PORTING.md, 2.3);
# RENDER and the EXE each have their own rand() seed, like their own runtimes.
game_module(vsmem SOURCES src/vsmem/vsmem.c
  DEFINES DllEntryPoint=vsmem_DllEntryPoint VS_AssertFailed=vsmem_VS_AssertFailed)
game_module(filehand SOURCES src/filehand/filehand.c
  DEFINES DllEntryPoint=filehand_DllEntryPoint)
game_module(gameover SOURCES src/gameover/gameover.c
  DEFINES DllEntryPoint=gameover_DllEntryPoint)
game_module(readdib SOURCES src/readdib/readdib.c
  DEFINES DllEntryPoint=readdib_DllEntryPoint VS_AssertFailed=readdib_VS_AssertFailed)
game_module(loadlvl SOURCES src/loadlvl/loadlvl.c
  DEFINES DllEntryPoint=loadlvl_DllEntryPoint VS_AssertFailed=loadlvl_VS_AssertFailed)
game_module(render SOURCES src/render/render.c
  DEFINES DllEntryPoint=render_DllEntryPoint VS_AssertFailed=render_VS_AssertFailed
          PORT_MODULE_RAND=port_render_seed)
file(GLOB PROCESS_SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/process/*.cpp)
game_module(process SOURCES ${PROCESS_SRC}
  DEFINES g_scale=process_g_scale g_list=process_g_list g_list2=process_g_list2
          ProcessLemmings=process_ProcessLemmings)
file(GLOB LEMMINGS_SRC CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/lemmings/*.cpp)
game_module(lemmings_exe SOURCES ${LEMMINGS_SRC} DEFINES PORT_MODULE_RAND=port_exe_seed)

add_executable(lemmings $<TARGET_OBJECTS:winsdl>)
# in dependency order, for linkers that read archives once
target_link_libraries(lemmings PRIVATE lemmings_exe process loadlvl render readdib filehand gameover vsmem winsdl)
if(APPLE)
  lemmings_macos_target(lemmings)
endif()
