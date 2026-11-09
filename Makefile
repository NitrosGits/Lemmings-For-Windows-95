# Lemmings for Windows 95, rebuilt from the decompiled source with MinGW.
#
#   make            build every module that has sources into build/out
#   make game       build/game: the original game's data with the rebuilt binaries
#
# Needs a MinGW-w64 compiler for 32-bit Windows (GCC or llvm-mingw's Clang)
# and Python 3. On macOS/Linux it is the cross compiler i686-w64-mingw32-*;
# on Windows, run make in an MSYS2 shell with llvm-mingw's bin on the PATH, or
# from an MSYS2 MINGW32 shell (plain gcc, dlltool, ...; MSYS2 is phasing out
# its 32-bit environments). CROSS=prefix overrides the tool names.
# The byte-matching build with Visual C++ 2.2 is tools/msvc/build.py.

ifeq ($(OS),Windows_NT)
  # MSYS2's native toolchain has no i686-w64-mingw32- names for binutils
  ifeq ($(shell command -v i686-w64-mingw32-dlltool 2>/dev/null),)
    CROSS ?=
  endif
  # python3 may be the Microsoft Store stub, which exists but does not run
  ifndef PYTHON
    PYTHON := $(shell python3 -c "" >/dev/null 2>&1 && echo python3 || echo python)
  endif
endif
CROSS   ?= i686-w64-mingw32-
PYTHON  ?= python3
CC      := $(CROSS)gcc
CXX     := $(CROSS)g++
DLLTOOL := $(CROSS)dlltool
WINDRES := $(CROSS)windres

OUT := build/out
OBJ := build/obj
LIB := build/lib

CFLAGS  := -O2 -std=gnu99 -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable \
           -Wno-unused-but-set-global -Isrc/include \
           -fno-strict-aliasing -fno-stack-protector -fno-ident -march=i386 -mtune=i686
CXXFLAGS := $(filter-out -std=gnu99,$(CFLAGS)) -std=gnu++98 -fno-exceptions -fno-rtti \
           -fno-threadsafe-statics -fno-use-cxa-atexit -Wno-write-strings
CRTFLAGS := $(CFLAGS) -ffreestanding -fno-builtin \
            $(shell $(CC) -fno-tree-loop-distribute-patterns -E -x c /dev/null >/dev/null 2>&1 && echo -fno-tree-loop-distribute-patterns)
LDFLAGS := -nostdlib -Wl,--enable-stdcall-fixup -Wl,--subsystem,windows:4.0 \
           -Wl,--major-os-version,4 -Wl,--minor-os-version,0 -s
DLLFLAGS := -shared -Wl,--image-base,0x10000000 -Wl,-e,_DllMain@12

WINLIBS := -L$(LIB) -lkernel32 -luser32 -lgdi32 -ladvapi32 -lwinmm -lversion $(shell $(CC) -print-libgcc-file-name)

CRT_OBJ := $(OBJ)/crt/vscrt.o $(OBJ)/crt/dllmain.o $(OBJ)/crt/stdio.o

# The game's DLLs and the ones each imports
VSMEM_DEPS    :=
FILEHAND_DEPS :=
GAMEOVER_DEPS :=
READDIB_DEPS  := FILEHAND VSMEM
LOADLVL_DEPS  := FILEHAND READDIB VSMEM
RENDER_DEPS   := READDIB VSMEM
PROCESS_DEPS  := FILEHAND RENDER VSMEM
LEMMINGS_DEPS := VSMEM READDIB FILEHAND LOADLVL RENDER PROCESS

lower = $(shell echo $(1) | tr A-Z a-z)
srcs  = $(wildcard src/$(call lower,$(1))/*.c src/$(call lower,$(1))/*.cpp)
objs  = $(patsubst src/%,$(OBJ)/%.o,$(basename $(call srcs,$(1)))) \
        $(patsubst src/%.rc,$(OBJ)/%.res.o,$(wildcard src/$(call lower,$(1))/*.rc))

# the modules with sources so far
DLLS := $(foreach m,VSMEM FILEHAND GAMEOVER READDIB LOADLVL RENDER PROCESS,$(if $(call srcs,$(m)),$(m)))

all: $(foreach d,$(DLLS),$(OUT)/$(d).DLL) $(if $(call srcs,LEMMINGS),$(OUT)/LEMMINGS.EXE)

$(OBJ)/crt/%.o: src/crt/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CRTFLAGS) -c $< -o $@

$(OBJ)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/%.res.o: src/%.rc
	@mkdir -p $(dir $@)
	$(WINDRES) -Isrc/include -I$(dir $<) $< -o $@

# A DLL from its sources and export .def, and its import library for the
# modules that use it (mapping our symbols to the original export names)
define DLL_RULE
$(OUT)/$(1).DLL: $(call objs,$(1)) $(CRT_OBJ) src/$(call lower,$(1))/$(call lower,$(1)).def \
		$(foreach d,$($(1)_DEPS),$(LIB)/lib$(call lower,$(d)).a)
	@mkdir -p $(OUT)
	$(CC) $(DLLFLAGS) $(LDFLAGS) -o $$@ $$(filter %.o %.def,$$^) \
		$(foreach d,$($(1)_DEPS),-l$(call lower,$(d))) $(WINLIBS)

$(LIB)/lib$(call lower,$(1)).a: src/$(call lower,$(1))/$(call lower,$(1)).def tools/impdef.py
	@mkdir -p $(LIB)
	$(PYTHON) tools/impdef.py $$< $(LIB)/$(call lower,$(1)).imp.def
	$(DLLTOOL) -k -d $(LIB)/$(call lower,$(1)).imp.def -l $$@
endef
$(foreach d,$(DLLS),$(eval $(call DLL_RULE,$(d))))

# The game itself: its sources and resources, our startup code (exemain.c)
EXE_CRT := $(OBJ)/crt/vscrt.o $(OBJ)/crt/exemain.o $(OBJ)/crt/stdio.o
$(OUT)/LEMMINGS.EXE: $(call objs,LEMMINGS) $(EXE_CRT) $(foreach d,$(LEMMINGS_DEPS),$(LIB)/lib$(call lower,$(d)).a)
	@mkdir -p $(OUT)
	$(CXX) $(LDFLAGS) -Wl,-e,_WinMainCRTStartup -o $@ $(filter %.o,$^) \
		$(foreach d,$(LEMMINGS_DEPS),-l$(call lower,$(d))) $(WINLIBS)

ORIG ?= orig/WINLEMM
GAME ?= build/game

# the original's files (without MANUALS), then the rebuilt binaries over them
game: all
	@mkdir -p $(GAME)
	for f in $(ORIG)/*; do case "$$f" in */MANUALS) ;; *) cp -Rp "$$f" $(GAME)/ ;; esac; done
	cp $(OUT)/* $(GAME)/

clean:
	rm -rf $(OUT) $(OBJ) $(LIB)

.PHONY: all game clean
