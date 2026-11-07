/*
 * READDIB.DLL - loading .BMP files and turning them into GDI bitmaps and
 * palettes. (Its strings name it READDIB.CPP, but it was compiled as C.)
 *
 * The copies DoubleDib and FlipXDib make assume an 8-bit DIB with a
 * 40-byte header and a 256-colour table.
 */
#include <windows.h>

#define READDIB_BUILD
#include "readdib.h"
#include "filehand.h"
#include "vsmem.h"
#include "vscrt.h"

#define DIB_HEADER  (sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD))

/* 0x10001000 */
void WINAPI DIB_LibMain(HINSTANCE hinst)
{
}

/* 0x10001010 */
DWORD WINAPI GetDibInfoHeaderSize(PDIB dib)
{
    return dib->biSize;
}

/* 0x10001020 */
WORD WINAPI GetDibWidth(PDIB dib)
{
    if (GetDibInfoHeaderSize(dib) == sizeof(BITMAPCOREHEADER))
        return ((BITMAPCOREHEADER *)dib)->bcWidth;
    return (WORD)dib->biWidth;
}

/* 0x10001040 */
WORD WINAPI GetDibHeight(PDIB dib)
{
    if (GetDibInfoHeaderSize(dib) == sizeof(BITMAPCOREHEADER))
        return ((BITMAPCOREHEADER *)dib)->bcHeight;
    return (WORD)dib->biHeight;
}

/* 0x10001060 */
WORD WINAPI GetDibBits(PDIB dib)
{
    if (GetDibInfoHeaderSize(dib) == sizeof(BITMAPCOREHEADER))
        return ((BITMAPCOREHEADER *)dib)->bcBitCount;
    return dib->biBitCount;
}

/* 0x10001080 */
WORD WINAPI GetDibPlanes(PDIB dib)
{
    if (GetDibInfoHeaderSize(dib) == sizeof(BITMAPCOREHEADER))
        return ((BITMAPCOREHEADER *)dib)->bcPlanes;
    return dib->biPlanes;
}

/* 0x100010a0 */
BYTE *WINAPI GetDibBitsAddr(PDIB dib)
{
    DWORD palette;
    DWORD colors;
    WORD bits;

    if (GetDibInfoHeaderSize(dib) == sizeof(BITMAPCOREHEADER)) {
        bits = ((BITMAPCOREHEADER *)dib)->bcBitCount;
        palette = (bits != 24 ? 1 << bits : 0) * sizeof(RGBTRIPLE);
    } else {
        bits = dib->biBitCount;
        colors = GetDibInfoHeaderSize(dib) >= 0x24 ? dib->biClrUsed : 0;
        if (colors == 0)
            colors = bits == 24 ? 0 : 1 << bits;
        palette = colors * sizeof(RGBQUAD);
    }
    return (BYTE *)dib + GetDibInfoHeaderSize(dib) + palette;
}

/* 0x10001120 */
BOOL WINAPI FreeDib(PDIB dib)
{
    if (dib == NULL)
        return FALSE;
    VSMEM_Free(dib);
    return TRUE;
}

/*
 * Symbol padding (see docs/DECOMP.md): the original had more declarations
 * before this point, and Visual C++ 2.x picks registers by symbol position.
 */
extern int pad_readdib0, pad_readdib1, pad_readdib2, pad_readdib3, pad_readdib4;

