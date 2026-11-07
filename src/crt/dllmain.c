/*
 * MinGW builds only: the DLLs' entry point. The originals used the Visual C++
 * runtime's _DllMainCRTStartup, which initialises the runtime and calls the
 * runtime's default DllMain (none of the game's DLLs has its own); our runtime
 * needs no initialising, so all that is left is running the C++ modules'
 * static constructors (PROCESS.DLL's global objects).
 */
#include <windows.h>
#include "vscrt.h"

/* The linker's list of constructors: -1 (or a count), the functions, 0 */
typedef void (*CTOR)(void);
extern CTOR __CTOR_LIST__[];

static void RunConstructors(void)
{
    unsigned long n = (unsigned long)__CTOR_LIST__[0];

    if (n == (unsigned long)-1)
        for (n = 0; __CTOR_LIST__[n + 1]; n++)
            ;
    for (; n > 0; n--)
        __CTOR_LIST__[n]();
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    (void)hinst;
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH)
        RunConstructors();
    return TRUE;
}

/* What a call to a pure virtual function would reach (none is ever made) */
void __cxa_pure_virtual(void)
{
    ExitProcess(255);
}

BOOL WINAPI _CRT_INIT(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    (void)hinst;
    (void)reason;
    (void)reserved;
    return TRUE;
}
