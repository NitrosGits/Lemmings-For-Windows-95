/*
 * READDIB.DLL: device-independent bitmaps. A DIB here is a .BMP file without
 * its BITMAPFILEHEADER, kept in one VSMEM block: the info header (or an old
 * OS/2 core header), the colour table, then the pixels.
 */
#ifndef READDIB_H
#define READDIB_H

#include <windows.h>

#if defined(READDIB_BUILD) && defined(_MSC_VER)
#define DIBAPI __declspec(dllexport)
#else
#define DIBAPI
#endif

typedef BITMAPINFOHEADER *PDIB;

#ifdef __cplusplus
extern "C" {
#endif

DIBAPI DWORD WINAPI GetDibInfoHeaderSize(PDIB dib);
DIBAPI WORD WINAPI GetDibWidth(PDIB dib);
DIBAPI WORD WINAPI GetDibHeight(PDIB dib);
DIBAPI WORD WINAPI GetDibBits(PDIB dib);        /* bits per pixel */
DIBAPI WORD WINAPI GetDibPlanes(PDIB dib);
DIBAPI BYTE *WINAPI GetDibBitsAddr(PDIB dib);   /* the pixels */
DIBAPI BOOL WINAPI FreeDib(PDIB dib);
DIBAPI PDIB WINAPI ReadDib(const char *name);
DIBAPI HPALETTE WINAPI GetPalette(PDIB dib);
DIBAPI PDIB WINAPI DoubleDib(PDIB dib);         /* a copy at twice the size */
DIBAPI PDIB WINAPI FlipXDib(PDIB dib);          /* a mirrored copy */
DIBAPI HBITMAP WINAPI Dib2HBITMAP(PDIB dib, HPALETTE *pal);
DIBAPI HBITMAP WINAPI FlipXDib2HBITMAP(PDIB dib, HPALETTE *pal);
DIBAPI HBITMAP WINAPI LoadDIBitMap(const char *name, HPALETTE *pal);

#ifdef __cplusplus
}
#endif

#endif