/* 0x10001140: reads a .BMP file into a new DIB */
PDIB WINAPI ReadDib(const char *name)
{
    DWORD done;
    HANDLE file;
    DWORD got;
    char tag[128];
    BITMAPFILEHEADER bf;
    WORD chunk;
    PDIB dib;
    DWORD size;

    file = FILE_ReadOpen(name);
    if (FILE_Read(file, &bf, sizeof bf) != sizeof bf) {
        FILE_Close(file);
        return NULL;
    }
    if (*(WORD *)"BM" != bf.bfType) {
        FILE_Close(file);
        return NULL;
    }
    size = bf.bfSize - sizeof bf;
    wsprintfA(tag, "ReadDib( %s )", name);
    dib = (PDIB)VSMEM_Allocate(size, tag);
    if (dib == NULL) {
        FILE_Close(file);
        return NULL;
    }
    done = 0;
    while (size != 0) {
        chunk = (WORD)(size < 0x4000 ? size : 0x4000);
        got = FILE_Read(file, (BYTE *)dib + done, chunk);
        size -= got;
        done += got;
    }
    FILE_Close(file);
    size = GetDibInfoHeaderSize(dib);
    if (size < sizeof(BITMAPCOREHEADER) || (size > sizeof(BITMAPCOREHEADER) && size < 16)) {
        VSMEM_Free(dib);
        return NULL;
    }
    return dib;
}

/* 0x10001230 */
HPALETTE WINAPI GetPalette(PDIB dib)
{
    LOGPALETTE *lp;
    int colors = 1 << dib->biBitCount;
    HPALETTE pal;
    int i;

    lp = (LOGPALETTE *)VSMEM_Allocate(sizeof(LOGPALETTE) + 255 * sizeof(PALETTEENTRY), "GetPalette");
    if (lp == NULL)
        return NULL;
    lp->palVersion = 0x300;
    lp->palNumEntries = colors;
    for (i = 0; i < colors; i++) {
        lp->palPalEntry[i].peRed = ((BITMAPINFO *)dib)->bmiColors[i].rgbRed;
        lp->palPalEntry[i].peGreen = ((BITMAPINFO *)dib)->bmiColors[i].rgbGreen;
        lp->palPalEntry[i].peBlue = ((BITMAPINFO *)dib)->bmiColors[i].rgbBlue;
        lp->palPalEntry[i].peFlags = 0;
    }
    pal = CreatePalette(lp);
    VSMEM_Free(lp);
    return pal;
}

/*
 * Symbol padding (see docs/DECOMP.md): the original had more declarations
 * before this point, and Visual C++ 2.x picks registers by symbol position.
 */
extern int pad_doubledib0, pad_doubledib1, pad_doubledib2, pad_doubledib3, pad_doubledib4, pad_doubledib5, pad_doubledib6, pad_doubledib7, pad_doubledib8, pad_doubledib9, pad_doubledib10, pad_doubledib11, pad_doubledib12, pad_doubledib13, pad_doubledib14, pad_doubledib15, pad_doubledib16, pad_doubledib17, pad_doubledib18, pad_doubledib19, pad_doubledib20, pad_doubledib21, pad_doubledib22, pad_doubledib23;

/* 0x100012b0: every pixel doubled across and every row doubled down */
PDIB WINAPI DoubleDib(PDIB dib)
{
    int stride2;
    int x;
    BYTE *s;
    int height;
    BYTE *src;
    PDIB big;
    BYTE *s2;
    int width;
    BYTE *bits;
    int y;
    int stride;
    int height2;
    int width2;
    BYTE *d2;
    BYTE *d;
    BYTE *dst;

    width = GetDibWidth(dib);
    height = GetDibHeight(dib);
    bits = GetDibBitsAddr(dib);
    width2 = width * 2;
    height2 = height * 2;
    stride = (width + 3) & ~3;
    stride2 = (width2 + 3) & ~3;
    big = (PDIB)VSMEM_Allocate(stride2 * height2 + DIB_HEADER, "READDIB.CPP : DoubleDib ");
    memcpy(big, dib, DIB_HEADER);
    big->biWidth = width2;
    big->biHeight = height2;
    src = (BYTE *)dib + DIB_HEADER;
    dst = (BYTE *)big + DIB_HEADER;
    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++) {
            *d++ = *s;
            *d++ = *s++;
        }
        d2 = dst + stride2;
        s2 = src;
        for (x = 0; x < width; x++) {
            *d2++ = *s2;
            *d2++ = *s2++;
        }
        dst += stride2 * 2;
        src += stride;
    }
    return big;
}

