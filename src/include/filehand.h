/* FILEHAND.DLL: reading whole files; any failure is fatal. */
#ifndef FILEHAND_H
#define FILEHAND_H

#include <windows.h>

#if defined(FILEHAND_BUILD) && defined(_MSC_VER)
#define FILEAPI __declspec(dllexport)
#else
#define FILEAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

FILEAPI HANDLE WINAPI FILE_ReadOpen(const char *name);
FILEAPI void WINAPI FILE_Close(HANDLE file);
FILEAPI DWORD WINAPI FILE_Read(HANDLE file, void *buf, long size);
FILEAPI DWORD WINAPI FILE_Length(HANDLE file);


#ifdef __cplusplus
}
#endif

#endif
