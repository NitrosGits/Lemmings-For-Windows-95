/*
 * VSMEM.DLL - Visual Sciences memory manager.
 *
 * Every allocation is a moveable GlobalAlloc block, locked for its lifetime and
 * recorded in a fixed table of 4096 entries with the caller's name for it, so a
 * debug build can list what is allocated (VSMEM_DumpMemoryArray).
 */
#include <windows.h>
#include <string.h>

#define VSMEM_BUILD
#include "vsmem.h"
#include "vscrt.h"

#define VSMEM_MAX_BLOCKS 4096

typedef struct VSMEM_BLOCK {
    HGLOBAL handle;
    void   *ptr;            /* NULL: entry free */
    DWORD   size;
    char    name[128];
} VSMEM_BLOCK;

static VSMEM_BLOCK g_blocks[VSMEM_MAX_BLOCKS];
static int  g_blockCount = 0;
static long g_used = 0;
static long g_peak = 0;
static int  g_initCount = 0;

/* 0x10001000 */
static VSMEM_BLOCK *FindFreeBlock(void)
{
    VSMEM_BLOCK *b = NULL;
    int i;

    for (i = 0; i < VSMEM_MAX_BLOCKS; i++) {
        if (g_blocks[i].ptr == NULL) {
            b = &g_blocks[i];
            g_blockCount++;
            break;
        }
    }
    return b;
}

/* 0x10001040 */
static void ClearBlock(VSMEM_BLOCK *b)
{
    b->ptr = NULL;
    b->size = (DWORD)-1;
    memset(b->name, 0, sizeof b->name);
    g_blockCount--;
}

/* 0x10001070 */
static VSMEM_BLOCK *FindBlock(void *ptr)
{
    int i;

    for (i = 0; i < VSMEM_MAX_BLOCKS; i++)
        if (g_blocks[i].ptr == ptr)
            return &g_blocks[i];
    return NULL;
}

/* 0x100010b0 */
void WINAPI VSMEM_FreeAllMemory(void)
{
    int i;

    for (i = 0; i < VSMEM_MAX_BLOCKS; i++)
        if (g_blocks[i].ptr != NULL)
            VSMEM_Free(g_blocks[i].ptr);
}

/* 0x100010e0 */
void *WINAPI VSMEM_Allocate(DWORD size, const char *name)
{
    VSMEM_BLOCK *b = FindFreeBlock();

    b->handle = GlobalAlloc(GMEM_MOVEABLE, size);
    b->ptr = GlobalLock(b->handle);
    if (b->ptr == NULL) {
        ClearBlock(b);
        MessageBoxA(NULL,
                    "Lemmings was unable to allocate memory it required\n"
                    "Please shut down other applications and restart lemmings\n",
                    "FATAL ERROR", MB_ICONHAND | MB_SYSTEMMODAL);
        VSMEM_FreeAllMemory();
        ExitProcess(0xaaaa);
        return NULL;
    }
    b->size = size;
    strncpy(b->name, name, sizeof b->name);
    g_used += size;
    if (g_peak < g_used)
        g_peak = g_used;
    return b->ptr;
}

/* 0x10001180 */
DWORD WINAPI VSMEM_GetUsed(void)
{
    DWORD total = 0;
    int i;

    for (i = 0; i < VSMEM_MAX_BLOCKS; i++)
        if (g_blocks[i].ptr != NULL)
            total += g_blocks[i].size;
    return total;
}

/* 0x100011a0 */
void WINAPI VSMEM_DumpMemoryArray(void)
{
    char line[128];
    char peak[128];
    int i;

    OutputDebugStringA("VSMEM_DumpMemoryArray...\n-------------------------------\n");
    wsprintfA(peak, "Maximum memory allocated=%ld\n", g_peak);
    OutputDebugStringA(peak);
    for (i = 0; i < VSMEM_MAX_BLOCKS; i++) {
        if (g_blocks[i].ptr != NULL) {
            wsprintfA(line, "%08lx\t:%ld\t:%s\n", g_blocks[i].ptr, g_blocks[i].size, g_blocks[i].name);
            OutputDebugStringA(line);
        }
    }
    OutputDebugStringA("-----------------------------------\nVSMEM_DumpMemoryArray fini...\n");
}

/* 0x10001230 */
void WINAPI VSMEM_Free(void *ptr)
{
    char msg[128];
    VSMEM_BLOCK *b = FindBlock(ptr);

    if (b == NULL) {
        wsprintfA(msg, "VSMEM_Free(%08lx): Trying to free memory we don't own\n", ptr);
        OutputDebugStringA(msg);
        VSMEM_DumpMemoryArray();
        MessageBoxA(NULL, "Trying to free memory we don't own", "VSMEM", MB_OK);
        return;
    }
    ClearBlock(b);
    if (GlobalUnlock(b->handle))
        MessageBoxA(NULL, "Memory is still locked", "VSMEM", MB_OK);
    if (GlobalFree(b->handle))
        MessageBoxA(NULL, "Failed to free the memory", "VSMEM", MB_OK);
    /* ClearBlock has already set size to -1, so this adds one (as the original). */
    g_used -= b->size;
}

/*
 * Library initialisation. The original links it, but its
 * DllEntryPoint (0x10001390, below) is not the DLL's entry point: the C
 * runtime's _DllMainCRTStartup is, with the runtime's default DllMain. So this
 * never runs and the table starts out as zeroed .bss.
 */
/* 0x10001300 */
void WINAPI VSMEM_LibMain(HINSTANCE hinst)
{
    VSMEM_BLOCK *b;

    (void)hinst;
    if (g_initCount == 0) {
        for (b = g_blocks; b < g_blocks + VSMEM_MAX_BLOCKS; b++)
            ClearBlock(b);
        g_blockCount = 0;
    }
    g_initCount++;
}

/* 0x10001340: the shared assertion handler (unreferenced here). */
void VS_AssertFailed(const char *expr, const char *file, unsigned line)
{
    char msg[256];

    wsprintfA(msg, "%s\nin file \"%s\" at line %u\n", expr, file, line);
    MessageBoxA(NULL, msg, "Visual Sciences Assertion Failure", MB_ICONEXCLAMATION);
    ExitProcess(0xaaaa);
}

/*
 * The entry point this DLL was presumably meant to have (it initialises the C
 * runtime around VSMEM_LibMain). It was linked in but not made the entry point,
 * so it is never called.
 */
/* 0x10001390 */
BOOL WINAPI DllEntryPoint(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    if (reason == DLL_PROCESS_ATTACH)
        VSMEM_LibMain(hinst);
    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    return TRUE;
}
