/* C runtime entry points the game's sources call directly. */
#ifndef VSCRT_H
#define VSCRT_H

#include <windows.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialises (attach) or shuts down (detach) the C runtime for a DLL. */
BOOL WINAPI _CRT_INIT(HINSTANCE hinst, DWORD reason, LPVOID reserved);

#ifdef __cplusplus
}
#endif

#endif
