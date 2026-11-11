/*
 * Included first into every source file of the game in the 64-bit Windows
 * build (-include win64_prefix.h; PORTING.md, section 10). The game runs on
 * the real Win32 here, so there is no layer: only the few calls a 64-bit
 * program on today's Windows can no longer make as the game did in 1995 go
 * to port/windows/win64.c instead:
 *   - the registry: a 64-bit program may not write HKEY_LOCAL_MACHINE (and
 *     gets no "virtual store" as old 32-bit programs do), so the game's key
 *     is the user's own, and "ProgramFilesDir" names a folder the user may
 *     write (the players' level databases);
 *   - GetDeviceCaps: a 256-colour display, so the game does not warn about
 *     the colour depth at every start;
 *   - WinHelp: Windows has no viewer for LEMMINGS.HLP any more, the manual
 *     opens instead;
 *   - DialogBoxParam: the game's dialog procedures return a 32-bit BOOL,
 *     Win64 wants an INT_PTR;
 *   - GetCursorPos, GetAsyncKeyState: the real ones, or the test harness's
 *     pointer and keys in a scripted run;
 *   - rand/srand: Visual C++'s generator, one per module, as each DLL had its
 *     own copy of the runtime (the build defines PORT_MODULE_RAND to a name
 *     per module).
 * PORT_SDL (the name the first native port gave it) marks the 64-bit build
 * in the game's sources.
 */
#ifndef WIN64_PREFIX_H
#define WIN64_PREFIX_H

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <io.h>

#ifndef PORT_SDL
#define PORT_SDL 1
#endif

/* Win64's headers have only the pointer-sized name */
#ifndef GWL_USERDATA
#define GWL_USERDATA GWLP_USERDATA
#endif

#ifdef __cplusplus
extern "C" {
#endif

LONG WINAPI port_RegOpenKeyExA(HKEY parent, LPCSTR subkey, DWORD options, REGSAM access, PHKEY key);
LONG WINAPI port_RegCreateKeyExA(HKEY parent, LPCSTR subkey, DWORD reserved, LPSTR cls, DWORD options, REGSAM access,
                                 LPSECURITY_ATTRIBUTES sa, PHKEY key, LPDWORD disposition);
LONG WINAPI port_RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size);
int WINAPI port_GetDeviceCaps(HDC hdc, int index);
BOOL WINAPI port_WinHelpA(HWND hwnd, LPCSTR file, UINT command, ULONG_PTR data);
INT_PTR WINAPI port_DialogBoxParamA(HINSTANCE inst, LPCSTR name, HWND parent, DLGPROC proc, LPARAM param);
BOOL WINAPI port_GetCursorPos(LPPOINT pt);
SHORT WINAPI port_GetAsyncKeyState(int vk);
int port_rand(unsigned int *seed);

/* What the game tells a port with native views (PORTING.md, section 5). Here
   the game draws its own skill panel and status bar; the about box gets the
   real system's name and memory. */
int port_native_icon(HWND hwnd, const BYTE *sprite, int frame, const RGBQUAD *colours, int count, int pressed);
int port_native_status(HWND hwnd, const char *const *texts, int n);
int port_native_system(char *os, int osSize, char *memory, int memorySize);

#ifdef __cplusplus
}
#endif

#define RegOpenKeyExA port_RegOpenKeyExA
#define RegCreateKeyExA port_RegCreateKeyExA
#define RegQueryValueExA port_RegQueryValueExA
#define GetDeviceCaps port_GetDeviceCaps
#define WinHelpA port_WinHelpA
#define DialogBoxParamA port_DialogBoxParamA
#define GetCursorPos port_GetCursorPos
#define GetAsyncKeyState port_GetAsyncKeyState

#ifdef PORT_MODULE_RAND
/* One seed per module, like the per-DLL C runtimes of the original. Each
   module's rand() users are in one file (render.c, midi.cpp), so a seed per
   file is a seed per module. */
static unsigned int PORT_MODULE_RAND __attribute__((unused)) = 1;
#define rand() port_rand(&PORT_MODULE_RAND)
#define srand(s) ((void)(PORT_MODULE_RAND = (unsigned int)(s)))
#endif

#endif
