/* The DLL's own entry point: not used (the runtime's entry point is the DLL's). */
#include "lemming.h"
#include "vscrt.h"

void WINAPI LibMain(HINSTANCE hinst);

/* 0x100065c0 */
BOOL WINAPI DllEntryPoint(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    if (reason == DLL_PROCESS_ATTACH)
        LibMain(hinst);
    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    return TRUE;
}
