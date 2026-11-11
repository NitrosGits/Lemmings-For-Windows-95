# The native 64-bit build of Lemmings for Windows 95 (PORTING.md).
#
#   cmake -S . -B build/native -G Ninja
#   cmake --build build/native
#   build/native/lemmings --data orig/WINLEMM
#
# The 7 DLLs become static libraries linked into one executable. On macOS it
# is linked with the Win32 layer (port/src) and the platform's own backend,
# AppKit and Core Audio (port/macos). On 64-bit Windows the game runs on the
# real Win32, with one small support file (port/windows; PORTING.md, 10):
#
#   cmake -S . -B build/win64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=port/windows/llvm-mingw.cmake
#   cmake --build build/win64          # build/win64/Lemmings64.exe
#
# Linux gets its backend later (PORTING.md, 9). The 32-bit builds (Makefile,
# tools/msvc) are separate and unchanged.

cmake_minimum_required(VERSION 3.21)
project(Lemmings VERSION 1.0 LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

if(WIN32)
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "This is the 64-bit build: 32-bit Windows builds use the Makefile.")
  endif()
elseif(NOT APPLE)
  message(FATAL_ERROR "The native build is for macOS and 64-bit Windows so far (PORTING.md, sections 9 and 10): "
                      "Linux gets a GTK backend later.")
endif()

if(APPLE)
enable_language(OBJC)
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
add_library(winlayer OBJECT
  port/src/core.c port/src/file.c port/src/reg.c port/src/gdi.c port/src/font.c
  port/src/wnd.c port/src/msg.c port/src/res.c port/src/menu.c port/src/dlg.c
  port/src/controls.c port/src/peer.c port/src/wave.c port/src/mci.c port/src/harness.c
  ${RES_C})
set_target_properties(winlayer PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS ON)
target_include_directories(winlayer PUBLIC port/include PRIVATE port/src)
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(winlayer PRIVATE -Wall -Wno-unused-function -Wno-unused-parameter)
endif()

# The backend: windows, input, menus, controls, sound and the MIDI synthesizer
# are the platform's own (PORTING.md, section 3)
include(port/macos/macos.cmake)
endif()

# ---- The game ----
# What every game source gets. macOS: the layer's Win32 headers first, the
# original headers, and port_prefix.h forced in (the C runtime pieces).
# Windows: the system's headers, and win64_prefix.h forced in (the few calls
# that go to port/windows/win64.c).
if(APPLE)
  set(GAME_INCLUDES port/include src/include)
  set(GAME_FLAGS -include port_prefix.h -fno-strict-aliasing)
else()
  set(GAME_INCLUDES port/windows src/include)
  set(GAME_FLAGS -include win64_prefix.h -fno-strict-aliasing)
endif()
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  list(APPEND GAME_FLAGS -w)    # the decompiled code is kept as it is
endif()

function(game_module name)
  cmake_parse_arguments(M "" "" "SOURCES;DEFINES" ${ARGN})
  add_library(${name} STATIC ${M_SOURCES})
  set_target_properties(${name} PROPERTIES C_STANDARD 99 C_EXTENSIONS ON CXX_STANDARD 98 CXX_EXTENSIONS ON)
  target_include_directories(${name} PRIVATE ${GAME_INCLUDES})
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
if(APPLE)
  game_module(lemmings_exe SOURCES ${LEMMINGS_SRC} DEFINES PORT_MODULE_RAND=port_exe_seed)
else()
  # the program's entry is port/windows/win64.c's, which calls the game's
  game_module(lemmings_exe SOURCES ${LEMMINGS_SRC} DEFINES PORT_MODULE_RAND=port_exe_seed WinMain=game_WinMain)
endif()

# in dependency order, for linkers that read archives once
set(GAME_MODULES lemmings_exe process loadlvl render readdib filehand gameover vsmem)
if(APPLE)
  add_executable(lemmings $<TARGET_OBJECTS:winlayer>)
  target_link_libraries(lemmings PRIVATE ${GAME_MODULES} winlayer)
  lemmings_macos_target(lemmings)
else()
  include(port/windows/windows.cmake)
endif()