/*
 * Symbol padding (see docs/DECOMP.md): the original had more declarations
 * before this point, and Visual C++ 2.x picks registers by symbol position.
 */
extern int pad_flipxdib0, pad_flipxdib1, pad_flipxdib2, pad_flipxdib3, pad_flipxdib4, pad_flipxdib5;

/* 0x100013c0: mirrored left to right */
PDIB WINAPI FlipXDib(PDIB dib)
{
    int width = GetDibWidth(dib);
    int y;
    int height = GetDibHeight(dib);
    BYTE *s;
    BYTE *d;
    int x;
    BYTE *dst;
    BYTE *bits = GetDibBitsAddr(dib);
    BYTE *src;
    PDIB flip;
    int stride = (width + 3) & ~3;

    flip = (PDIB)VSMEM_Allocate(stride * height + DIB_HEADER, "READDIB.CPP : FlipXDib ");
    memcpy(flip, dib, DIB_HEADER);
    src = (BYTE *)dib + DIB_HEADER;
    dst = (BYTE *)flip + DIB_HEADER;
    for (y = 0; y < height; y++) {
        s = src;
        d = &dst[width - 1];
        for (x = 0; x < width; x++)
            *d-- = *s++;
        dst += stride;
        src += stride;
    }
    return flip;
}

/* 0x10001480: a device-dependent bitmap for the display, and the DIB's palette */
HBITMAP WINAPI Dib2HBITMAP(PDIB dib, HPALETTE *pal)
{
    HDC dc;
    HBITMAP bm;
    HPALETTE hpal;
    BYTE *bits;
    WORD width, height, planes, bpp;

    dc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    if (dc == NULL)
        return NULL;
    width = GetDibWidth(dib);
    height = GetDibHeight(dib);
    planes = GetDibPlanes(dib);
    bpp = GetDibBits(dib);
    bits = GetDibBitsAddr(dib);
    hpal = GetPalette(dib);
    if (pal != NULL)
        *pal = hpal;
    SelectPalette(dc, hpal, FALSE);
    RealizePalette(dc);
    bm = CreateDIBitmap(dc, dib, CBM_INIT, bits, (BITMAPINFO *)dib, DIB_RGB_COLORS);
    SelectPalette(dc, hpal, FALSE);
    DeleteDC(dc);
    return bm;
}

/* 0x10001520 */
HBITMAP WINAPI FlipXDib2HBITMAP(PDIB dib, HPALETTE *pal)
{
    PDIB flip = FlipXDib(dib);
    HBITMAP bm = Dib2HBITMAP(flip, pal);

    FreeDib(flip);
    return bm;
}

/* 0x10001550 */
HBITMAP WINAPI LoadDIBitMap(const char *name, HPALETTE *pal)
{
    PDIB dib;
    HBITMAP bm;

    dib = ReadDib(name);
    if (dib == NULL)
        return NULL;
    bm = Dib2HBITMAP(dib, pal);
    VSMEM_Free(dib);
    return bm;
}

/* 0x10001590: the shared assertion handler (unreferenced here) */
void VS_AssertFailed(const char *expr, const char *file, unsigned line)
{
    char msg[256];

    wsprintfA(msg, "%s\nin file \"%s\" at line %u\n", expr, file, line);
    MessageBoxA(NULL, msg, "Visual Sciences Assertion Failure", MB_ICONEXCLAMATION);
    ExitProcess(0xaaaa);
}

/* 0x100015e0: unused, as in VSMEM.DLL (the runtime's entry point is the DLL's) */
BOOL WINAPI DllEntryPoint(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    if (reason == DLL_PROCESS_ATTACH)
        DIB_LibMain(hinst);
    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    return TRUE;
}
