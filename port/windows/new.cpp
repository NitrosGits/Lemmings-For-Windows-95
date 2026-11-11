/*
 * The 64-bit Windows build: operator new and delete on malloc, as Visual
 * C++ 2.2's were (no exceptions: a failed new gives 0, which the game tests
 * for), so the program needs no C++ runtime library. PROCESS.DLL's own pair
 * (onto VSMEM) is left out of this build, see src/process/lemmings.cpp.
 */
#include <stdlib.h>

void *operator new(size_t n) { return malloc(n ? n : 1); }
void *operator new[](size_t n) { return malloc(n ? n : 1); }
void operator delete(void *p) { free(p); }
void operator delete[](void *p) { free(p); }

/* what a call to a pure virtual function would reach (none is ever made) */
extern "C" void __cxa_pure_virtual(void)
{
    abort();
}
