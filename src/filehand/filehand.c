/*
 * FILEHAND.DLL - file access for the game: open for reading, read, size,
 * close. Errors are reported in a message box and end the program.
 */
#include <windows.h>

#define FILEHAND_BUILD
#include "filehand.h"
#include "vscrt.h"

#define CHUNK 0x4000

/* 0x10001000 */
void WINAPI FILE_LibMain(HINSTANCE hinst)
{
}

/* 0x10001010 */
HANDLE WINAPI FILE_ReadOpen(const char *name)
{
    char msg[128];
    OFSTRUCT of;    /* left over from when this used OpenFile */
    HANDLE file;

    memset(&of, 0, sizeof of);
    of.cBytes = sizeof of;
    file = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        wsprintfA(msg, "Unable to open file %s\n, Error Code=%08lx\n", name, file);
        MessageBoxA(NULL, msg, "File Handler", MB_OK);
        ExitProcess(0xbbbb);
    }
    return file;
}

/* 0x100010a0 */
void WINAPI FILE_Close(HANDLE file)
{
    CloseHandle(file);
}

/* 0x100010b0 */
static DWORD ReadChunk(HANDLE file, void *buf, DWORD size)
{
    DWORD read;
    char msg[128];

    read = 0;
    if (!ReadFile(file, buf, size, &read, NULL)) {
        wsprintfA(msg, "Unable to read file chunk size %ld\n, Error Code=%08lx\n", size, read);
        MessageBoxA(NULL, msg, "File Handler", MB_OK);
        ExitProcess(0xbbbb);
    }
    return read;
}

/*
 * Symbol padding (see docs/DECOMP.md): the original had more declarations
 * before this point, and Visual C++ 2.x picks registers by symbol position.
 */
extern int pad_fileread0, pad_fileread1, pad_fileread2, pad_fileread3, pad_fileread4, pad_fileread5, pad_fileread6, pad_fileread7, pad_fileread8, pad_fileread9, pad_fileread10, pad_fileread11, pad_fileread12, pad_fileread13, pad_fileread14, pad_fileread15, pad_fileread16, pad_fileread17, pad_fileread18, pad_fileread19, pad_fileread20, pad_fileread21, pad_fileread22, pad_fileread23;

/* 0x10001130: reads in chunks of 16K; returns the size of the last one read */
DWORD WINAPI FILE_Read(HANDLE file, void *buf, long size)
{
    char *p = buf;

    while (size > CHUNK) {
        ReadChunk(file, p, CHUNK);
        p += CHUNK;
        size -= CHUNK;
    }
    return ReadChunk(file, p, size);
}

/* 0x10001190 */
DWORD WINAPI FILE_Length(HANDLE file)
{
    DWORD high = 0;

    return GetFileSize(file, &high);
}

/* 0x100011c0: unused, as in VSMEM.DLL (the runtime's entry point is the DLL's) */
BOOL WINAPI DllEntryPoint(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    if (reason == DLL_PROCESS_ATTACH)
        FILE_LibMain(hinst);
    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    return TRUE;
}
