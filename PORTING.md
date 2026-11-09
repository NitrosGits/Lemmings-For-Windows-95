# Native 64-bit port

This file plans and tracks the native port of Lemmings for Windows 95 to 64-bit
macOS, Linux and Windows, each on the system's own toolkit (macOS: AppKit and
Core Audio). It lives on the permanent branches `macOS-64`, `linux-64` and
`windows-64`, never on `main`. The port is written on `macOS-64`.

`main` stays the byte-matching decompilation. The port must not change what
Visual C++ 2.2 or the 32-bit MinGW build (`make`) compile, so both keep working
on these branches too.

**Status (2026-11-10):** the game builds as a native arm64 macOS program that
needs only the system's frameworks, and plays: title screen, registration,
player and level dialogs, options, About, level 1, the end screen, music and
sound effects. Its windows are AppKit windows, its menus are in the macOS menu
bar, and its dialogs' buttons, check boxes, radio buttons, group boxes, edit
fields and lists (the level tree included) are AppKit controls. Checked against
the original game's reference logs (`tools/port/checks.sh`, section 8): level
1, the digger replay (by window messages, by system mouse events and by a
system key) and a nuke to the end of the level all MATCH, and three dialogs
driven by system clicks and keys end in the same control states as the drawn
controls did. Visual C++ 2.2 still matches all 696 functions with the port's
source changes, and the MinGW build is unchanged. Floating (undocked) tool
windows are untried, and so are `linux-64` and `windows-64`.

The port first ran on SDL3 (2026-11-09). On the user's request it moved to
each system's own toolkit (2026-11-10): the layer reaches the system only
through `port/src/plat.h`, and SDL is gone (section 9).

---

## 1. Approach: a Win32 subset on the system's toolkit

The game uses a lot of Windows: about 180 different Win32 functions, a
main window with a menu bar and a status bar, child windows, tool windows that
dock and float, 6 dialogs from resource templates (with list boxes, owner-drawn
items, edit fields, check boxes and radio buttons), GDI drawing into 8-bit DIB
sections, palettes, timers, the registry, `waveOut` sound and MCI MIDI music.

There are two ways to port that:

- **A. Rewrite the front end** for a new toolkit and keep only the game logic.
  That changes most of the 23 files of `LEMMINGS.EXE`, which then no longer
  match `main`'s source, and every dialog and window gets redesigned.
- **B. Implement the part of Win32 the game uses** (a small, game-specific
  "Wine") on top of the system. The game's sources compile almost unchanged,
  the user interface behaves like the original, and changes merged from `main`
  apply cleanly.

**The port takes B.** The layer is new code under `port/`; the decompiled
sources get only small `#ifdef PORT_SDL` changes where 64-bit pointers don't fit
(section 5; the macro kept its first name). The layer (`port/src`) is plain C11
without system calls of its own: it reaches the system through one interface,
`port/src/plat.h` (windows, events, the menu bar, native controls, cursors,
message boxes, sound streams, timers, folders), which each system implements in
its own folder - `port/macos/` on AppKit, Core Animation and Core Audio. Where a
backend has no system menu bar or no native controls, the layer draws its own
(Windows 95 style) instead.

---

## 2. Inventory

Taken from the 32-bit MinGW build's import tables and object files (`make`,
then `llvm-nm -u` per object), and from reading the sources. Counts are distinct
functions.

### 2.1 Modules

