/*
 * MinGW builds only: LEMMINGS.EXE's startup and the runtime pieces it takes
 * from the Visual C++ runtime (LIBC.LIB in the original): the entry point,
 * which runs the C++ static constructors and calls WinMain the way
 * WinMainCRTStartup does, the heap (malloc/free, operator new/delete) on the
 * process heap, and what a pure virtual call would reach.
 */
#include <windows.h>
#include "vscrt.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

/* The linker's lists: -1 (or a count), the functions, 0 */
typedef void (*CTOR)(void);
extern CTOR __CTOR_LIST__[];
extern CTOR __DTOR_LIST__[];

static void RunConstructors(void)
{
    unsigned long n = (unsigned long)__CTOR_LIST__[0];

    if (n == (unsigned long)-1)
        for (n = 0; __CTOR_LIST__[n + 1]; n++)
            ;
    for (; n > 0; n--)
        __CTOR_LIST__[n]();
}

static void RunDestructors(void)
{
    unsigned long i;

    for (i = 1; __DTOR_LIST__[i]; i++)
        __DTOR_LIST__[i]();
}

/* The command line after the program's name, as LIBC passes it */
static char *SkipProgramName(char *p)
{
    if (*p == '"') {
        for (p++; *p && *p != '"'; p++)
            ;
        if (*p == '"')
            p++;
    } else
        while (*p > ' ')
            p++;
    while (*p && *p <= ' ')
        p++;
    return p;
}

void WinMainCRTStartup(void)
{
    STARTUPINFOA si;
    int ret;

    si.dwFlags = 0;
    GetStartupInfoA(&si);
    RunConstructors();
    ret = WinMain(GetModuleHandleA(NULL), NULL, SkipProgramName(GetCommandLineA()),
                  si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    RunDestructors();
    ExitProcess(ret);
}

void *malloc(size_t n)
{
    return HeapAlloc(GetProcessHeap(), 0, n ? n : 1);
}

void free(void *p)
{
    if (p)
        HeapFree(GetProcessHeap(), 0, p);
}

void *realloc(void *p, size_t n)
{
    if (!p)
        return malloc(n);
    return HeapReAlloc(GetProcessHeap(), 0, p, n ? n : 1);
}

/* operator new/delete and their array forms (Itanium-mangled names) */
void *_Znwj(unsigned n) { return malloc(n); }
void *_Znaj(unsigned n) { return malloc(n); }
void _ZdlPv(void *p) { free(p); }
void _ZdaPv(void *p) { free(p); }

void __cxa_pure_virtual(void)
{
    ExitProcess(255);
}
