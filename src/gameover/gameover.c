/*
 * GAMEOVER.DLL - the lines of text shown when the game is finished, from a
 * string table.
 */
#include <windows.h>

#define GAMEOVER_BUILD
#include "gameover.h"
#include "vscrt.h"

#define GO_MAXSTRINGS 5

/* Never set (see DllEntryPoint), so the strings come from LEMMINGS.EXE's own
   string table, not this DLL's. */
static HINSTANCE g_hinst = NULL;

/* 0x10001000 */
void WINAPI GO_LibMain(HINSTANCE hinst)
{
    g_hinst = hinst;
}

/*
 * Symbol padding (see docs/DECOMP.md): the original had more declarations
 * before this point, and Visual C++ 2.x picks registers by symbol position.
 */
extern int pad_go_strings0, pad_go_strings1, pad_go_strings2, pad_go_strings3, pad_go_strings4, pad_go_strings5, pad_go_strings6, pad_go_strings7, pad_go_strings8, pad_go_strings9;

/* 0x10001010: loads up to *count (at most 5) strings of up to 128 characters */
BOOL WINAPI GO_Strings(char **strings, int *count)
{
    int n = *count;
    int i;

    if (n >= GO_MAXSTRINGS)
        n = GO_MAXSTRINGS;
    for (i = 0; i < n; i++)
        LoadStringA(g_hinst, i, strings[i], 128);
    *count = n;
    return TRUE;
}

/* 0x10001060: unused, as in VSMEM.DLL (the runtime's entry point is the DLL's) */
BOOL WINAPI DllEntryPoint(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    if (reason == DLL_PROCESS_ATTACH)
        GO_LibMain(hinst);
    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    return TRUE;
}