| Module | Sources | Win32 use |
|---|---|---|
| `LEMMINGS.EXE` (`src/lemmings`, 23 C++ files + `lemmings.rc`) | windows, dialogs, menus, GDI, palette, timers, registry, MCI MIDI, files | almost all of it |
| `RENDER.DLL` (`render.c`) | 8-bit DIB sections, `BitBlt`/`StretchBlt`/`StretchDIBits`, `timeGetTime` | GDI |
| `PROCESS.DLL` (27 C++ files) | `waveOut*`, `mmio*`, `GetAsyncKeyState`, `fopen` (scripts) | sound |
| `LOADLVL.DLL` (`loadlvl.c`) | `_lwrite` (unused function), message boxes | almost none |
| `READDIB.DLL` (`readdib.c`) | `CreateDIBitmap`, `CreatePalette`, `CreateDC("DISPLAY")` | GDI |
| `FILEHAND.DLL` (`filehand.c`) | `CreateFile`/`ReadFile`/`GetFileSize` | files |
| `VSMEM.DLL` (`vsmem.c`) | `GlobalAlloc`/`GlobalLock` | memory |
| `GAMEOVER.DLL` (`gameover.c`) | `LoadString(NULL, …)` (reads the EXE's string table) | strings |

`src/crt` (the small C runtime of the 32-bit build) is not used by the native
build: the host's C library replaces it.

### 2.2 Win32 API by area (176 functions used by the game's own code)

| Area | Functions | Notes |
|---|---|---|
| Windows (USER) | `RegisterClass`, `CreateWindowEx`, `DestroyWindow`, `DefWindowProc`, `ShowWindow`, `UpdateWindow`, `SetWindowPos`, `GetWindowRect`, `GetClientRect`, `AdjustWindowRect`, `ClientToScreen`, `ScreenToClient`, `ChildWindowFromPoint`, `WindowFromPoint`, `GetParent`, `SetParent`, `GetWindow`, `GetDesktopWindow`, `FindWindow`, `Get/SetWindowLong`, `Get/SetWindowWord`, `Get/SetWindowText`, `EnableWindow`, `IsWindowEnabled`, `SetFocus`, `InvalidateRect`, `BeginPaint`/`EndPaint`, `GetDC`/`GetWindowDC`/`ReleaseDC`, `ScrollWindow`, `SetScrollRange`/`SetScrollPos`, `SetCapture`/`ReleaseCapture`, `GetCursorPos`, `SetCursor`/`GetCursor`, `LoadCursor`/`DestroyCursor`, `LoadIcon`, `GetSystemMetrics`, `GetSysColor` | 15 window/dialog creations; scroll bars on the game view; tool windows switch between `WS_POPUP` and `WS_CHILD` with `SetParent` |
| Messages | `PeekMessage`, `WaitMessage`, `TranslateMessage`, `DispatchMessage`, `SendMessage`, `PostMessage`, `PostQuitMessage`, `SetTimer`/`KillTimer`, `GetAsyncKeyState`, `LoadAccelerators`/`TranslateAccelerator` | own loop in `WinMain` and `WaitTime`; custom messages 0x400–0x800; `DockWnd::Drag` polls `GetAsyncKeyState` in a loop without pumping messages |
| Dialogs and controls | `DialogBoxParam`, `EndDialog`, `GetDlgItem`, `Get/SetDlgItemText`, `CheckDlgButton`, `IsDlgButtonChecked`, `DrawFocusRect` | 6 templates (level select ×2, options, about, select player, registration); controls: Button (push, default, check, radio, group box), Static (text, icon, frames), Edit, ListBox (plain and owner-drawn), plus the game's own `IconTextClass`/`IconImageClass`; messages `LB_*` (12), `LBN_*`, `EM_SETSEL`, `EN_UPDATE`, `WM_DRAWITEM`/`WM_MEASUREITEM`, `WM_CTLCOLORDLG`, `WM_SETFONT` |
| Menus | `LoadMenu`, `SetMenu`, `GetMenu`, `DestroyMenu`, `EnableMenuItem` | 3 menus (game, front end, end screen); help texts in the status bar via `WM_MENUSELECT`; 28 accelerators |
| GDI | `CreateCompatibleDC`, `CreateDC`, `DeleteDC`, `SelectObject`, `DeleteObject`, `GetObject`, `GetStockObject`, `CreateDIBSection`, `CreateDIBitmap`, `BitBlt`, `StretchBlt`, `StretchDIBits`, `PatBlt`, `FillRect`, `Rectangle`, `CreateSolidBrush`, `CreateHatchBrush`, `CreateBrushIndirect`, `GdiFlush`, `GetDeviceCaps`, `UpdateColors` | raster ops: only `SRCCOPY`, `PATCOPY`, `DSTINVERT`; all DIBs `DIB_RGB_COLORS` |
| Palettes | `CreatePalette`, `SelectPalette`, `RealizePalette`, `UnrealizeObject`, `GetPaletteEntries`, `GetSystemPaletteEntries`, `GetNearestPaletteIndex` | about 50 calls, written for 256-colour displays; `WM_QUERYNEWPALETTE`/`WM_PALETTECHANGED` |
| Text | `CreateFont`, `TextOut`, `ExtTextOut`, `GetTextExtentPoint`, `GetTextMetrics`, `SetTextColor`, `SetBkColor`, `SetBkMode`, `wsprintf`, `MulDiv`, `lstrcpy/lstrcat/lstrlen` | 4 fonts (status bar, icon-panel numbers, splash, about box) |
| Resources | `LoadString`, `FindResource`/`LoadResource`/`LockResource`/`FreeResource`, `GetModuleFileName`, `GetFileVersionInfo*`/`VerQueryValue` | `lemmings.rc`: 6 dialogs, 3 menus, accelerators, ~90 strings, 8 cursors, 1 icon, 1 bitmap (the level-tree icons), version info |
| Registry | `RegCreateKeyEx`, `RegOpenKeyEx`, `RegCloseKey`, `RegQueryValueEx`, `RegSetValueEx`, `RegEnumKeyEx`, `RegDeleteKey`, `RegFlushKey` | 6 call sites in `registry.cpp`; `HKLM\Software\Visual Sciences\Windows Lemmings` with a key per player; also reads `ProgramFilesDir` and writes an uninstall key |
| Files | `CreateFile`, `ReadFile`, `GetFileSize`, `CloseHandle`, `_lopen`/`_lread`/`_lwrite`/`_llseek`/`_lclose`, `CreateDirectory`, `DeleteFile`, `GetCurrentDirectory`, `GetWindowsDirectory`; C library `fopen`/`fread`/`fwrite`/`fgetc`/`_filelength`/`_fileno`/`_unlink` | all paths are Windows paths (`gfx\icons.bmp`); the data files on disk are upper case (`GFX/ICONS.BMP`) |
| Sound (WINMM) | `waveOutOpen`/`Close`/`PrepareHeader`/`UnprepareHeader`/`Write`/`Reset`/`GetPosition`/`GetNumDevs`/`GetDevCaps`, `mmioOpen`/`Descend`/`Ascend`/`Read`/`Close` | 23 WAVs, one effect at a time, the device opened per effect |
| Music (MCI) | `mciSendCommand` (`MCI_SYSINFO`, `OPEN`, `SEEK`, `SET`, `STATUS`, `PLAY`+`MCI_NOTIFY`, `STOP`, `CLOSE`), `mciGetErrorString`, `midiOutGetDevCaps` | 15 `.MID` files played through the MCI sequencer, restarted on `MM_MCINOTIFY` |
| System | `GetVersion`, `GlobalMemoryStatus`, `GlobalAlloc/Lock/Unlock/Free`, `Sleep`, `timeGetTime`, `MessageBox`, `ExitProcess`, `GetLastError`, `OutputDebugString`, `WinHelp` | |

The per-file list is in appendix A.

### 2.3 32-bit and compiler assumptions

| What | Where | Port |
|---|---|---|
| Pointers stored in window longs (`SetWindowLong(…, (LONG)ptr)`) | `window.cpp` (the C++ object of every window), `statusbar.cpp` (the status bar's data) | `Get/SetWindowLongPtr` under `#ifdef PORT_SDL` |
| An `HWND` kept in an `int` (`g_global28`, the status bar) | `frame.cpp`, `winmain.cpp`, `winlemm.h` | an `INT_PTR` under `#ifdef PORT_SDL` |
| Pointers passed as `DWORD` (`mciSendCommand(…, (DWORD)&parms)`, `dwCallback = (DWORD)hwnd`, `WinHelp(…, (DWORD)"CONTENTS")`) | `midi.cpp` (10), `mainwnd.cpp` (2) | `DWORD_PTR` under `#ifdef PORT_SDL` |
| `long` is 64-bit on LP64 (macOS, Linux) | only local sizes and counters (`user.cpp`, `settings.cpp`, `loadlvl.c`, `vsmem.c`, `FILE_Read`) | harmless; `DWORD`, `LONG`, `UINT` stay 32-bit in the layer's headers |
| Structures read from files | `BITMAPFILEHEADER` (packed, 14 bytes), `BITMAPINFOHEADER`, `BITMAPCOREHEADER`, `RGBQUAD`, `WAVEFORMATEX` (18 bytes), the level database (`DBENTRY`, `#pragma pack(1)`, `int` header), sprite files (`DWORD` offsets, read unaligned) | same layout with fixed-width types and packing in the layer's headers; no file structure holds a pointer |
| Structures with pointers that only live in memory | `DLENTRY` (24 → 32 bytes), `LEVEL`, `LEMMING` (pointers from 0x5c), `WAVEHDR` | fine: always used through `sizeof` and member names (to check while porting) |
| Inline assembly | `render.c` (13 blitter loops), `gameview.cpp` (scroll clamp), `dockwnd.cpp` and `sound.cpp` (padding bytes for the matching build only) | already have C versions under `#ifndef _MSC_VER`, used by the MinGW build: nothing to do |
| Calling conventions (`WINAPI`, `CALLBACK`, `PASCAL`, `__declspec`) | everywhere, `__declspec` only under `_MSC_VER` | `WINAPI` etc. are empty on 64-bit |
| Each DLL has its own copy of the C runtime: own `rand()` seed (MSVC's generator) | `render.c` (`Random`), `midi.cpp` (`RandomTune`) | a per-module MSVC-compatible `rand`/`srand` (the host's `rand` is a different generator) |
| Names defined in more than one DLL | `DllEntryPoint` (6 DLLs), `VS_AssertFailed` (4), `g_scale`, `g_list`, `g_list2` (EXE and PROCESS, different variables) | renamed per module by the build (`-Dg_scale=process_g_scale` …), no source change |
| The original deletes the dock windows in `EndGame` but keeps them in `g_dockList` until the next `StartGame` | `mainwnd.cpp` | the portable build's `DELETE_DOCKWND` (`#ifndef _MSC_VER`) already keeps their memory until then; the port uses it as is, and the layer must cope with `Move`/`SetWindowPos` on destroyed windows (it ignores unknown handles) |
| Pointer error codes (`(WAVEHDR *)-1` … `-7` from `LoadWave`, later passed back to `waveOutUnprepareHeader`) | `wave.cpp` | the layer checks headers against the ones it knows before touching them |

---

## 3. Layer design

```
port/
  include/            Win32 headers for the game: windows.h, mmsystem.h, io.h,
                      plus port_prefix.h (forced first into every game file)
  src/                the layer (C11), one file per area below; plat.h is what
                      it needs from the system, peer.c ties dialog controls to
                      native views
  macos/              the macOS backend: plat_cocoa.m (application, windows,
                      events, cursors, alerts), menu_cocoa.m (the menu bar),
                      controls_cocoa.m (dialog controls), audio_coreaudio.c
                      (sound effects), midi_coreaudio.c (music), keycodes.h,
                      macos.cmake, Info.plist.in
tools/port/
  rc2c.py             lemmings.rc -> C tables (dialogs, menus, strings, accelerators,
                      version info, embedded .bmp/.cur/.ico)
  run.sh              scripted test run of the native game (like tools/wine/run.sh)
  checks.sh           all the checks of section 8, one line each
  expect/             the dialog checks' expected control states
CMakeLists.txt        the native build
```

**Types (`port/include/windows.h`).** Fixed-width types as on Win64:
`BYTE`/`WORD`/`DWORD`/`UINT`/`LONG`/`BOOL` 8/16/32/32/32/32 bits;
`WPARAM`, `LPARAM`, `LRESULT`, `*_PTR` pointer-sized. Handles (`HWND`, `HDC`,
`HBITMAP`, …) are pointers to the layer's objects. Structures the game reads from
files are packed exactly as in the Windows SDK. Only the functions, messages and
constants the game uses are declared; anything missing is a compile error, so the
inventory stays honest. `DLGPROC` returns `BOOL` as in the 1995 SDK, matching the
game's dialog procedures.

**Windows and messages.** A window is an object with class, procedure, style,
parent, children, rectangle, extra bytes and text. Every top-level window (main
frame, dialogs, floating tool windows, splash) is a system window (macOS: an
`NSWindow`); child windows are areas drawn into their top-level window. The
message queue, `SendMessage` (direct call), `PostMessage`, timers (`WM_TIMER`),
`WM_PAINT` from invalid areas, mouse capture, focus, double clicks
(`CS_DBLCLKS`) and the usual creation and destruction message sequences follow
Win32. The layer's message loop pumps the system's events itself (on macOS
`nextEventMatchingMask:`/`sendEvent:`, never `[NSApp run]`); the backend turns
them into `WM_MOUSEMOVE`, `WM_xBUTTONDOWN/UP`, `WM_KEYDOWN/UP`, `WM_CHAR`
(through `TranslateMessage`, from the characters the key typed), `WM_SIZE`,
`WM_MOVE`, `WM_ACTIVATEAPP` and `WM_CLOSE`; Quit (Cmd+Q) becomes `WM_CLOSE` to
the main window. `GetAsyncKeyState` and `GetCursorPos` read the system's state
directly, so `DockWnd::Drag`'s polling loop works.

**Drawing.** Each top-level window has a 32-bit framebuffer. A DC draws into a
window's framebuffer (offset and clipped to that window) or into a bitmap. DIB
sections keep their own bit depth (the game's are 8-bit) and colour table; other
bitmaps are 32-bit. `BitBlt`/`StretchBlt`/`StretchDIBits` convert 8-bit pixels
through the source's colour table. This is how true-colour Windows draws the
game today, so palettes need no emulation: palette calls keep their entries
(`GetPaletteEntries`, `GetNearestPaletteIndex`) and realising does nothing.
`GetDeviceCaps` reports a 256-colour display, so the game shows no colour-depth
warning. Changed framebuffers are shown when the message queue is empty: on
macOS as the contents of the window's Core Animation layer, scaled with
nearest-neighbour filtering on Retina screens.

**Text.** A built-in proportional bitmap font, drawn for this project (no font
files), sized like MS Sans Serif 8 (about 13 pixels high) and scaled for larger
requests; bold by drawing twice. Dialog units are computed from it as Windows
does.

**Dialogs, controls, menus.** `tools/port/rc2c.py` turns `lemmings.rc` into C
tables at build time. The dialog manager creates the controls from the
templates, runs a modal loop, handles Tab/Enter/Escape and default buttons, and
sends `WM_INITDIALOG`, `WM_COMMAND`, `WM_DRAWITEM`, `WM_MEASUREITEM`,
`WM_CTLCOLORDLG`. Built-in classes: Button (push, default, check box, radio, group
box, auto variants), Static (text, icon, frames), Edit (one line), ListBox (with
owner-drawn items, `LBS_NOTIFY`, a scroll bar) and scroll bars on any window
with `WS_HSCROLL`/`WS_VSCROLL`.

*Native controls (`port/src/peer.c`, `port/macos/controls_cocoa.m`).* A
dialog's buttons, check boxes, radio buttons, group boxes, edit fields and list
boxes are shown by the system's own controls (`NSButton`, `NSBox`,
`NSTextField`, `NSTableView`), as subviews over the framebuffer. The layer's
control stays the truth: its state, messages and notifications are as before,
it just stops drawing itself. Before each present the layer shows its state in
the views (text, check marks, enabled, visible, frame, rows, selection, scroll
position); what the user does in a view comes back as the messages the drawn
control would have handled (`BM_CLICK`, `LB_SETCURSEL` + `LBN_SELCHANGE`,
`LBN_DBLCLK`, `WM_SETTEXT` + `EN_UPDATE`/`EN_CHANGE`, and Return, Escape and Tab
through the dialog manager). The owner-drawn level tree is an `NSTableView`
whose rows the game draws (`WM_DRAWITEM` into a DIB section, one image per
row). Static text and the game's own classes (`IconTextClass`,
`IconImageClass`) stay drawn: they rely on `WM_CTLCOLOR*` and the game's
backgrounds. Each radio button gets its own superview, because AppKit groups
all the radio buttons of a view while the dialog's groups are its `WS_GROUP`
runs. `LEMMINGS_NATIVE_CONTROLS` (letters `b`uttons, `g`roup boxes, `e`dit,
`l`ists, `t`ree) limits which kinds get views, for comparisons.

*Menus.* With a system menu bar (macOS), a window's menu goes there: the
application menu (About, Hide, Quit) and then the game's drop-down menus,
filled as they open (`WM_INITMENU` and `WM_INITMENUPOPUP` first, so the game
enables and checks its items), with `WM_MENUSELECT` for the highlighted item
(the game's status-bar help) and `WM_COMMAND` for the chosen one. Their items
are disabled while a modal dialog has the frame disabled. Without one, menus
are drawn in Windows 95 style as a bar at the top of the window, with
drop-down lists, mouse and keyboard (Alt, mnemonics). Accelerator tables are
honoured either way; the native menus have no key equivalents of their own.
Message boxes are the system's (`NSAlert`).

**Settings.** The registry is a tree of keys and values in memory, loaded from
and saved to `registry.txt` in the user's preferences folder
(`~/Library/Application Support/Visual Sciences/Lemmings` on macOS). Values are
`REG_DWORD` and `REG_SZ`.

**Files.** Windows paths are translated: `\` becomes `/`, each name is matched
case-insensitively against the disk (the data files are upper case), and drive
paths map like Wine's: the game's folder (`C:\WINLEMM`, what
`GetCurrentDirectory` returns) is the original data folder, read-only in
practice; the rest of `C:\` is `drive_c/` in the preferences folder. Player
databases, which the game writes under `ProgramFilesDir\WinLemm` (the layer's
registry sets `ProgramFilesDir` to `C:\Program Files`), so land in the
preferences folder and the original data is never written. The data folder is
found from `--data DIR`, `LEMMINGS_DATA`, the app bundle's resources, or
`orig/WINLEMM` next to the build. `fopen` and the CRT extras (`_filelength`,
`_fileno`, `_unlink`, `itoa`) go through the same translation (`port_prefix.h`).

**Sound.** Each `waveOutOpen` handle is a sound stream of the backend in the
WAV's format; `waveOutWrite` queues the data, `waveOutGetPosition` counts the
bytes played, `waveOutReset` clears the stream. On macOS one default-output
AudioUnit mixes the streams, resampling each linearly from its own rate
(`port/macos/audio_coreaudio.c`); a stream's queued count reaches exactly 0
when its sound has ended, which `WOM_DONE` relies on. `mmio*` is a small RIFF
reader.

**Music.** The layer emulates the MCI sequencer commands the game sends
(`port/src/mci.c`) and posts `MM_MCINOTIFY` when a tune ends. The tunes are
played by the operating system's own MIDI synthesizer (the user's choice,
section 8), through a small backend interface (`port/src/midi_backend.h`: open a
file, play from a position, stop, position, length):

- macOS: Core Audio's `MusicPlayer` with Apple's built-in General MIDI
  synthesizer (`port/macos/midi_coreaudio.c`, AudioToolbox);
- Windows (`windows-64`, later): the system's MIDI through WINMM;
- Linux (`linux-64`, later): no built-in synthesizer, so no music unless a
  backend is added there; `port/src/midi_none.c` reports no MIDI device, which
  the game handles (it never opens the sequencer).

**Other.** `WinHelp` (no Windows Help on modern systems): opens the English PDF
manual from the original data (`MANUALS/ENGLISH.PDF`) with the system's viewer.
`GetFileVersionInfo` serves the `VERSIONINFO` block from the resource tables
(the About box). `timeGetTime` is the system's monotonic clock in
milliseconds, `GlobalAlloc` is `malloc`.

**Test harness.** With `LEMMINGS_HARNESS=1` the layer runs the same commands as
`tools/harness/proxy.c` (`wait`, `waitupd`, `click`, `rclick`, `dbl`, `key`,
`move`, `menu`, `cmd`, `ctl`, `edit`, `shot`) from `CMD.TXT`, plus its own
(section 8: `sclick`, `sdbl`, `sclickw`, `skey`, `smenu`, `ctls`, `wshot`), and writes
`LEMLOG.TXT` in the same format (each lemming's first 0x5c bytes as in the 32-bit
layout, its pointers cut to 32 bits). `tools/cmplog.py` then compares a native run
with `build/runs/hd-base-l1.txt` and `hd-base-dig.txt`, the reference logs of the
original game under Wine.

---

## 4. Build

```
cmake -S . -B build/native -G Ninja
cmake --build build/native
build/native/lemmings --data orig/WINLEMM      # -DLEMMINGS_MACOS_BUNDLE=ON: Lemmings.app
```

- The 7 DLLs become static libraries linked into one executable with the layer;
  the game files are compiled with `-DPORT_SDL -include port_prefix.h
  -Iport/include -Isrc/include`, C99 / C++98-compatible as now.
- Per-module definitions keep the DLLs apart: the renames of section 2.3 and
  each module's own `rand`.
- Needs CMake ≥ 3.21, Apple's command line tools (clang with Objective-C and
  the macOS SDK), Python 3 (for `rc2c.py`) and Ninja or make. No other library:
  the executable links only the system's frameworks (AppKit, QuartzCore,
  CoreGraphics, AudioToolbox, CoreAudio). CMake stops with a message on other
  systems until they have a backend.
- The MinGW `Makefile` and `tools/msvc/` are not touched. Checks after each step:
  `make` still builds, and `python3 tools/msvc/build.py --funcs` still reports
  696/696 (the `#ifdef PORT_SDL` changes are invisible to Visual C++).
- `.github/workflows/macos-build.yml` on `macOS-64`: builds the arm64 executable
  and `Lemmings.app` (checking it links only system libraries and is signed),
  starts it without game data, and uploads the app. It cannot play the game
  (no game data in CI).

---

## 5. Changes to the game's sources

Rules: only where the 64-bit build cannot compile or work otherwise; each change
inside `#ifdef PORT_SDL … #else original #endif`, so Visual C++ 2.2 and MinGW
see exactly what they see on `main`; each listed here.

Expected (section 2.3): `window.cpp`, `statusbar.cpp`, `frame.cpp`,
`winmain.cpp` + `winlemm.h` (`g_global28`), `midi.cpp`, `mainwnd.cpp`. More may
turn up when the sources first compile for arm64.

| File | Change |
|---|---|
| `src/include/port64.h` (new) | macros `PORT_PTR_CAST`, `PORT_GetWindowLong`/`PORT_SetWindowLong`, `PORT_LONG`, `PORT_DWORD`: the pointer-sized Win64 names under `PORT_SDL`, the original's names and types otherwise |
| `src/include/winlemm.h` | includes `port64.h` (the EXE's files) |
| `src/include/lemming.h` | `Message(..., (int)this)` inside `#ifdef PORT_SDL ... #else ... #endif`, `(int)(INT_PTR)this` natively (the handlers ignore that argument); no `port64.h` here, see below |
| `src/lemmings/window.cpp`, `statusbar.cpp` | the window object / status bar pointer in window longs through `PORT_Get/SetWindowLong` and `PORT_LONG` |
| `src/lemmings/frame.cpp` | `g_global28 = PORT_PTR_CAST(int, CreateStatBar(...))` (handles are 32-bit values in the layer, as on Win64) |
| `src/lemmings/levelsel.cpp` | `return PORT_PTR_CAST(BOOL, g_gameDlgBrush);` (WM_CTLCOLORDLG) |
| `src/lemmings/midi.cpp` | MCI parameter blocks and `dwCallback` as `PORT_DWORD` |
| `src/lemmings/mainwnd.cpp`, `frontend.cpp`, `endscreen.cpp` | WinHelp's string argument as `PORT_DWORD` |
| `src/process/lemmings.cpp` | PROCESS's global `operator new`/`delete` (onto VSMEM) left out: in one executable they would replace the allocator of the whole process, system libraries included. PROCESS never mixes `new`/`delete` with `VSMEM_Allocate`/`VSMEM_Free`. |
| `src/lemmings/settings.cpp` | no `InstallUninstaller()` (it copies the CD's Windows uninstaller and registers it) |

Visual C++ 2.2 notices more than the tokens it compiles, so two lessons:

- `PORT_PTR_CAST(type, p)` gives it exactly the original's tokens `(type)p`.
  A first version wrote `(int)(PORT_INT)p`, i.e. `(int)(int)p` for Visual
  C++: the same instructions, but C2's internal numbering shifted and 17
  PROCESS functions and one LEMMINGS function stopped matching (the project's
  known sensitivity, docs/DECOMP.md).
- Even macro definitions count: with `port64.h` included in `lemming.h`, every
  function still matched but PROCESS.DLL's vtables came out in another order
  in `.rdata` (19 bytes of the whole file). A plain `#ifdef PORT_SDL` at the
  one use site leaves the DLL exactly as before; the EXE, with `port64.h` in
  `winlemm.h`, is unaffected.

Checked after every change of `src/`: `python3 tools/msvc/build.py --funcs`
for all 8 modules (696/696, the same whole-file differences from the original
as `main`'s sources give), PROCESS.DLL and LEMMINGS.EXE section by section
against a build of `main` (identical apart from timestamps), and a clean
`make` (the same 8 binaries as before).

---

## 6. Order of work

1. **Headers and build.** (done) `port/include`, `CMakeLists.txt`, `rc2c.py`; all game
   sources compile for arm64; the `#ifdef PORT_SDL` changes of section 5.
2. **Windows, drawing, input, loop.** (done) Window manager, message loop, timers, GDI
   with DIB sections, the font; the splash screen and the title screen appear.
3. **Files and settings.** (done) Path translation, registry file; the registration
   and player dialogs need the dialog manager, so that comes next.
4. **Dialogs and menus.** (done) Dialog manager, the standard controls, the menu bar;
   registration, player selection, options, level selection.
5. **The game.** (done; floating tool windows not tried yet) Level 1 plays: game view with scroll bars, docked icon panel
   and map, status bar; the end screen; floating tool windows.
6. **Sound.** (done) WAV effects, then music (MCI emulation and the Core Audio backend).
7. **Checks.** (done) The harness; level 1 and the digger run compared with the
   reference logs; a level ends cleanly; `make` and the 696/696 matching build
   still work.
8. **macOS packaging and CI.** (done) App bundle, the CI workflow, README section.
9. **The system's own toolkit.** (done, 2026-11-10) The layer reaches the
   system only through `plat.h` (first still on SDL, every check passing);
   the AppKit and Core Audio backend; the game's menus in the macOS menu bar;
   native dialog controls (buttons, then edit fields and lists, then the level
   tree), with dialog checks made against the drawn controls first; SDL
   deleted.

---

## 7. Risks

- **Size of the layer.** The Win32 part the game needs is still a lot (window
  manager, dialogs, five control types, menus, GDI). It is bounded by the
  inventory; missing pieces fail at compile time, not at run time.
- **Behaviour differences.** Some game code depends on exact Win32 message order
  (e.g. `WM_SIZE` before `WM_CREATE` returns, `WM_ERASEBKGND` in `BeginPaint`,
  `SetParent` on the dock windows). The harness and the reference logs catch
  differences that change the game; look and feel are checked by eye.
- **Fonts and layout.** Without MS Sans Serif, dialog text may not fit its
  controls exactly; the built-in font is sized to its metrics.
- **Floating tool windows.** The dock drag draws an outline on the desktop,
  which a modern system won't let a program draw on: the outline is left out
  (the docking itself works). Popup windows and modal dialogs depend on the
  window manager (macOS, X11, Wayland) more than the rest.
- **Timing.** The game steps every 80 ms from its idle loop. Presenting frames
  and the system's event handling must not delay it; the harness counts
  updates, so logs stay comparable. While AppKit tracks a menu or a window
  drag the game waits, as a Win32 program in a modal menu or move loop does; a
  timer in AppKit's tracking mode keeps presenting meanwhile.
- **Native controls and the layer.** A control's native view and its layer
  state can disagree only between a user's action and the next sync; the
  layer's state wins. Keys typed into a native field never reach the layer's
  key path, so a dialog's own key handling sees only Return, Escape and Tab
  (passed on explicitly).
- **Unaligned reads.** Sprite offsets are read as `DWORD` at 2-byte-aligned
  addresses; arm64 and x86-64 allow that, but it is undefined in C. The build
  uses `-fno-strict-aliasing`; checked with UBSan once.
- **macOS habits in the shared layer.** The data-folder search in `core.c`
  also looks at `../Resources/WINLEMM` (an app bundle; harmless elsewhere),
  and the desktop's height leaves 28 pixels for a system caption (the macOS
  value). To revisit on `linux-64` and `windows-64`.
- **Windows-64 and Linux later.** `windows-64` becomes a plain Win32 x64 build
  of the game without the layer (the system has Win32); `linux-64` gets a GTK
  backend of `plat.h`. Until then CMake builds only on macOS.

---

## 8. Results and how to check them

`tools/port/run.sh NAME UPDATES 'commands' BASELINE` runs the native game under
the layer's test harness with the Wine runs' player and settings (and the
client size of the Wine window), and compares its lemming log with a log of
the original game made by `tools/wine/run.sh`:

| Run | Commands | Result |
|---|---|---|
| `l1` | play level 1, 300 updates | MATCH `hd-base-l1` (301 updates) |
| `dig` | `waitupd 200;click 332 395;waitupd 228;click 596 138` (digger) | MATCH `hd-base-dig` (403 updates) |
| `nuke` | `waitupd 100;click 552 395;waitupd 110;click 552 395;waitupd 120;dbl 552 395` | MATCH `s43-nuke-orig2` (264 updates, to the end of the level); the end screen follows, no crash |
| `sdig` | the `dig` run with `sclick` (system mouse events through AppKit, hit testing, capture and the docked panel) | MATCH `hd-base-dig` |
| `skey` | `dig` with the digger chosen by the key `8` (`skey 8`: a system key event, `TranslateMessage`, accelerator table) | MATCH `hd-base-dig` |
| `smenu` | title screen, `smenu File/Exit` (the macOS menu bar: `WM_INITMENU(POPUP)`, `WM_MENUSELECT`, `WM_COMMAND`) | the game ends, exit code 0 |
| `squit` | title screen, `smenu App/Quit Lemmings` (the application menu, `WM_CLOSE`) | the game ends, exit code 0 |
| `zoomclk` | Options, `sclick` on the zoom radio button "2" (a native `NSButton`), OK | the level is drawn at 2x (frame 767 high) |
| `opts` | Options, `sclick` on Music On and Low Resolution | the controls' states (`ctls`) as `tools/port/expect/opts.txt` |
| `user` | Select Player, `sclick` in the name field, `skey x`, `skey y`, `sclick` on the list's row | as `expect/user.txt` (name "Testerxy", then "Tester" from the list) |
| `level` | Choose Level, `sdbl` on two tree rows (open Original Levels, then Fun), `sclick` on a level | as `expect/level.txt` (8 rows, the level's details shown) |
| `again` | nuke, Replay Level from the end screen, nuke again, replay a third time, `menu Exit` | levels keep running (`StartGame` after `EndGame` with the stale dock list), exit code 0, no crash report |

`tools/port/checks.sh [NAME...]` runs them all (except `again`) and prints one
line each; `EXE=` picks the binary (for instance the app bundle's). The
`expect/` files were made with the layer's drawn controls
(`LEMMINGS_NATIVE_CONTROLS=-` still gives those), so the dialog checks compare
the native controls with them.

`click`/`dbl`/`key` post window messages straight to the window under the
point, like the Wine harness; `sclick X Y`, `sdbl X Y`, `sclickw X Y` (window
coordinates) and `skey NAME` (a key name: a letter or digit, Return, Escape,
Tab, Space, Backspace, the arrows, F1-F3, F10) are posted to AppKit as
`NSEvent`s and go through its hit testing, native controls and the layer's
input translation; `smenu MENU/ITEM` chooses through the menu bar (`smenu ?`
lists it). `ctls` logs the dialog's controls (class, id, text, state), `shot
NAME` saves the framebuffer as a BMP, `wshot NAME` the window as AppKit shows
it, native controls included, as a PNG. A click on a native control posts its
button-up with the button-down: the control tracks the mouse in a loop of its
own until the button comes up, and the harness can't run meanwhile.

With `START=` it stays on the title screen: the registration, Select Player,
Choose Level (tree, preview, details), Options and About dialogs were opened
and photographed that way. With sound and music on (`SOUND=1`) the level-1 log
still matches, the MCI trace shows the tune opened, played with notification,
stopped and closed, and the nuke run plays its four sound effects (each one
ends, so the next one starts).

Known differences from the original on Windows: the text drawn by the layer
(the game's own windows, static text) is its own bitmap font; a top-level
window's frame and caption are the system's (so the game's window is its
client size); the menus are in the macOS menu bar, with About and Quit also in
the application menu; dialog controls are AppKit's (Aqua look, the system
font, the mini size where a Windows label is too wide for the small one); the
dock drag outline is not drawn; Help opens the English PDF manual. The zoom
shortcuts in the accelerator table (Alt+F1/F2/F3, commands 0x16f/0x171/0x173)
do nothing, as in the original, which has no handler for them: zoom is set in
the Options dialog.

## 9. Decisions

Asked on 2026-11-09; the user's answers:

- **Approach:** the Win32 subset of section 1 (approved; then on SDL3).
- **Music:** each operating system's own MIDI synthesizer (section 3, Music).
- **Third-party code:** small permissively licensed files may be added under
  `port/third_party/` with their licences.

The options that were offered for the music:

1. **Music synthesizer** (the MIDI files need one; nothing in the original data
   can play them):
   - *TinySoundFont* + *TinyMidiLoader* (MIT, two header files added to
     `port/`), with a General MIDI SoundFont (`.sf2`). By default the small
     *Vintage Dreams Waves v2* (314 KB, Ian Wilson 1996–98, freeware that may be
     redistributed unaltered with its notice; FluidSynth ships it, and Homebrew's
     `fluid-synth` put a copy on this Mac) shipped with the game, and any other
     SoundFont (for example GeneralUser GS, about 30 MB) chosen by a setting. No
     extra library, music works out of the box on all three platforms.
     Recommended.
   - *FluidSynth* (LGPL, a system library: Homebrew, apt, vcpkg) with a
     SoundFont as above. More complete, one more dependency on every platform.
   - An *FM synthesizer* emulating a 1995 Sound Blaster (OPL3), with a built-in
     instrument set: no SoundFont, period sound; more code and licence checks.
   - Each operating system's own MIDI synth (Core Audio's on macOS, `midiOut` on
     Windows; Linux has none built in): no files, but platform code outside the
     shared layer. **Chosen.**

Asked on 2026-11-10 ("can we drop SDL and try native toolkits"); the user's
answers:

- **Toolkit:** each system's own. macOS: AppKit and Core Audio inside the
  Win32 layer (native windows, the macOS menu bar, alerts); `windows-64`: a
  plain Win32 x64 build without the layer, later; `linux-64`: a GTK backend,
  later.
- **Dialog controls:** the system's own controls (buttons, check boxes, text
  fields, lists, the level tree), not the layer's drawn ones.
- **SDL:** deleted once the AppKit backend passed the same checks (it did:
  stage by stage, every check of section 8 on each).

The game sources' `PORT_SDL` macro kept its name: it marks the native 64-bit
build whatever the backend, and renaming it would touch every game file it
guards.

---

## Appendix A. Win32 functions per source file

From the MinGW build's objects (`build/obj`), without `src/crt`.

| File | Functions |
|---|---|
| `filehand/filehand.c` | CloseHandle CreateFileA ExitProcess GetFileSize MessageBoxA ReadFile wsprintfA |
| `gameover/gameover.c` | LoadStringA |
| `lemmings/about.cpp` | CreateFontA DeleteObject EndDialog GetDC GetDeviceCaps GetDlgItem GetDlgItemTextA GetModuleFileNameA GetVersion GetWindow GetWindowRect ReleaseDC SendMessageA SetDlgItemTextA SetWindowPos lstrcatA lstrcpyA lstrlenA midiOutGetDevCapsA wsprintfA |
| `lemmings/dockwnd.cpp` | ClientToScreen CreateHatchBrush DefWindowProcA DeleteObject GetAsyncKeyState GetClientRect GetCursorPos GetDC GetDesktopWindow GetSysColor GetWindowLongA GetWindowRect IntersectRect LoadCursorA PatBlt ReleaseDC SelectObject SendMessageA SetCursor SetParent SetWindowLongA SetWindowPos ShowWindow |
| `lemmings/endscreen.cpp` | BeginPaint DefWindowProcA DeleteObject DestroyWindow DialogBoxParamA EndPaint GetDC PostMessageA RealizePalette ReleaseCapture ReleaseDC SelectPalette SetCapture SetWindowTextA ShowWindow WinHelpA wsprintfA |
| `lemmings/frame.cpp` | AdjustWindowRect BeginPaint CreatePalette CreateSolidBrush CreateWindowExA DefWindowProcA DeleteObject DestroyMenu EndPaint FindWindowA GetClientRect GetDC GetMenu GetPaletteEntries GetSysColor GetSystemMetrics GetSystemPaletteEntries GetWindowRect InvalidateRect LoadCursorA LoadIconA LoadMenuA PatBlt PostQuitMessage RealizePalette RegisterClassA ReleaseDC SelectObject SelectPalette SendMessageA SetMenu SetWindowPos ShowWindow UnrealizeObject UpdateColors wsprintfA |
| `lemmings/frontend.cpp` | BeginPaint CreateDCA DefWindowProcA DeleteDC DeleteObject DestroyWindow DialogBoxParamA EndPaint GetDC GetDeviceCaps PostMessageA RealizePalette ReleaseCapture ReleaseDC SelectPalette SendMessageA SetCapture SetWindowPos SetWindowTextA WinHelpA wsprintfA |
| `lemmings/gameview.cpp` | BeginPaint ChildWindowFromPoint ClientToScreen CreateWindowExA DefWindowProcA EndPaint GetAsyncKeyState GetClientRect GetCursor GetCursorPos GetDC GetParent GetStockObject GetSystemMetrics GetWindowRect InvalidateRect KillTimer LoadIconA PostMessageA PtInRect RealizePalette RegisterClassA ReleaseCapture ReleaseDC ScreenToClient ScrollWindow SelectPalette SendMessageA SetCapture SetCursor SetScrollPos SetScrollRange SetTimer SetWindowPos SetWindowTextA UpdateWindow WindowFromPoint wsprintfA |
| `lemmings/icons.cpp` | CreateFontA CreateSolidBrush DefWindowProcA DeleteObject ExtTextOutA FillRect GetDC GetDesktopWindow GetDeviceCaps GetStockObject GetSysColor GetTextMetricsA InvalidateRect KillTimer LoadCursorA MessageBoxA MulDiv PostMessageA ReleaseCapture ReleaseDC SelectObject SendMessageA SetBkColor SetBkMode SetCapture SetCursor SetTimer SetWindowPos ShowWindow wsprintfA |
| `lemmings/idle.cpp` | ExitProcess MessageBoxA PostMessageA Sleep timeGetTime wsprintfA |
| `lemmings/levelsel.cpp` | BeginPaint CreateBrushIndirect CreateCompatibleDC CreatePalette DefWindowProcA DeleteDC DeleteObject DialogBoxParamA EnableWindow EndDialog EndPaint ExtTextOutA GetClientRect GetDlgItem GetLastError GetObjectA GetPaletteEntries GetStockObject GetSysColor GetTextExtentPointA GetWindowTextA GetWindowWord InvalidateRect MessageBoxA PostMessageA RealizePalette Rectangle RegisterClassA SelectObject SelectPalette SendMessageA SetBkColor SetWindowTextA SetWindowWord StretchBlt UnrealizeObject _lclose _llseek _lopen _lread _lwrite wsprintfA |
| `lemmings/mainwnd.cpp` | ClientToScreen CreateWindowExA DefWindowProcA DestroyWindow DialogBoxParamA EnableMenuItem GetClientRect GetDC GetDeviceCaps GetSystemMetrics GetWindowRect GetWindowTextA LoadCursorA LoadIconA RegisterClassA ReleaseDC SendMessageA SetWindowPos SetWindowTextA ShowWindow WinHelpA |
| `lemmings/map.cpp` | BitBlt CreateCompatibleDC CreateDIBSection CreatePalette DefWindowProcA DeleteDC DeleteObject GetDC GetNearestPaletteIndex GetSystemPaletteEntries InvalidateRect KillTimer LoadCursorA ReleaseDC SelectObject SendMessageA SetCursor SetTimer SetWindowPos ShowWindow |
| `lemmings/midi.cpp` | MessageBoxA mciGetErrorStringA mciSendCommandA timeGetTime wsprintfA |
| `lemmings/options.cpp` | CheckDlgButton EnableWindow EndDialog GetDlgItem GetSysColor IsDlgButtonChecked MessageBoxA PostMessageA |
| `lemmings/register.cpp` | EndDialog GetDesktopWindow GetDlgItem GetWindowTextA MessageBoxA wsprintfA |
| `lemmings/registry.cpp` | MessageBoxA RegCloseKey RegCreateKeyExA RegDeleteKeyA RegEnumKeyExA RegFlushKey RegOpenKeyExA RegQueryValueExA RegSetValueExA wsprintfA |
| `lemmings/settings.cpp` | DeleteFileA DialogBoxParamA ExitProcess GetClientRect GetDesktopWindow MessageBoxA PtInRect _filelength _fileno _lclose _lopen _lread wsprintfA |
| `lemmings/splash.cpp` | BeginPaint BitBlt CreateCompatibleDC CreateFontA CreateSolidBrush CreateWindowExA DefWindowProcA DeleteDC DeleteObject DestroyWindow EndPaint GetClientRect GetDC GetDesktopWindow GetDeviceCaps GetSysColor GetSystemMetrics GetWindowDC LoadCursorA MulDiv PatBlt RealizePalette RegisterClassA ReleaseDC SelectObject SelectPalette SetBkMode ShowWindow TextOutA UpdateWindow wsprintfA |
| `lemmings/statusbar.cpp` | BeginPaint CreateFontA CreateSolidBrush CreateWindowExA DefWindowProcA DeleteObject EndPaint ExtTextOutA FillRect GetClientRect GetDC GetDeviceCaps GetStockObject GetSysColor GetSystemMetrics GetTextMetricsA GetWindowLongA InvalidateRect LoadCursorA MulDiv RegisterClassA ReleaseDC SelectObject SendMessageA SetBkColor SetTextColor SetWindowLongA lstrlenA wsprintfA |
| `lemmings/treelist.cpp` | BitBlt CopyRect CreateCompatibleDC CreateDIBitmap DeleteDC DeleteObject DrawFocusRect ExtTextOutA FindResourceA FreeResource GetClientRect GetDC GetSysColor GetTextMetricsA LoadResource LockResource ReleaseDC SelectObject SendMessageA SetBkColor SetTextColor lstrlenA |
| `lemmings/user.cpp` | CreateDirectoryA DialogBoxParamA EnableWindow EndDialog ExitProcess GetDlgItem GetLastError GetWindowTextA GetWindowsDirectoryA IsWindowEnabled MessageBoxA SendMessageA SetFocus SetWindowTextA _filelength _fileno _unlink wsprintfA |
| `lemmings/window.cpp` | AdjustWindowRect BeginPaint ChildWindowFromPoint ClientToScreen CreateWindowExA DefWindowProcA DestroyWindow EndPaint ExtTextOutA GetClientRect GetCursorPos GetDC GetLastError GetWindowLongA LoadIconA PtInRect RegisterClassA ReleaseDC ScreenToClient SetBkColor SetCursor SetWindowLongA SetWindowPos ShowWindow |
| `lemmings/winmain.cpp` | DestroyCursor DispatchMessageA ExitProcess GetCurrentDirectoryA GetDC GetDeviceCaps GetLastError GetSystemMetrics GetVersion GlobalMemoryStatus LoadAcceleratorsA LoadCursorA LoadStringA MessageBoxA PeekMessageA PostMessageA ReleaseDC SetWindowPos ShowWindow TranslateAcceleratorA TranslateMessage WaitMessage timeGetTime wsprintfA |
| `loadlvl/loadlvl.c` | ExitProcess MessageBoxA _lwrite wsprintfA |
| `process/lemmings.cpp` | GetAsyncKeyState MessageBoxA |
| `process/objects.cpp`, `process/process.cpp`, `process/script.cpp` | ExitProcess MessageBoxA wsprintfA (and `fopen`/`fgetc` in `script.cpp`) |
| `process/wave.cpp` | DefWindowProcA DestroyWindow SetWindowTextA lstrcatA lstrcpyA mmioAscend mmioClose mmioDescend mmioOpenA mmioRead waveOutClose waveOutGetDevCapsA waveOutGetNumDevs waveOutGetPosition waveOutOpen waveOutPrepareHeader waveOutReset waveOutUnprepareHeader waveOutWrite |
| `readdib/readdib.c` | CreateDCA CreateDIBitmap CreatePalette DeleteDC ExitProcess MessageBoxA RealizePalette SelectPalette wsprintfA |
| `render/render.c` | BitBlt CreateCompatibleDC CreateDIBSection DeleteDC DeleteObject ExitProcess GdiFlush MessageBoxA SelectObject StretchBlt StretchDIBits timeGetTime wsprintfA |
| `vsmem/vsmem.c` | ExitProcess GlobalAlloc GlobalFree GlobalLock GlobalUnlock MessageBoxA OutputDebugStringA wsprintfA |
