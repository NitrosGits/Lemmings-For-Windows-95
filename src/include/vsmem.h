/* VSMEM.DLL: tracked GlobalAlloc memory. */
#ifndef VSMEM_H
#define VSMEM_H

#include <windows.h>

#if defined(VSMEM_BUILD) && defined(_MSC_VER)
#define VSMEMAPI __declspec(dllexport)  /* exports _VSMEM_Allocate@8 etc., as the original */
#elif defined(VSMEM_BUILD)
#define VSMEMAPI                        /* MinGW: names come from vsmem.def */
#else
#define VSMEMAPI                        /* the game calls these through import thunks */
#endif

#ifdef __cplusplus
extern "C" {
#endif

VSMEMAPI void *WINAPI VSMEM_Allocate(DWORD size, const char *name);
VSMEMAPI void WINAPI VSMEM_Free(void *ptr);
VSMEMAPI void WINAPI VSMEM_FreeAllMemory(void);
VSMEMAPI DWORD WINAPI VSMEM_GetUsed(void);
VSMEMAPI void WINAPI VSMEM_DumpMemoryArray(void);


#ifdef __cplusplus
}
#endif

#endif
