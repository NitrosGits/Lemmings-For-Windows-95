/*
 * RENDER.DLL - the game's renderer: canvases (8-bit DIB sections), the
 * renderers' slots and their changed-area grids, display lists, and the
 * blitters that draw sprites into canvases.
 */
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>

#define RENDER_BUILD
#include "render.h"
#include "readdib.h"
#include "vsmem.h"
#include "vscrt.h"

#define MAX_CANVASES    32
#define MAX_RENDERERS   10
#define MAX_SLOTS       50
#define MAX_SPRITELISTS 50
#define HANDLE_XOR      0x55aa

#define SLOT_OF(r, i)   (g_renderers[r].slots[i])
#define CANVAS_OF(r, i) (g_canvases[SLOT_OF(r, i).canvas])

typedef struct CANVAS {
    DWORD free;
    WORD owner;
    WORD unused6;
    HBITMAP bitmap;
    BITMAPINFOHEADER header;
    RGBQUAD colors[256];
    WORD width, height;
    WORD clipLeft, clipTop, clipRight, clipBottom;
    BYTE *bits;
} CANVAS;

typedef struct SLOT {
    WORD flags;
    WORD width, height;
    WORD canvas;            /* what is drawn into */
    WORD backup;            /* SLOT_SCREEN: the background, to erase with */
    WORD canvas3;
    HDC dc;                 /* SLOT_SCREEN: a DC with the canvas selected */
    BYTE *pending;          /* cells changed by this frame's drawing */
    BYTE *changed;          /* cells to copy to the screen */
} SLOT;

typedef struct RENDERER {
    DWORD free;
    WORD handle;
    WORD param;
    WORD numSlots;
    WORD unusedA;
    SLOT slots[MAX_SLOTS];
    WORD numSpriteLists;
    WORD unused4be;
    SPRITELIST *spriteLists[MAX_SPRITELISTS];
} RENDERER;


const char g_copyright[] = "DLL for Windows Lemmings (c)1994 Visual Sciences Ltd.";

WORD g_numCanvases = 0;
int g_numRenderers = 0;
int g_unused8 = 0;
DWORD g_blitTime = 0;
DWORD g_copyTime = 0;

static CANVAS g_canvases[MAX_CANVASES];
static RENDERER g_renderers[MAX_RENDERERS];

static int ClipRect(int *x, int *y, int *width, int *height, int *skipX, int *skipY, int left, int top, int right, int bottom);
static void DrawTile(int r, TILEDATA *t, BYTE *bits, WORD slot);
static void DrawTileMasked(int r, TILEDATA *t, BYTE *bits, WORD slot);
static void DrawTileUnder(int r, TILEDATA *t, BYTE *bits, WORD slot);
static void DrawTileBoth(int r, TILEDATA *t, BYTE *bits, WORD slot);
static void DrawTileErase(int r, TILEDATA *t, BYTE *bits, WORD slot);
static void DrawTileEntry(int r, TILEDATA *t, BYTE *bits, WORD slot);
static WORD *GetRLEFrame(RLEDATA *d, int *w, int *h, int *ox, int *oy);
static void DrawRLE(int r, RLEDATA *d, BYTE *bits, WORD slot);
static void DrawRLEMasked(int r, RLEDATA *d, BYTE *bits, WORD slot);
static void DrawRLEUnder(int r, RLEDATA *d, BYTE *bits, WORD slot);
static void DrawRLEBoth(int r, RLEDATA *d, BYTE *bits, WORD slot);
static void DrawRLEErase(int r, RLEDATA *d, BYTE *bits, WORD slot);
static void DrawRLEEntry(int r, RLEDATA *d, BYTE *bits, WORD slot);
static int Random(int n);
static void DrawPixels(int r, PIXELDATA *p, BYTE *bits, WORD slot);
static int NewRenderer(WORD param);
static void FreeCanvas(CANVAS *c);
static void FreeRenderer(int r);
static WORD NewCanvas(int owner);
static SLOT *NewSlot(WORD renderer, WORD flags, WORD *index);
static void MakeDIBSection(WORD canvas, PDIB dib, HDC *pdc, RGBQUAD *colors);
static void CopyDib(WORD canvas, PDIB dib);
static void MarkChanged(int r, WORD slot, int left, int top, int right, int bottom, int which);
static void MarkPoint(int r, WORD slot, int x, int y, int which);
static void NextFrame(int r, WORD slot);
static void BlitTile(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src);
static void BlitTileTopDown(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src);
static void BlitTileClipped(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src);
static void EraseTile(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src);
static void EraseTileClipped(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src);
static void EraseTileTopDown(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src);
static void UnderTile(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src);
static void UnderTileTopDown(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src);
static void MaskTile(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src, BYTE *mask);
static void MaskTileTopDown(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY, int srcW, int srcH, BYTE *src, BYTE *mask);
void PutPixelTopDown(BYTE *bits, int x, int y, int colour, int w);
static void PutPixel(BYTE *bits, int x, int y, int colour, int w, int h);
void CopyBlockTopDown(BYTE *dst, BYTE *src, int x, int y, int w, int h, int stride, int height, int dstStride);
static void CopyBlock(BYTE *dst, BYTE *src, int x, int y, int w, int h, int stride, int height, int dstStride);
static BYTE *UnpackRLE(WORD *spr);
static void BlitRLE(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr);
void BlitRLETopDown(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr);
static void BlitRLEClipped(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr);
static void EraseRLE(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr);
static void EraseRLEClipped(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr);
void EraseRLETopDown(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr);
static void UnderRLE(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr);
void UnderRLETopDown(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr);
static void MaskRLE(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr, BYTE *mask);
void MaskRLETopDown(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX, int skipY, WORD *spr, BYTE *mask);
static void SetBlitters(void);
static void CopyChanged(HDC dst, int dx, int dy, HDC src, int left, int top, int right, int bottom, BYTE *changed, int across, int down, BYTE *bits, BITMAPINFO *bmi);
static void StretchChanged(HDC dst, int dx, int dy, HDC src, int left, int top, int right, int bottom, BYTE *changed, int across, int down, int width, int height, BYTE *bits, BITMAPINFO *bmi);
static void StretchAll(HDC dst, HDC src, int left, int top, int right, int bottom, BYTE *changed, int across, int down, int width, int height);
static void RestoreChanged(int r, WORD slot);
static void *RenderAlloc(DWORD size, const char *tag);
static void RenderFree(void *p);
void VS_AssertFailed(const char *expr, const char *file, unsigned line);

/* 0x10001000 */
void WINAPI RENDER_LibMain(HINSTANCE hinst)
{
    g_numRenderers = 0;
    memset(g_renderers, 0, sizeof g_renderers);
}

/*
 * Copies a screen slot's changed cells to dc (mode 1), all of it (0),
 * stretched to width x height (2), or all of it stretched (3).
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */


extern int pad_renderrender0, pad_renderrender1, pad_renderrender2, pad_renderrender3, pad_renderrender4, pad_renderrender5, pad_renderrender6, pad_renderrender7, pad_renderrender8, pad_renderrender9, pad_renderrender10, pad_renderrender11, pad_renderrender12, pad_renderrender13, pad_renderrender14, pad_renderrender15, pad_renderrender16, pad_renderrender17, pad_renderrender18, pad_renderrender19, pad_renderrender20, pad_renderrender21, pad_renderrender22, pad_renderrender23, pad_renderrender24, pad_renderrender25, pad_renderrender26, pad_renderrender27, pad_renderrender28, pad_renderrender29, pad_renderrender30, pad_renderrender31, pad_renderrender32, pad_renderrender33, pad_renderrender34, pad_renderrender35, pad_renderrender36, pad_renderrender37, pad_renderrender38, pad_renderrender39, pad_renderrender40, pad_renderrender41, pad_renderrender42, pad_renderrender43, pad_renderrender44, pad_renderrender45, pad_renderrender46, pad_renderrender47, pad_renderrender48, pad_renderrender49, pad_renderrender50, pad_renderrender51, pad_renderrender52, pad_renderrender53, pad_renderrender54, pad_renderrender55, pad_renderrender56, pad_renderrender57, pad_renderrender58, pad_renderrender59, pad_renderrender60, pad_renderrender61, pad_renderrender62, pad_renderrender63, pad_renderrender64, pad_renderrender65;

/* 0x10001020 */
void WINAPI Renderer_Render(WORD renderer, HDC dc, WORD slot, int left, int top, int right,
                            int bottom, int width, int height, int mode)
{
    /* Numbering padding for VC++ 2.2 register colouring (docs/C2_REGALLOC.md): 17 unused locals here,
       17 fewer pad_newcanvas externs above NewCanvas keep later functions' numbering unchanged. */
    int dm0;
    int dm1;
    int dm2;
    int dm3;
    int dm4;
    int dm5;
    int dm6;
    int dm7;
    int dm8;
    int dm9;
    int dm10;
    int dm11;
    int dm12;
    int dm13;
    int dm14;
    int dm15;
    int dm16;
    DWORD end;
    CANVAS *c;
    int dy;
    HDC sdc;
    DWORD start;
    int r;
    int across;
    HGDIOBJ old;
    int down;
    SLOT *s;
    int dx;
    BYTE *changed;

    if (renderer == 0xffff)
        return;
    r = (WORD)(renderer ^ HANDLE_XOR);
    s = &g_renderers[r].slots[slot];
    c = &g_canvases[s->canvas];
    changed = SLOT_OF(r, slot).changed;
    across = (SLOT_OF(r, slot).width + 15) >> 4;
    down = (SLOT_OF(r, slot).height + 7) >> 3;
    dx = 0;
    dy = 0;
    if (left & 15) {
        dx = -(left & 15);
        left &= 0xfff0;
    }
    if (top & 7) {
        dy = -(top & 7);
        top &= 0xfff8;
    }
    if (s->flags & SLOT_SCREEN)
        sdc = s->dc;
    else
        MessageBoxA(NULL, "We should not get this", "RENDER_RENDER", MB_OK);
    old = SelectObject(sdc, CANVAS_OF(r, slot).bitmap);
    switch (mode) {
    case 0:
        if (right > across * 16)
            right = across * 16;
        if (bottom > down * 8)
            bottom = down * 8;
        start = timeGetTime();
        BitBlt(dc, dx, 0, right - left + 1, bottom - top + 1, sdc, left, top, SRCCOPY);
        end = timeGetTime();
        break;
    case 1:
        if (right > across * 16)
            right = across * 16;
        if (bottom > down * 8)
            bottom = down * 8;
        start = timeGetTime();
        CopyChanged(dc, dx, dy, sdc, left, top, right, bottom, changed, across, down, c->bits,
                    (BITMAPINFO *)&c->header);
        end = timeGetTime();
        break;
    case 2:
        if (right > across * 16)
            right = across * 16;
        if (bottom > down * 8)
            bottom = down * 8;
        start = timeGetTime();
        StretchChanged(dc, dx, dy, sdc, left, top, right, bottom, changed, across, down, width, height,
                       c->bits, (BITMAPINFO *)&c->header);
        end = timeGetTime();
        break;
    case 3:
        if (right > across * 16)
            right = across * 16;
        if (bottom > down * 8)
            bottom = down * 8;
        start = timeGetTime();
        StretchAll(dc, sdc, left, top, right, bottom, changed, across, down, width, height);
        end = timeGetTime();
        break;
    }
    g_blitTime += end - start;
    GdiFlush();
    if (s->flags & SLOT_SCREEN) {
        start = timeGetTime();
        RestoreChanged(r, slot);
        g_copyTime += timeGetTime() - start;
    }
    NextFrame(r, slot);
    SelectObject(sdc, old);
}

/* 0x100013a0: clips a rectangle to a canvas's clip region; returns whether it was cut */
static int ClipRect(int *x, int *y, int *width, int *height, int *skipX, int *skipY,
                    int left, int top, int right, int bottom)
{
    int clipped = 0;
    int w, h;

    *x -= left;
    *y -= top;
    w = right - left;
    h = bottom - top;
    *skipX = 0;
    *skipY = 0;
    if (*width + *x >= w) {
        *width = w - *x;
        clipped = 1;
    }
    if (*x < 0) {
        *skipX -= *x;
        *width += *x;
        clipped = 1;
        *x = 0;
    }
    if (*height + *y >= h) {
        *height = h - *y;
        clipped = 1;
    }
    if (*y < 0) {
        *skipY -= *y;
        *height += *y;
        clipped = 1;
        *y = 0;
    }
    *x += left;
    *y += top;
    return clipped;
}

/* the blitters, set by SetBlitters (0x10004a60) */
void (*g_blitTile)(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                   int srcW, int srcH, BYTE *src) = 0;
void (*g_blitTileBackup)(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                         int srcW, int srcH, BYTE *src) = 0;
void (*g_eraseTile)(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                    int srcW, int srcH, BYTE *src) = 0;
void (*g_eraseTileBackup)(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                          int srcW, int srcH, BYTE *src) = 0;
void (*g_underTile)(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                    int srcW, int srcH, BYTE *src) = 0;
void (*g_maskTile)(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                   int srcW, int srcH, BYTE *src, BYTE *mask) = 0;
void (*g_blitRLE)(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                  int skipX, int skipY, WORD *spr) = 0;
void (*g_blitRLEBackup)(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                        int skipX, int skipY, WORD *spr) = 0;
void (*g_eraseRLE)(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                   int skipX, int skipY, WORD *spr) = 0;
void (*g_eraseRLEBackup)(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                         int skipX, int skipY, WORD *spr) = 0;
void (*g_underRLE)(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                   int skipX, int skipY, WORD *spr) = 0;
void (*g_maskRLE)(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                  int skipX, int skipY, WORD *spr, BYTE *mask) = 0;
void (*g_copyRect)(BYTE *dst, BYTE *src, int x, int y, int w, int h, int srcW, int srcH, int dstW) = 0;
void (*g_putPixel)(BYTE *bits, int x, int y, int colour, int w, int h) = 0;

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawtile0, pad_drawtile1, pad_drawtile2, pad_drawtile3, pad_drawtile4, pad_drawtile5, pad_drawtile6, pad_drawtile7, pad_drawtile8, pad_drawtile9, pad_drawtile10, pad_drawtile11, pad_drawtile12;

/* 0x10001450: a tile drawn normally */
static void DrawTile(int r, TILEDATA *t, BYTE *bits, WORD slot)
{
    SPRITELIST *list = g_renderers[r].spriteLists[t->list];
    SPRITEREC *rec = &list->frames[t->frame];
    CANVAS *src = &CANVAS_OF(r, list->slot);
    CANVAS *dst;
    int x, y, w, h, skipX, skipY;
    int which;
    BYTE *srcBits;

    dst = &CANVAS_OF(r, slot);
    srcBits = src->bits;
    skipX = 0;
    skipY = 0;
    x = t->x;
    y = t->y;
    w = rec->width;
    h = rec->height;
    ClipRect(&x, &y, &w, &h, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight, dst->clipBottom);
    if (w > 0 && h > 0) {
        skipX += rec->x;
        skipY += rec->y;
        g_blitTile(x, y, w, h, dst->width, dst->height, bits, skipX, skipY, src->width, src->height, srcBits);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawtilemasked0, pad_drawtilemasked1, pad_drawtilemasked2, pad_drawtilemasked3, pad_drawtilemasked4, pad_drawtilemasked5, pad_drawtilemasked6, pad_drawtilemasked7, pad_drawtilemasked8;

/* 0x10001650: a tile drawn where a mask (the backup canvas, or the sheet itself) has pixels */
static void DrawTileMasked(int r, TILEDATA *t, BYTE *bits, WORD slot)
{
    SPRITELIST *list = g_renderers[r].spriteLists[t->list];
    SPRITEREC *rec = &list->frames[t->frame];
    CANVAS *src = &CANVAS_OF(r, list->slot);
    CANVAS *dst;
    int x, y, w, h, skipX, skipY;
    int which;
    BYTE *srcBits;
    BYTE *mask;

    dst = &CANVAS_OF(r, slot);
    srcBits = src->bits;
    if (SLOT_OF(r, slot).backup != 0)
        mask = g_canvases[SLOT_OF(r, slot).backup].bits;
    else
        mask = srcBits;
    skipX = 0;
    skipY = 0;
    x = t->x;
    y = t->y;
    w = rec->width;
    h = rec->height;
    ClipRect(&x, &y, &w, &h, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight, dst->clipBottom);
    if (w > 0 && h > 0) {
        skipX += rec->x;
        skipY += rec->y;
        g_maskTile(x, y, w, h, dst->width, dst->height, bits, skipX, skipY, src->width, src->height, srcBits, mask);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
}

/* 0x10001890: a tile drawn only where the canvas is empty */
static void DrawTileUnder(int r, TILEDATA *t, BYTE *bits, WORD slot)
{
    SPRITELIST *list = g_renderers[r].spriteLists[t->list];
    SPRITEREC *rec = &list->frames[t->frame];
    CANVAS *src = &CANVAS_OF(r, list->slot);
    CANVAS *dst;
    int x, y, w, h, skipX, skipY;
    int which;
    BYTE *srcBits;

    dst = &CANVAS_OF(r, slot);
    srcBits = src->bits;
    skipX = 0;
    skipY = 0;
    x = t->x;
    y = t->y;
    w = rec->width;
    h = rec->height;
    ClipRect(&x, &y, &w, &h, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight, dst->clipBottom);
    if (w > 0 && h > 0) {
        skipX += rec->x;
        skipY += rec->y;
        g_underTile(x, y, w, h, dst->width, dst->height, bits, skipX, skipY, src->width, src->height, srcBits);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawtileboth0, pad_drawtileboth1, pad_drawtileboth2, pad_drawtileboth3, pad_drawtileboth4;

/* 0x10001a90: a tile drawn into the canvas and, on a screen slot, its backup too */
static void DrawTileBoth(int r, TILEDATA *t, BYTE *bits, WORD slot)
{
    SPRITELIST *list = g_renderers[r].spriteLists[t->list];
    SPRITEREC *rec = &list->frames[t->frame];
    CANVAS *src = &CANVAS_OF(r, list->slot);
    CANVAS *dst;
    int x, y, w, h, skipX, skipY;
    int which;
    BYTE *srcBits;
    CANVAS *b;

    dst = &CANVAS_OF(r, slot);
    srcBits = src->bits;
    skipX = 0;
    skipY = 0;
    x = t->x;
    y = t->y;
    w = rec->width;
    h = rec->height;
    ClipRect(&x, &y, &w, &h, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight, dst->clipBottom);
    if (w > 0 && h > 0) {
        skipX += rec->x;
        skipY += rec->y;
        g_blitTile(x, y, w, h, dst->width, dst->height, bits, skipX, skipY, src->width, src->height, srcBits);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
    if (SLOT_OF(r, slot).flags & SLOT_SCREEN) {
        b = &g_canvases[SLOT_OF(r, slot).backup];
        g_blitTileBackup(t->x, t->y, rec->width, rec->height, b->width, b->height, b->bits, rec->x, rec->y,
            src->width, src->height, srcBits);
    }
}

/* 0x10001d30: a tile's pixels erased from the canvas and, on a screen slot, its backup */
static void DrawTileErase(int r, TILEDATA *t, BYTE *bits, WORD slot)
{
    SPRITELIST *list = g_renderers[r].spriteLists[t->list];
    SPRITEREC *rec = &list->frames[t->frame];
    CANVAS *src = &CANVAS_OF(r, list->slot);
    CANVAS *dst;
    int x, y, w, h, skipX, skipY;
    int which;
    BYTE *srcBits;
    CANVAS *b;

    dst = &CANVAS_OF(r, slot);
    srcBits = src->bits;
    skipX = 0;
    skipY = 0;
    x = t->x;
    y = t->y;
    w = rec->width;
    h = rec->height;
    ClipRect(&x, &y, &w, &h, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight, dst->clipBottom);
    if (w > 0 && h > 0) {
        skipX += rec->x;
        skipY += rec->y;
        g_eraseTile(x, y, w, h, dst->width, dst->height, bits, skipX, skipY, src->width, src->height, srcBits);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
    if (SLOT_OF(r, slot).flags & SLOT_SCREEN) {
        b = &g_canvases[SLOT_OF(r, slot).backup];
        g_eraseTileBackup(t->x, t->y, rec->width, rec->height, b->width, b->height, b->bits, rec->x, rec->y,
            src->width, src->height, srcBits);
    }
}

/* 0x10001fd0 */
static void DrawTileEntry(int r, TILEDATA *t, BYTE *bits, WORD slot)
{
    switch (t->mode) {
    case 1:
        DrawTile(r, t, bits, slot);
        break;
    case 2:
        DrawTileErase(r, t, bits, slot);
        break;
    case 3:
        DrawTileBoth(r, t, bits, slot);
        break;
    case 4:
        DrawTileUnder(r, t, bits, slot);
        break;
    case 5:
        DrawTileMasked(r, t, bits, slot);
        break;
    }
}

/* 0x10002080: an RLE sprite's frame, its size and offset */
static WORD *GetRLEFrame(RLEDATA *d, int *w, int *h, int *ox, int *oy)
{
    WORD *spr = (WORD *)(d->sprites + *(DWORD *)(d->sprites + 6 + d->frame * 4));

    *ox = spr[0];
    *oy = spr[1];
    *w = spr[2];
    *h = spr[3];
    return spr;
}

/* 0x100020d0 */
static void DrawRLE(int r, RLEDATA *d, BYTE *bits, WORD slot)
{
    CANVAS *dst;
    int w, h, ox, oy, x, y, skipX, skipY;
    int clipped;
    int which;
    WORD *spr;

    dst = &CANVAS_OF(r, slot);
    w = 0;
    h = 0;
    ox = 0;
    oy = 0;
    spr = GetRLEFrame(d, &w, &h, &ox, &oy);
    skipX = 0;
    skipY = 0;
    x = d->x + ox;
    y = d->y + oy;
    clipped = ClipRect(&x, &y, &w, &h, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight,
                       dst->clipBottom);
    if (w > 0 && h > 0) {
        g_blitRLE(x, y, w, h, clipped, dst->width, dst->height, bits, skipX, skipY, spr);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
}

/* 0x10002280 */
static void DrawRLEMasked(int r, RLEDATA *d, BYTE *bits, WORD slot)
{
    CANVAS *dst;
    BYTE *mask;
    int w, h, ox, oy, x, y, skipX, skipY;
    int clipped;
    int which;
    WORD *spr;

    dst = &CANVAS_OF(r, slot);
    if (SLOT_OF(r, slot).backup != 0)
        mask = g_canvases[SLOT_OF(r, slot).backup].bits;
    else
        mask = bits;
    w = 0;
    h = 0;
    ox = 0;
    oy = 0;
    spr = GetRLEFrame(d, &w, &h, &ox, &oy);
    skipX = 0;
    skipY = 0;
    x = d->x + ox;
    y = d->y + oy;
    clipped = ClipRect(&x, &y, &w, &h, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight,
                       dst->clipBottom);
    if (w > 0 && h > 0) {
        g_maskRLE(x, y, w, h, clipped, dst->width, dst->height, bits, skipX, skipY, spr, mask);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawrleunder0, pad_drawrleunder1, pad_drawrleunder2;

/* 0x10002460 */
static void DrawRLEUnder(int r, RLEDATA *d, BYTE *bits, WORD slot)
{
    CANVAS *dst;
    int w, h, ox, oy, x, y, skipX, skipY;
    int clipped;
    int which;
    WORD *spr;

    dst = &CANVAS_OF(r, slot);
    w = 0;
    h = 0;
    ox = 0;
    oy = 0;
    spr = GetRLEFrame(d, &w, &h, &ox, &oy);
    skipX = 0;
    skipY = 0;
    x = d->x + ox;
    y = d->y + oy;
    clipped = ClipRect(&x, &y, &w, &h, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight,
                       dst->clipBottom);
    if (w > 0 && h > 0) {
        g_underRLE(x, y, w, h, clipped, dst->width, dst->height, bits, skipX, skipY, spr);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawrleboth0, pad_drawrleboth1, pad_drawrleboth2, pad_drawrleboth3, pad_drawrleboth4, pad_drawrleboth5, pad_drawrleboth6, pad_drawrleboth7, pad_drawrleboth8, pad_drawrleboth9, pad_drawrleboth10;

/* 0x10002610 */
static void DrawRLEBoth(int r, RLEDATA *d, BYTE *bits, WORD slot)
{
    CANVAS *dst;
    int w, h, ox, oy, x, y, skipX, skipY, cw, ch;
    int clipped;
    int which;
    WORD *spr;

    dst = &CANVAS_OF(r, slot);
    w = 0;
    h = 0;
    ox = 0;
    oy = 0;
    spr = GetRLEFrame(d, &w, &h, &ox, &oy);
    skipX = 0;
    skipY = 0;
    x = d->x + ox;
    y = d->y + oy;
    cw = w;
    ch = h;
    clipped = ClipRect(&x, &y, &cw, &ch, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight,
                       dst->clipBottom);
    if (cw > 0 && ch > 0)
        g_blitRLE(x, y, cw, ch, clipped, dst->width, dst->height, bits, skipX, skipY, spr);
    if (SLOT_OF(r, slot).flags & SLOT_SCREEN) {
        CANVAS *b = &g_canvases[SLOT_OF(r, slot).backup];
        BYTE *backup = b->bits;

        x = d->x + ox;
        y = d->y + oy;
        g_blitRLEBackup(x, y, cw, ch, 0, b->width, b->height, backup, 0, 0, spr);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
}

/* 0x10002850 */
static void DrawRLEErase(int r, RLEDATA *d, BYTE *bits, WORD slot)
{
    CANVAS *dst;
    int w, h, ox, oy, x, y, skipX, skipY, cw, ch;
    int clipped;
    int which;
    WORD *spr;

    dst = &CANVAS_OF(r, slot);
    w = 0;
    h = 0;
    ox = 0;
    oy = 0;
    spr = GetRLEFrame(d, &w, &h, &ox, &oy);
    skipX = 0;
    skipY = 0;
    x = d->x + ox;
    y = d->y + oy;
    cw = w;
    ch = h;
    clipped = ClipRect(&x, &y, &cw, &ch, &skipX, &skipY, dst->clipLeft, dst->clipTop, dst->clipRight,
                       dst->clipBottom);
    if (cw > 0 && ch > 0)
        g_eraseRLE(x, y, cw, ch, clipped, dst->width, dst->height, bits, skipX, skipY, spr);
    if (SLOT_OF(r, slot).flags & SLOT_SCREEN) {
        CANVAS *b = &g_canvases[SLOT_OF(r, slot).backup];
        BYTE *backup = b->bits;

        x = d->x + ox;
        y = d->y + oy;
        g_eraseRLEBackup(x, y, w, h, 0, b->width, b->height, backup, 0, 0, spr);
        which = 2;
        if (SLOT_OF(r, slot).flags & SLOT_SCREEN)
            which = 3;
        MarkChanged(r, slot, x, y, x + w, y + h, which);
    }
}

/* 0x10002a90 */
static void DrawRLEEntry(int r, RLEDATA *d, BYTE *bits, WORD slot)
{
    switch (d->mode) {
    case 1:
        DrawRLE(r, d, bits, slot);
        break;
    case 2:
        DrawRLEErase(r, d, bits, slot);
        break;
    case 3:
        DrawRLEBoth(r, d, bits, slot);
        break;
    case 4:
        DrawRLEUnder(r, d, bits, slot);
        break;
    case 5:
        DrawRLEMasked(r, d, bits, slot);
        break;
    }
}

/* 0x10002b40 */
static int Random(int n)
{
    return rand() % n;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawpixels0, pad_drawpixels1, pad_drawpixels2, pad_drawpixels3, pad_drawpixels4, pad_drawpixels5, pad_drawpixels6;

/* 0x10002b50: pixels of random colours, at x + dx, y + dy for each point */
static void DrawPixels(int r, PIXELDATA *p, BYTE *bits, WORD slot)
{
    SLOT *s;
    short h;
    short y;
    short w;
    short *pt;
    int which;
    short x0;
    short y0;
    WORD n;
    WORD i;
    short x;

    s = &g_renderers[r].slots[slot];
    which = 2;
    if (s->flags & SLOT_SCREEN)
        which = 3;
    w = s->width;
    h = s->height;
    x0 = p->x;
    y0 = p->y;
    n = p->count;
    pt = p->points;
    for (i = 0; i < n; i++, pt += 2) {     /* VC makes i the original's down-counter (dec si; jne) */
        if (pt[0] != -128 && pt[1] != -128) {
            x = x0 + pt[0];
            y = pt[1] + y0;
            if (x > 0 && y > 0 && x < w && y < h) {
                g_putPixel(bits, x, y, Random(256), w, h);
                MarkPoint(r, slot, x, y, which);
            }
        }
    }
}

/* 0x10002c70 */
WORD WINAPI Renderer_RenderList(WORD renderer, DISPLAYLIST *list)
{
    int r;
    DLENTRY *e;
    BYTE *bits;
    WORD i;

    if (renderer == 0xffff)
        return 0;
    r = (WORD)(renderer ^ HANDLE_XOR);
    bits = g_canvases[g_renderers[r].slots[list->entries[0].slot].canvas].bits;
    e = list->entries;
    for (i = 0; i < list->count; i++, e++) {
        switch (e->type) {
        case DL_TILE:
            DrawTileEntry(r, &e->u.tile, bits, e->slot);
            break;
        case DL_PIXELS:
            DrawPixels(r, &e->u.pixels, bits, e->slot);
            break;
        case DL_RLE:
            DrawRLEEntry(r, &e->u.rle, bits, e->slot);
            break;
        }
    }
    return 0;
}

/* 0x10002d50: keeps a copy of a sprite list; returns its index (0: none) */
WORD WINAPI Renderer_SpriteList(WORD renderer, SPRITELIST *list)
{
    RENDERER *rd;
    WORD index;
    SPRITELIST *copy;

    if (renderer == 0xffff)
        return 0xffff;
    index = 0;
    rd = &g_renderers[(WORD)(renderer ^ HANDLE_XOR)];
    if (rd->numSpriteLists < MAX_SPRITELISTS) {
        index = rd->numSpriteLists++;
        copy = NewSpriteList(list->count);
        rd->spriteLists[index] = copy;
        memcpy(copy, list, list->count * sizeof(SPRITEREC) + 6);
    }
    return index;
}

/* 0x10002de0 */
DISPLAYLIST *WINAPI NewDisplayList(int count)
{
    DWORD size;

    size = count * sizeof(DLENTRY) + 0x3ac;
    return (DISPLAYLIST *)RenderAlloc(size, "NewDisplayList");
}

/* 0x10002e00 */
SPRITELIST *WINAPI NewSpriteList(int count)
{
    DWORD size;

    size = count * sizeof(SPRITEREC) + 6;
    return (SPRITELIST *)RenderAlloc(size, "NewSpriteList");
}

/* 0x10002e20 */
void WINAPI FreeDisplayList(DISPLAYLIST *list)
{
    RenderFree(list);
}

/* 0x10002e30 */
void WINAPI FreeSpriteList(SPRITELIST *list)
{
    RenderFree(list);
}

/* 0x10002e40 */
static int NewRenderer(WORD param)
{
    int i;

    for (i = 0; i < MAX_RENDERERS; i++) {
        if (g_renderers[i].free) {
            g_numRenderers++;
            g_renderers[i].free = 0;
            g_renderers[i].param = param;
            g_renderers[i].handle = (WORD)i ^ HANDLE_XOR;
            g_renderers[i].numSpriteLists = 1;
            g_renderers[i].numSlots = 1;
            return i ^ HANDLE_XOR;
        }
    }
    return -1;
}

/* 0x10002ec0 */
WORD WINAPI Renderer_Initialise(WORD param)
{
    int i;
    int h;

    if (g_numRenderers >= MAX_RENDERERS)
        return 0;
    if (g_numRenderers == 0) {
        for (i = 0; i < MAX_RENDERERS; i++) {
            memset(&g_renderers[i], 0, sizeof g_renderers[i]);
            g_renderers[i].free = 1;
        }
    }
    h = NewRenderer(param);
    SetBlitters();
    return h;
}

/* 0x10002f20 */
static void FreeCanvas(CANVAS *c)
{
    if (c->bitmap != NULL)
        DeleteObject(c->bitmap);
    else if (c->bits != NULL)
        RenderFree(c->bits);
    g_numCanvases--;
    memset(c, 0, sizeof *c);
    c->free = 1;
}

/* 0x10002f70 */
static void FreeRenderer(int r)
{
    WORD i, j;
    int canvas, canvas3, backup;
    HDC dc;
    SLOT *s;

    for (i = 1; i < g_renderers[r].numSlots; i++) {
        s = &g_renderers[r].slots[i];
        canvas = s->canvas;
        dc = s->dc;
        canvas3 = s->canvas3;
        backup = s->backup;
        if (s->flags & SLOT_SCREEN)
            DeleteDC(dc);
        if (canvas)
            FreeCanvas(&g_canvases[canvas]);
        if (canvas3)
            FreeCanvas(&g_canvases[canvas3]);
        if (backup)
            FreeCanvas(&g_canvases[backup]);
        if (s->pending)
            RenderFree(s->pending);
        if (s->changed)
            RenderFree(s->changed);
        memset(s, 0, sizeof *s);
    }
    for (j = 1; j < g_renderers[r].numSpriteLists; j++)
        RenderFree(g_renderers[r].spriteLists[j]);
    memset(&g_renderers[r], 0, sizeof g_renderers[r]);
    g_renderers[r].free = 1;
    g_numRenderers--;
}

/* 0x10003120 */
void WINAPI Renderer_Quit(WORD renderer)
{
    if (renderer != 0xffff)
        FreeRenderer((WORD)(renderer ^ HANDLE_XOR));
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_newcanvas0, pad_newcanvas1, pad_newcanvas2, pad_newcanvas3, pad_newcanvas4, pad_newcanvas5, pad_newcanvas6, pad_newcanvas7, pad_newcanvas8, pad_newcanvas9, pad_newcanvas10, pad_newcanvas11;

/* 0x10003140: a free canvas (0: none) */
static WORD NewCanvas(int owner)
{
    WORD n = g_numCanvases;
    WORD i = 0;
    CANVAS *c;
    int j;

    if (n == 0) {
        memset(g_canvases, 0, sizeof g_canvases);
        for (c = g_canvases; c < g_canvases + MAX_CANVASES; c++)
            c->free = 1;
    }
    if (n < MAX_CANVASES) {
        for (j = 1; j < MAX_CANVASES; j++)
            if (g_canvases[j].free) {
                i = j;
                g_numCanvases = n + 1;
                g_canvases[i].free = 0;
                g_canvases[i].owner = owner;
                g_canvases[i].bitmap = NULL;
                g_canvases[i].bits = NULL;
                g_canvases[i].width = 0;
                g_canvases[i].height = 0;
                memset(&g_canvases[i].header, 0, sizeof g_canvases[i].header + sizeof g_canvases[i].colors);
                break;
            }
    }
    return i;
}

/* 0x10003200 */
static SLOT *NewSlot(WORD renderer, WORD flags, WORD *index)
{
    int r = (WORD)(renderer ^ HANDLE_XOR);
    int i = 0;
    SLOT *s = NULL;
    RENDERER *rd;

    if (g_renderers[r].numSlots < MAX_SLOTS) {
        rd = &g_renderers[r];
        i = rd->numSlots++;
        s = &rd->slots[i];
        s->flags = flags;
        s->canvas = NewCanvas(r);
        s->backup = 0;
        s->canvas3 = 0;
        if (flags & SLOT_SCREEN)
            s->backup = NewCanvas(r);
    }
    *index = i;
    return s;
}

/* 0x10003290: makes the canvas a DIB section with the DIB's pixels (and these colours) */
static void MakeDIBSection(WORD canvas, PDIB dib, HDC *pdc, RGBQUAD *colors)
{
    struct {
        BITMAPINFOHEADER header;
        RGBQUAD colors[256];
    } bi;
    HDC dc;
    HGDIOBJ old;

    g_canvases[canvas].width = GetDibWidth(dib);
    g_canvases[canvas].height = GetDibHeight(dib);
    g_canvases[canvas].clipLeft = 0;
    g_canvases[canvas].clipTop = 0;
    g_canvases[canvas].clipRight = g_canvases[canvas].width + g_canvases[canvas].clipLeft;
    g_canvases[canvas].clipBottom = g_canvases[canvas].clipTop + g_canvases[canvas].height;
    {
        BITMAPINFOHEADER *h = &g_canvases[canvas].header;
        h->biSize = sizeof(BITMAPINFOHEADER);
        h->biHeight = 1;
        h->biPlanes = 1;
        h->biBitCount = 8;
        h->biCompression = 0;
        h->biSizeImage = 0;
        h->biXPelsPerMeter = 0;
        h->biYPelsPerMeter = 0;
        h->biClrUsed = 0;
        h->biClrImportant = 0;
    }
    if (colors == NULL)
        colors = ((BITMAPINFO *)dib)->bmiColors;
    memcpy(g_canvases[canvas].colors, colors, sizeof g_canvases[canvas].colors);
    g_canvases[canvas].header.biHeight = g_canvases[canvas].height;
    g_canvases[canvas].header.biWidth = g_canvases[canvas].width;
    dc = CreateCompatibleDC(NULL);
    g_canvases[canvas].bitmap = CreateDIBSection(dc, (BITMAPINFO *)&g_canvases[canvas].header, DIB_RGB_COLORS, (void **)&g_canvases[canvas].bits, NULL, 0);
    if (pdc != NULL)
        *pdc = dc;
    old = SelectObject(dc, g_canvases[canvas].bitmap);
    memcpy(&bi, &g_canvases[canvas].header, sizeof bi);
    bi.header.biHeight = GetDibHeight(dib);
    StretchDIBits(dc, 0, 0, GetDibWidth(dib), GetDibHeight(dib), 0, 0, GetDibWidth(dib), GetDibHeight(dib),
                  GetDibBitsAddr(dib), (BITMAPINFO *)&bi, DIB_RGB_COLORS, SRCCOPY);
    SelectObject(dc, old);
    if (pdc == NULL)
        DeleteDC(dc);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_copydib0, pad_copydib1, pad_copydib2, pad_copydib3, pad_copydib4, pad_copydib5, pad_copydib6, pad_copydib7;

/* 0x10003490: the canvas gets a copy of the DIB's pixels (no DIB section) */
static void CopyDib(WORD canvas, PDIB dib)
{
    DWORD size;

    if (GetDibBits(dib) == 8 && GetDibPlanes(dib) == 1) {
        g_canvases[canvas].width = GetDibWidth(dib);
        g_canvases[canvas].height = GetDibHeight(dib);
        g_canvases[canvas].clipLeft = 0;
        g_canvases[canvas].clipTop = 0;
        g_canvases[canvas].clipRight = g_canvases[canvas].width + g_canvases[canvas].clipLeft;
        g_canvases[canvas].clipBottom = g_canvases[canvas].height + g_canvases[canvas].clipTop;
        size = g_canvases[canvas].width * g_canvases[canvas].height;
        g_canvases[canvas].bits = (BYTE *)RenderAlloc(size, "CanvasCopy");
        if (g_canvases[canvas].bits == NULL) {
            MessageBoxA(NULL, "Unable to copy DIB no memory", "ERROR : RENDERER", MB_OK);
            return;
        }
        memcpy(g_canvases[canvas].bits, GetDibBitsAddr(dib), size);
    } else
        MessageBoxA(NULL, "DIB not 8 bits 1 plane", "ERROR", MB_OK);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_renderer_loadcanvas_bitmap0, pad_renderer_loadcanvas_bitmap1, pad_renderer_loadcanvas_bitmap2, pad_renderer_loadcanvas_bitmap3, pad_renderer_loadcanvas_bitmap4, pad_renderer_loadcanvas_bitmap5, pad_renderer_loadcanvas_bitmap6, pad_renderer_loadcanvas_bitmap7, pad_renderer_loadcanvas_bitmap8, pad_renderer_loadcanvas_bitmap9, pad_renderer_loadcanvas_bitmap10, pad_renderer_loadcanvas_bitmap11, pad_renderer_loadcanvas_bitmap12, pad_renderer_loadcanvas_bitmap13, pad_renderer_loadcanvas_bitmap14, pad_renderer_loadcanvas_bitmap15, pad_renderer_loadcanvas_bitmap16, pad_renderer_loadcanvas_bitmap17, pad_renderer_loadcanvas_bitmap18, pad_renderer_loadcanvas_bitmap19, pad_renderer_loadcanvas_bitmap20, pad_renderer_loadcanvas_bitmap21, pad_renderer_loadcanvas_bitmap22, pad_renderer_loadcanvas_bitmap23, pad_renderer_loadcanvas_bitmap24, pad_renderer_loadcanvas_bitmap25, pad_renderer_loadcanvas_bitmap26, pad_renderer_loadcanvas_bitmap27, pad_renderer_loadcanvas_bitmap28, pad_renderer_loadcanvas_bitmap29, pad_renderer_loadcanvas_bitmap30, pad_renderer_loadcanvas_bitmap31, pad_renderer_loadcanvas_bitmap32, pad_renderer_loadcanvas_bitmap33, pad_renderer_loadcanvas_bitmap34, pad_renderer_loadcanvas_bitmap35, pad_renderer_loadcanvas_bitmap36, pad_renderer_loadcanvas_bitmap37, pad_renderer_loadcanvas_bitmap38, pad_renderer_loadcanvas_bitmap39, pad_renderer_loadcanvas_bitmap40, pad_renderer_loadcanvas_bitmap41, pad_renderer_loadcanvas_bitmap42, pad_renderer_loadcanvas_bitmap43, pad_renderer_loadcanvas_bitmap44, pad_renderer_loadcanvas_bitmap45, pad_renderer_loadcanvas_bitmap46, pad_renderer_loadcanvas_bitmap47, pad_renderer_loadcanvas_bitmap48, pad_renderer_loadcanvas_bitmap49, pad_renderer_loadcanvas_bitmap50, pad_renderer_loadcanvas_bitmap51, pad_renderer_loadcanvas_bitmap52;

/* 0x100035a0: a new slot holding the DIB; returns its index */
WORD WINAPI Renderer_LoadCanvas_BitMap(WORD renderer, PDIB dib, WORD flags, RGBQUAD *colors)
{
    WORD width, height;
    WORD index;
    SLOT *s;
    DWORD cells;
    BYTE *changed;

    if (renderer == 0xffff)
        return 0xffff;
    width = GetDibWidth(dib);
    height = GetDibHeight(dib);
    s = NewSlot(renderer, flags, &index);
    if (s != NULL) {
        s->flags = flags;
        s->width = width;
        s->height = height;
        s->pending = NULL;
        s->dc = NULL;
        cells = ((width + 15) >> 4) * ((height + 7) >> 3);
        changed = s->changed = (BYTE *)RenderAlloc(cells, "ChangedArea");
        if (changed != NULL)
            memset(changed, 1, cells);
        if (flags & SLOT_SCREEN) {
            MakeDIBSection(s->canvas, dib, &s->dc, colors);
            CopyDib(s->backup, dib);
            s->pending = (BYTE *)RenderAlloc(cells, "HitArea");
        } else
            CopyDib(s->canvas, dib);
    }
    return index;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_markchanged0, pad_markchanged1, pad_markchanged2, pad_markchanged3, pad_markchanged4, pad_markchanged5, pad_markchanged6, pad_markchanged7, pad_markchanged8, pad_markchanged9, pad_markchanged10, pad_markchanged11, pad_markchanged12, pad_markchanged13, pad_markchanged14, pad_markchanged15, pad_markchanged16, pad_markchanged17, pad_markchanged18, pad_markchanged19, pad_markchanged20, pad_markchanged21, pad_markchanged22, pad_markchanged23, pad_markchanged24, pad_markchanged25;

/* 0x100036c0: marks the cells under a rectangle: in pending (which & 1), changed (which & 2) */
static void MarkChanged(int r, WORD slot, int left, int top, int right, int bottom, int which)
{
    int across;
    int x1;
    int lastX;
    int x0;
    int lastY;
    int y0;
    int y1;
    SLOT *s;
    BYTE *changed;
    int offset;
    BYTE *pending;

    s = &g_renderers[r].slots[slot];
    across = (SLOT_OF(r, slot).width + 15) >> 4;
    lastX = across - 1;
    lastY = ((SLOT_OF(r, slot).height + 7) >> 3) - 1;
    x0 = left >> 4;
    y0 = top >> 3;
    x1 = right >> 4;
    y1 = bottom >> 3;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x0 > lastX || y0 > lastY || x1 < 0 || y1 < 0)
        return;
    if (x1 > lastX)
        x1 = lastX;
    if (y1 > lastY)
        y1 = lastY;
    x1 = x1 - x0 + 1;
    y1 = y1 - y0 + 1;
    offset = across * y0 + x0;
    pending = s->pending;
    changed = s->changed;
    for (; y1 > 0; offset += across, y1--) {
        if (which & 1)
            memset(pending + offset, 1, x1);
        if (which & 2)
            memset(changed + offset, 1, x1);
    }
}

/* 0x100037f0 */
static void MarkPoint(int r, WORD slot, int x, int y, int which)
{
    int offset;
    int across;
    BYTE *pending;
    int down;
    int lastX;
    int lastY;
    int cx;
    BYTE *changed;
    SLOT *s;
    int cy;

    s = &g_renderers[r].slots[slot];
    across = (s->width + 15) >> 4;
    lastX = across - 1;
    down = (s->height + 7) >> 3;
    lastY = down - 1;
    cx = x >> 4;
    cy = y >> 3;
    if (cx >= 0 && cy >= 0 && cx <= lastX && lastY >= cy) {
        offset = across * cy + cx;
        pending = s->pending;
        changed = s->changed;
        if (which & 1)
            pending[offset] = 1;
        if (which & 2)
            changed[offset] = 1;
    }
}

/* 0x10003880: what changed this frame becomes what to copy next time */
static void NextFrame(int r, WORD slot)
{
    BYTE *pending = SLOT_OF(r, slot).pending;
    BYTE *changed = SLOT_OF(r, slot).changed;
    WORD down = (SLOT_OF(r, slot).height + 7) >> 3;
    WORD across = (SLOT_OF(r, slot).width + 15) >> 4;
    DWORD cells = down * across;

    if (pending != NULL) {
        if (changed != NULL)
            memcpy(changed, pending, cells);
    } else if (changed != NULL)
        memset(changed, 0, cells);
    if (pending != NULL)
        memset(pending, 0, cells);
}

/* 0x10003930 */
void WINAPI Renderer_Revealed(WORD renderer, WORD slot, int left, int top, int right, int bottom)
{
    WORD r;

    if (renderer != 0xffff) {
        r = (WORD)(renderer ^ HANDLE_XOR);
        MarkChanged(r, slot, left, top, right, bottom, 2);
    }
}

/* 0x10003970 */
HBITMAP WINAPI GetBMP4Canvas(WORD renderer, WORD slot)
{
    return NULL;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_getdib4canvas0;

/* 0x10003980: a copy of part of a slot's canvas, bottom row first */
BYTE *WINAPI GetDIB4Canvas(WORD renderer, WORD slot, int x, int y, int width, int height)
{
    int r;
    CANVAS *c;
    BYTE *buf, *dst, *src;
    DWORD size;

    if (renderer == 0xffff)
        return NULL;
    r = (WORD)(renderer ^ HANDLE_XOR);
    size = width * height;
    c = &g_canvases[g_renderers[r].slots[slot].canvas];
    buf = (BYTE *)RenderAlloc(size, "GetDIB4Canvas");
    dst = buf + (height - 1) * width;
    src = c->bits + (c->height - y - 1) * c->width + x;
    if (buf == NULL) {
        MessageBoxA(NULL, "Unable to allocate memory for DIB", "GetDIB4Canvas", MB_OK);
        return NULL;
    }
    for (; height > 0; height--) {
        memcpy(dst, src, width);
        dst -= width;
        src -= c->width;
    }
    return buf;
}

/* 0x10003a90 */
BYTE *WINAPI GetSurfacePointer(WORD renderer, WORD slot)
{
    return g_canvases[g_renderers[(WORD)(renderer ^ HANDLE_XOR)].slots[slot].canvas].bits;
}

/* 0x10003ae0: the step from one row to the next (rows are stored bottom up) */
int WINAPI GetSurfaceModulo(WORD renderer, WORD slot)
{
    return -(int)g_canvases[g_renderers[(WORD)(renderer ^ HANDLE_XOR)].slots[slot].canvas].width;
}

/* 0x10003b30 */
void WINAPI SetClipRegion(WORD renderer, WORD slot, int x, int y, int width, int height)
{
    CANVAS *c = &g_canvases[g_renderers[(WORD)(renderer ^ HANDLE_XOR)].slots[slot].canvas];

    /* (the original compares the bottom against clipRight too) */
    if (c->clipLeft == x && c->clipTop == y && c->clipRight - width == x && c->clipRight - height == y)
        return;
    c->clipLeft = x;
    c->clipTop = y;
    c->clipRight = x + width;
    c->clipBottom = y + height;
}

/*
 * The blitters. Canvases are stored bottom row first; x, y count from the top.
 * Tiles come from a canvas (srcW x srcH); RLE sprites are runs: 0x80 ends a
 * row, 0x81-0xff that many pixels follow, 0-0x7f that many are skipped. Each
 * also has an unused twin for top-down canvases. The originals' inner loops
 * are inline assembly (and kept as such for Visual C++); other compilers get
 * the same loops in C.
 */

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_blittile0, pad_blittile1;

/* 0x10003bf0: copies a tile's non-zero pixels */
static void BlitTile(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                     int srcW, int srcH, BYTE *src)
{
    BYTE *d, *s;
    int srcStride = srcW;

    y = dstH - y - 1;
    d = dstW * y + dst + x;
    s = (srcH - srcY - 1) * srcW + src + srcX;
#ifdef _MSC_VER
    __asm {
        mov     ebx, srcStride
        mov     esi, s
        mov     edi, d
        mov     ecx, h
    row:
        mov     edx, w
    pixel:
        mov     al, [esi]
        and     al, al
        je      skip
        mov     [edi], al
    skip:
        inc     edi
        inc     esi
        dec     edx
        jne     pixel
        sub     edi, w
        sub     esi, w
        sub     edi, dstW
        sub     esi, ebx
        dec     ecx
        jne     row
    }
#else
    {
        int i, j;
        for (j = h; j; j--, d -= dstW, s -= srcStride)
            for (i = 0; i < w; i++)
                if (s[i])
                    d[i] = s[i];
    }
#endif
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_blittiletopdown0, pad_blittiletopdown1, pad_blittiletopdown2, pad_blittiletopdown3, pad_blittiletopdown4, pad_blittiletopdown5, pad_blittiletopdown6, pad_blittiletopdown7, pad_blittiletopdown8;

/* 0x10003c70: BlitTile for a top-down canvas (unused) */
static void BlitTileTopDown(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                            int srcW, int srcH, BYTE *src)
{
    BYTE *d, *s;
    int srcStride = srcW;

    d = dstW * y + x + dst;
    s = (srcH - srcY - 1) * srcW + src + srcX;
#ifdef _MSC_VER
    __asm {
        mov     ebx, srcStride
        mov     esi, s
        mov     edi, d
        mov     ecx, h
    row:
        mov     edx, w
    pixel:
        mov     al, [esi]
        and     al, al
        je      skip
        mov     [edi], al
    skip:
        inc     edi
        inc     esi
        dec     edx
        jne     pixel
        sub     edi, w
        sub     esi, w
        add     edi, dstW
        sub     esi, ebx
        dec     ecx
        jne     row
    }
#else
    {
        int i, j;
        for (j = h; j; j--, d += dstW, s -= srcStride)
            for (i = 0; i < w; i++)
                if (s[i])
                    d[i] = s[i];
    }
#endif
}

/* 0x10003cf0: BlitTile clipped to the whole canvas */
static void BlitTileClipped(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                            int srcW, int srcH, BYTE *src)
{
    int skipX = 0;
    int skipY = 0;

    ClipRect(&x, &y, &w, &h, &skipX, &skipY, 0, 0, dstW, dstH);
    srcX += skipX;
    srcY += skipY;
    BlitTile(x, y, w, h, dstW, dstH, dst, srcX, srcY, srcW, srcH, src);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_erasetile0, pad_erasetile1, pad_erasetile2, pad_erasetile3, pad_erasetile4, pad_erasetile5;

/* 0x10003d80: clears the canvas under a tile's non-zero pixels */
static void EraseTile(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                      int srcW, int srcH, BYTE *src)
{
    BYTE *d, *s;
    BYTE *sp, *dp;
    int i, j;

    y = dstH - y - 1;
    d = dst + dstW * y + x;
    s = srcX + (srcH - srcY - 1) * srcW + src;
    for (j = 0; j < h; j++) {
        sp = s;
        dp = d;
        for (i = 0; i < w; i++, dp++, sp++)
            if (*sp)
                *dp = 0;
        d -= dstW;
        s -= srcW;
    }
}

/* 0x10003df0 */
static void EraseTileClipped(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                             int srcW, int srcH, BYTE *src)
{
    int skipX = 0;
    int skipY = 0;

    ClipRect(&x, &y, &w, &h, &skipX, &skipY, 0, 0, dstW, dstH);
    srcX += skipX;
    srcY += skipY;
    EraseTile(x, y, w, h, dstW, dstH, dst, srcX, srcY, srcW, srcH, src);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_erasetiletopdown0, pad_erasetiletopdown1, pad_erasetiletopdown2, pad_erasetiletopdown3, pad_erasetiletopdown4, pad_erasetiletopdown5, pad_erasetiletopdown6, pad_erasetiletopdown7, pad_erasetiletopdown8, pad_erasetiletopdown9, pad_erasetiletopdown10, pad_erasetiletopdown11, pad_erasetiletopdown12, pad_erasetiletopdown13, pad_erasetiletopdown14, pad_erasetiletopdown15;

/* 0x10003e80: EraseTile for a top-down canvas (unused) */
static void EraseTileTopDown(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                             int srcW, int srcH, BYTE *src)
{
    BYTE *d, *s;
    int j;

    d = dst + dstW * y + x;
    s = srcX + (srcH - srcY - 1) * srcW + src;
    for (j = 0; j < h; j++) {
        int i;
        BYTE *sp = s, *dp = d;

        for (i = 0; i < w; i++, dp++, sp++)
            if (*sp)
                *dp = 0;
        d += dstW;
        s -= srcW;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_undertile0, pad_undertile1, pad_undertile2;

/* 0x10003ef0: copies a tile's pixels only where the canvas is empty */
static void UnderTile(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                      int srcW, int srcH, BYTE *src)
{
    BYTE *d, *sp, *dp, *s;
    int i, j;

    y = dstH - y - 1;
    d = dst + dstW * y + x;
    s = srcX + (srcH - srcY - 1) * srcW + src;
    for (j = 0; j < h; j++) {
        sp = s;
        dp = d;
        for (i = 0; i < w; i++, dp++, sp++)
            if (*dp == 0 && *sp)
                *dp = *sp;
        d -= dstW;
        s -= srcW;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_undertiletopdown0;

/* 0x10003f60: UnderTile for a top-down canvas (unused) */
static void UnderTileTopDown(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                             int srcW, int srcH, BYTE *src)
{
    BYTE *d, *sp, *dp, *s;
    int i, j;

    d = dst + dstW * y + x;
    s = srcX + (srcH - srcY - 1) * srcW + src;
    for (j = 0; j < h; j++) {
        sp = s;
        dp = d;
        for (i = 0; i < w; i++, dp++, sp++)
            if (*dp == 0 && *sp)
                *dp = *sp;
        d += dstW;
        s -= srcW;
    }
}

/* 0x10003fd0: copies a tile's pixels where the mask (laid out as the canvas) has pixels */
static void MaskTile(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                     int srcW, int srcH, BYTE *src, BYTE *mask)
{
    BYTE *d, *m, *s;
    int srcStride = srcW;

    y = dstH - y - 1;
    d = dst + dstW * y + x;
    m = mask + dstW * y + x;
    s = (srcH - srcY - 1) * srcW + src + srcX;
#ifdef _MSC_VER
    __asm {
        mov     ebx, m
        mov     esi, s
        mov     edi, d
        mov     ecx, h
    row:
        mov     edx, w
    pixel:
        mov     al, [ebx]
        and     al, al
        je      skip
        mov     al, [esi]
        and     al, al
        je      skip
        mov     [edi], al
    skip:
        inc     edi
        inc     esi
        inc     ebx
        dec     edx
        jne     pixel
        sub     edi, w
        sub     esi, w
        sub     ebx, w
        sub     edi, dstW
        sub     ebx, dstW
        sub     esi, srcStride
        dec     ecx
        jne     row
    }
#else
    {
        int i, j;
        for (j = h; j; j--, d -= dstW, m -= dstW, s -= srcStride)
            for (i = 0; i < w; i++)
                if (m[i] && s[i])
                    d[i] = s[i];
    }
#endif
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_masktiletopdown0, pad_masktiletopdown1, pad_masktiletopdown2, pad_masktiletopdown3, pad_masktiletopdown4, pad_masktiletopdown5, pad_masktiletopdown6, pad_masktiletopdown7, pad_masktiletopdown8, pad_masktiletopdown9, pad_masktiletopdown10, pad_masktiletopdown11;

/* 0x10004070: MaskTile for a top-down canvas (unused) */
static void MaskTileTopDown(int x, int y, int w, int h, int dstW, int dstH, BYTE *dst, int srcX, int srcY,
                            int srcW, int srcH, BYTE *src, BYTE *mask)
{
    BYTE *d, *m, *s;
    int srcStride = srcW;

    d = dst + dstW * y + x;
    y = dstH - y - 1;
    m = mask + dstW * y + x;
    s = (srcH - srcY - 1) * srcW + src + srcX;
#ifdef _MSC_VER
    __asm {
        mov     ebx, m
        mov     esi, s
        mov     edi, d
        mov     ecx, h
    row:
        mov     edx, w
    pixel:
        mov     al, [ebx]
        and     al, al
        je      skip
        mov     al, [esi]
        and     al, al
        je      skip
        mov     [edi], al
    skip:
        inc     edi
        inc     esi
        inc     ebx
        dec     edx
        jne     pixel
        sub     edi, w
        sub     esi, w
        sub     ebx, w
        add     edi, dstW
        sub     ebx, dstW
        sub     esi, srcStride
        dec     ecx
        jne     row
    }
#else
    {
        int i, j;
        for (j = h; j; j--, d += dstW, m -= dstW, s -= srcStride)
            for (i = 0; i < w; i++)
                if (m[i] && s[i])
                    d[i] = s[i];
    }
#endif
}

/* 0x10004120: one pixel, top-down canvas (unused) */
void PutPixelTopDown(BYTE *bits, int x, int y, int colour, int w)
{
    x += w * y;
    bits[x] = (BYTE)colour;
}

/* 0x10004140 */
static void PutPixel(BYTE *bits, int x, int y, int colour, int w, int h)
{
    y = h - y - 1;
    x += w * y;
    bits[x] = (BYTE)colour;
}

/* 0x10004160: copies a rectangle, top-down destination (unused) */
void CopyBlockTopDown(BYTE *dst, BYTE *src, int x, int y, int w, int h, int stride, int height,
                            int dstStride)
{
    dst += stride * y + x;
    src += (height - y - 1) * stride + x;
#ifdef _MSC_VER
    __asm {
        mov     esi, src
        mov     edi, dst
        mov     edx, stride
        mov     ecx, h
    row:
        mov     ebx, w
        shr     ebx, 2
        and     ebx, ebx
        je      tail
    dwords:
        mov     eax, [esi]
        mov     [edi], eax
        add     esi, 4
        add     edi, 4
        dec     ebx
        jne     dwords
    tail:
        mov     ebx, w
        and     bl, 3
        je      next
        cmp     bl, 2
        jb      onebyte
        mov     ax, [esi]
        mov     [edi], ax
        add     esi, 2
        add     edi, 2
        sub     bl, 2
        je      next
    onebyte:
        mov     al, [esi]
        mov     [edi], al
        inc     esi
        inc     edi
    next:
        sub     esi, w
        sub     edi, w
        sub     esi, edx
        add     edi, dstStride
        dec     ecx
        jne     row
    }
#else
    for (; h; h--, src -= stride, dst += dstStride)
        memcpy(dst, src, w);
#endif
}

/* 0x100041f0: copies a rectangle between two canvases of the same layout */
static void CopyBlock(BYTE *dst, BYTE *src, int x, int y, int w, int h, int stride, int height, int dstStride)
{
    x += (height - y - 1) * stride;
    dst += x;
    src += x;
#ifdef _MSC_VER
    __asm {
        mov     esi, src
        mov     edi, dst
        mov     edx, stride
        mov     ecx, h
    row:
        mov     ebx, w
        shr     ebx, 2
        and     ebx, ebx
        je      tail
    dwords:
        mov     eax, [esi]
        mov     [edi], eax
        add     esi, 4
        add     edi, 4
        dec     ebx
        jne     dwords
    tail:
        mov     ebx, w
        and     bl, 3
        je      next
        cmp     bl, 2
        jb      onebyte
        mov     ax, [esi]
        mov     [edi], ax
        add     esi, 2
        add     edi, 2
        sub     bl, 2
        je      next
    onebyte:
        mov     al, [esi]
        mov     [edi], al
        inc     esi
        inc     edi
    next:
        sub     esi, w
        sub     edi, w
        sub     esi, edx
        sub     edi, dstStride
        dec     ecx
        jne     row
    }
#else
    for (; h; h--, src -= stride, dst -= dstStride)
        memcpy(dst, src, w);
#endif
}

/* 0x10004280: an RLE sprite unpacked into a new bottom-up buffer */
static BYTE *UnpackRLE(WORD *spr)
{
    DWORD size;
    BYTE *buf;
    BYTE *data;
    int w, h;
    BYTE *row;

    size = spr[2] * spr[3];
    buf = (BYTE *)VSMEM_Allocate(size, "UncompressRLESprite");
    data = (BYTE *)spr + 12;
    memset(buf, 0, size);
    row = (spr[3] - 1) * spr[2] + buf;
    h = spr[3];
    w = spr[2];
#ifdef _MSC_VER
    __asm {
        mov     esi, data
        mov     edi, row
        mov     edx, edi
        mov     ecx, h
    next:
        mov     al, [esi]
        inc     esi
        cmp     al, 80h
        je      eol
        ja      run
        movzx   eax, al
        add     edi, eax
        jmp     next
    run:
        and     al, 7fh
        mov     bl, al
    copy:
        mov     al, [esi]
        mov     [edi], al
        inc     esi
        inc     edi
        dec     bl
        jne     copy
        jmp     next
    eol:
        sub     edx, w
        mov     edi, edx
        dec     ecx
        jne     next
    }
#else
    {
        BYTE *d = row;
        BYTE c;
        while (h) {
            c = *data++;
            if (c == 0x80) {
                row -= w;
                d = row;
                h--;
            } else if (c > 0x80) {
                for (c &= 0x7f; c; c--)
                    *d++ = *data++;
            } else
                d += c;
        }
    }
#endif
    return buf;
}

/* RLE blitters: an RLE sprite (spr) at x, y; a clipped one is unpacked and drawn as a tile */

/* 0x10004330 */
static void BlitRLE(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX,
                    int skipY, WORD *spr)
{
    BYTE *unpacked;
    BYTE *d, *data;

    if (clipped) {
        unpacked = UnpackRLE(spr);
        BlitTile(x, y, w, h, dstW, dstH, dst, skipX, skipY, spr[2], spr[3], unpacked);
        VSMEM_Free(unpacked);
        return;
    }
    y = dstH - y - 1;
    d = dst + dstW * y + x;
    data = (BYTE *)spr + 12;
#ifdef _MSC_VER
    __asm {
        mov     esi, data
        mov     edi, d
        mov     edx, edi
        mov     ecx, h
    next:
        mov     al, [esi]
        inc     esi
        cmp     al, 80h
        je      eol
        ja      run
        movzx   eax, al
        add     edi, eax
        jmp     next
    run:
        and     al, 7fh
        mov     bl, al
    copy:
        mov     al, [esi]
        mov     [edi], al
        inc     esi
        inc     edi
        dec     bl
        jne     copy
        jmp     next
    eol:
        sub     edx, dstW
        mov     edi, edx
        dec     ecx
        jne     next
    }
#else
    {
        BYTE *p = d;
        BYTE c;
        while (h) {
            c = *data++;
            if (c == 0x80) {
                d -= dstW;
                p = d;
                h--;
            } else if (c > 0x80) {
                for (c &= 0x7f; c; c--)
                    *p++ = *data++;
            } else
                p += c;
        }
    }
#endif
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_blitrletopdown0;

/* 0x10004410: BlitRLE for a top-down canvas (unused) */
void BlitRLETopDown(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                           int skipX, int skipY, WORD *spr)
{
    BYTE *unpacked;
    BYTE *d, *data;

    if (clipped) {
        unpacked = UnpackRLE(spr);
        BlitTileTopDown(x, y, w, h, dstW, dstH, dst, skipX, skipY, spr[2], spr[3], unpacked);
        VSMEM_Free(unpacked);
        return;
    }
    d = y * dstW + x + dst;
    data = (BYTE *)spr + 12;
#ifdef _MSC_VER
    __asm {
        mov     esi, data
        mov     edi, d
        mov     edx, edi
        mov     ecx, h
    next:
        mov     al, [esi]
        inc     esi
        cmp     al, 80h
        je      eol
        ja      run
        movzx   eax, al
        add     edi, eax
        jmp     next
    run:
        and     al, 7fh
        mov     bl, al
    copy:
        mov     al, [esi]
        mov     [edi], al
        inc     esi
        inc     edi
        dec     bl
        jne     copy
        jmp     next
    eol:
        add     edx, dstW
        mov     edi, edx
        dec     ecx
        jne     next
    }
#else
    {
        BYTE *p = d;
        BYTE c;
        while (h) {
            c = *data++;
            if (c == 0x80) {
                d += dstW;
                p = d;
                h--;
            } else if (c > 0x80) {
                for (c &= 0x7f; c; c--)
                    *p++ = *data++;
            } else
                p += c;
        }
    }
#endif
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_blitrleclipped0;

/* 0x100044e0: BlitRLE clipped to the whole canvas */
static void BlitRLEClipped(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                           int skipX, int skipY, WORD *spr)
{
    int sx = 0;
    int sy = 0;

    clipped = ClipRect(&x, &y, &w, &h, &sx, &sy, 0, 0, dstW, dstH);
    skipX += sx;
    skipY += sy;
    BlitRLE(x, y, w, h, clipped, dstW, dstH, dst, skipX, skipY, spr);
}

/* 0x10004570 */
static void EraseRLE(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX,
                     int skipY, WORD *spr)
{
    BYTE *unpacked = UnpackRLE(spr);

    EraseTile(x, y, w, h, dstW, dstH, dst, skipX, skipY, spr[2], spr[3], unpacked);
    VSMEM_Free(unpacked);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_eraserleclipped0, pad_eraserleclipped1;

/* 0x100045d0 */
static void EraseRLEClipped(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                            int skipX, int skipY, WORD *spr)
{
    int sx = 0;
    int sy = 0;

    clipped = ClipRect(&x, &y, &w, &h, &sx, &sy, 0, 0, dstW, dstH);
    skipX += sx;
    skipY += sy;
    EraseRLE(x, y, w, h, clipped, dstW, dstH, dst, skipX, skipY, spr);
}

/* 0x10004660: EraseRLE for a top-down canvas (unused) */
void EraseRLETopDown(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                            int skipX, int skipY, WORD *spr)
{
    BYTE *unpacked = UnpackRLE(spr);

    EraseTileTopDown(x, y, w, h, dstW, dstH, dst, skipX, skipY, spr[2], spr[3], unpacked);
    VSMEM_Free(unpacked);
}

/* 0x100046c0 */
static void UnderRLE(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX,
                     int skipY, WORD *spr)
{
    BYTE *unpacked;
    BYTE *d, *data;

    if (clipped) {
        unpacked = UnpackRLE(spr);
        UnderTile(x, y, w, h, dstW, dstH, dst, skipX, skipY, spr[2], spr[3], unpacked);
        VSMEM_Free(unpacked);
        return;
    }
    y = dstH - y - 1;
    d = dst + dstW * y + x;
    data = (BYTE *)spr + 12;
#ifdef _MSC_VER
    __asm {
        mov     esi, data
        mov     edi, d
        mov     edx, edi
        mov     ecx, h
    next:
        mov     al, [esi]
        inc     esi
        cmp     al, 80h
        je      eol
        ja      run
        movzx   eax, al
        add     edi, eax
        jmp     next
    run:
        and     al, 7fh
        mov     bl, al
    copy:
        mov     al, [edi]
        and     al, al
        jne     skip
        mov     al, [esi]
        mov     [edi], al
    skip:
        inc     esi
        inc     edi
        dec     bl
        jne     copy
        jmp     next
    eol:
        sub     edx, dstW
        mov     edi, edx
        dec     ecx
        jne     next
    }
#else
    {
        BYTE *p = d;
        BYTE c;
        while (h) {
            c = *data++;
            if (c == 0x80) {
                d -= dstW;
                p = d;
                h--;
            } else if (c > 0x80) {
                for (c &= 0x7f; c; c--, p++, data++)
                    if (*p == 0)
                        *p = *data;
            } else
                p += c;
        }
    }
#endif
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_underrletopdown0, pad_underrletopdown1, pad_underrletopdown2, pad_underrletopdown3;

/* 0x100047a0: UnderRLE for a top-down canvas (unused) */
void UnderRLETopDown(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                            int skipX, int skipY, WORD *spr)
{
    BYTE *unpacked;
    BYTE *d, *data;

    if (clipped) {
        unpacked = UnpackRLE(spr);
        UnderTileTopDown(x, y, w, h, dstW, dstH, dst, skipX, skipY, spr[2], spr[3], unpacked);
        VSMEM_Free(unpacked);
        return;
    }
    d = y * dstW + x + dst;
    data = (BYTE *)spr + 12;
#ifdef _MSC_VER
    __asm {
        mov     esi, data
        mov     edi, d
        mov     edx, edi
        mov     ecx, h
    next:
        mov     al, [esi]
        inc     esi
        cmp     al, 80h
        je      eol
        ja      run
        movzx   eax, al
        add     edi, eax
        jmp     next
    run:
        and     al, 7fh
        mov     bl, al
    copy:
        mov     al, [edi]
        and     al, al
        jne     skip
        mov     al, [esi]
        mov     [edi], al
    skip:
        inc     esi
        inc     edi
        dec     bl
        jne     copy
        jmp     next
    eol:
        add     edx, dstW
        mov     edi, edx
        dec     ecx
        jne     next
    }
#else
    {
        BYTE *p = d;
        BYTE c;
        while (h) {
            c = *data++;
            if (c == 0x80) {
                d += dstW;
                p = d;
                h--;
            } else if (c > 0x80) {
                for (c &= 0x7f; c; c--, p++, data++)
                    if (*p == 0)
                        *p = *data;
            } else
                p += c;
        }
    }
#endif
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_maskrle0;

/* 0x10004880: an RLE sprite's pixels where the mask (laid out as the canvas) has pixels */
static void MaskRLE(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst, int skipX,
                    int skipY, WORD *spr, BYTE *mask)
{
    BYTE *unpacked;
    BYTE *d, *m, *data;

    if (clipped) {
        unpacked = UnpackRLE(spr);
        MaskTile(x, y, w, h, dstW, dstH, dst, skipX, skipY, spr[2], spr[3], unpacked, mask);
        VSMEM_Free(unpacked);
        return;
    }
    y = dstH - y - 1;
    m = mask + dstW * y + x;
    d = dst + dstW * y + x;
    data = (BYTE *)spr + 12;
#ifdef _MSC_VER
    __asm {
        mov     ebx, d
        mov     esi, data
        mov     edi, m
        mov     edx, edi
        mov     ecx, h
    row:
        push    ebx
    next:
        mov     al, [esi]
        inc     esi
        cmp     al, 80h
        je      eol
        ja      run
        movzx   eax, al
        add     edi, eax
        add     ebx, eax
        jmp     next
    run:
        push    ecx
        and     al, 7fh
        mov     cl, al
    copy:
        mov     al, [edi]
        and     al, al
        je      skip
        mov     al, [esi]
        mov     [ebx], al
    skip:
        inc     esi
        inc     edi
        inc     ebx
        dec     cl
        jne     copy
        pop     ecx
        jmp     next
    eol:
        pop     ebx
        sub     ebx, dstW
        sub     edx, dstW
        mov     edi, edx
        dec     ecx
        jne     row
    }
#else
    {
        BYTE *p = d, *q = m;
        BYTE c;
        while (h) {
            c = *data++;
            if (c == 0x80) {
                d -= dstW;
                m -= dstW;
                p = d;
                q = m;
                h--;
            } else if (c > 0x80) {
                for (c &= 0x7f; c; c--, p++, q++, data++)
                    if (*q)
                        *p = *data;
            } else {
                p += c;
                q += c;
            }
        }
    }
#endif
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_maskrletopdown0, pad_maskrletopdown1, pad_maskrletopdown2, pad_maskrletopdown3, pad_maskrletopdown4, pad_maskrletopdown5, pad_maskrletopdown6, pad_maskrletopdown7, pad_maskrletopdown8, pad_maskrletopdown9, pad_maskrletopdown10;

/* 0x10004970: MaskRLE for a top-down canvas (unused) */
void MaskRLETopDown(int x, int y, int w, int h, int clipped, int dstW, int dstH, BYTE *dst,
                           int skipX, int skipY, WORD *spr, BYTE *mask)
{
    BYTE *unpacked;
    BYTE *d, *m, *data;

    if (clipped) {
        unpacked = UnpackRLE(spr);
        MaskTileTopDown(x, y, w, h, dstW, dstH, dst, skipX, skipY, spr[2], spr[3], unpacked, mask);
        VSMEM_Free(unpacked);
        return;
    }
    d = dst + dstW * y + x;
    y = dstH - y - 1;
    m = mask + dstW * y + x;
    data = (BYTE *)spr + 12;
#ifdef _MSC_VER
    __asm {
        mov     ebx, d
        mov     esi, data
        mov     edi, m
        mov     edx, edi
        mov     ecx, h
    row:
        push    ebx
    next:
        mov     al, [esi]
        inc     esi
        cmp     al, 80h
        je      eol
        ja      run
        movzx   eax, al
        add     edi, eax
        add     ebx, eax
        jmp     next
    run:
        push    ecx
        and     al, 7fh
        mov     cl, al
    copy:
        mov     al, [edi]
        and     al, al
        je      skip
        mov     al, [esi]
        mov     [ebx], al
    skip:
        inc     esi
        inc     edi
        inc     ebx
        dec     cl
        jne     copy
        pop     ecx
        jmp     next
    eol:
        pop     ebx
        sub     edx, dstW
        add     ebx, dstW
        mov     edi, edx
        dec     ecx
        jne     row
    }
#else
    {
        BYTE *p = d, *q = m;
        BYTE c;
        while (h) {
            c = *data++;
            if (c == 0x80) {
                d += dstW;
                m -= dstW;
                p = d;
                q = m;
                h--;
            } else if (c > 0x80) {
                for (c &= 0x7f; c; c--, p++, q++, data++)
                    if (*q)
                        *p = *data;
            } else {
                p += c;
                q += c;
            }
        }
    }
#endif
}

/* 0x10004a60 */
static void SetBlitters(void)
{
    g_blitTile = BlitTile;
    g_eraseTile = EraseTile;
    g_underTile = UnderTile;
    g_maskTile = MaskTile;
    g_blitRLE = BlitRLE;
    g_eraseRLE = EraseRLE;
    g_underRLE = UnderRLE;
    g_maskRLE = MaskRLE;
    g_copyRect = CopyBlock;
    g_putPixel = PutPixel;
    g_blitTileBackup = BlitTileClipped;
    g_eraseTileBackup = EraseTileClipped;
    g_blitRLEBackup = BlitRLEClipped;
    g_eraseRLEBackup = EraseRLEClipped;
}

extern int pad_copychanged0, pad_copychanged1, pad_copychanged2, pad_copychanged3, pad_copychanged4, pad_copychanged5, pad_copychanged6, pad_copychanged7, pad_copychanged8, pad_copychanged9, pad_copychanged10, pad_copychanged11, pad_copychanged12, pad_copychanged13, pad_copychanged14, pad_copychanged15, pad_copychanged16, pad_copychanged17, pad_copychanged18, pad_copychanged19, pad_copychanged20, pad_copychanged21, pad_copychanged22, pad_copychanged23, pad_copychanged24, pad_copychanged25;
/* 0x10004af0: copies the changed cells of the rectangle to the screen, a run of cells at a time */
static void CopyChanged(HDC dst, int dx, int dy, HDC src, int left, int top, int right, int bottom,
                        BYTE *changed, int across, int down, BYTE *bits, BITMAPINFO *bmi)
{
    int n;
    int rows;
    register int y;
    int cx0;
    int cy0;
    int cx1;
    int cy1;
    register int cx;
    int x;

    cx0 = left >> 4;
    cy0 = top >> 3;
    cx1 = (right + 15) >> 4;
    cy1 = (bottom + 7) >> 3;
    changed += across * cy0;
    if (cy1 > cy0) {
        y = 0;
        rows = cy1 - cy0;
        do {
            cx = cx0;
            while (cx < cx1) {
                if (changed[cx]) {
                    n = 16;
                    x = (cx - cx0) << 4;
                    for (cx++; cx < cx1 && changed[cx]; cx++)
                        n += 16;
                    BitBlt(dst, x + dx, dy + y, n, 8, src, left + x, top + y, SRCCOPY);
                } else
                    for (cx++; cx < cx1 && !changed[cx]; cx++)
                        ;
            }
            changed += across;
            y += 8;
        } while (--rows);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_stretchchanged0, pad_stretchchanged1, pad_stretchchanged2, pad_stretchchanged3, pad_stretchchanged4, pad_stretchchanged5, pad_stretchchanged6, pad_stretchchanged7, pad_stretchchanged8, pad_stretchchanged9, pad_stretchchanged10, pad_stretchchanged11, pad_stretchchanged12, pad_stretchchanged13, pad_stretchchanged14, pad_stretchchanged15, pad_stretchchanged16, pad_stretchchanged17, pad_stretchchanged18, pad_stretchchanged19, pad_stretchchanged20, pad_stretchchanged21, pad_stretchchanged22, pad_stretchchanged23, pad_stretchchanged24, pad_stretchchanged25, pad_stretchchanged26, pad_stretchchanged27, pad_stretchchanged28, pad_stretchchanged29, pad_stretchchanged30, pad_stretchchanged31, pad_stretchchanged32, pad_stretchchanged33, pad_stretchchanged34, pad_stretchchanged35, pad_stretchchanged36, pad_stretchchanged37, pad_stretchchanged38, pad_stretchchanged39, pad_stretchchanged40, pad_stretchchanged41, pad_stretchchanged42, pad_stretchchanged43, pad_stretchchanged44, pad_stretchchanged45, pad_stretchchanged46, pad_stretchchanged47, pad_stretchchanged48, pad_stretchchanged49, pad_stretchchanged50, pad_stretchchanged51, pad_stretchchanged52, pad_stretchchanged53, pad_stretchchanged54, pad_stretchchanged55, pad_stretchchanged56, pad_stretchchanged57, pad_stretchchanged58, pad_stretchchanged59, pad_stretchchanged60, pad_stretchchanged61, pad_stretchchanged62, pad_stretchchanged63, pad_stretchchanged64, pad_stretchchanged65, pad_stretchchanged66, pad_stretchchanged67, pad_stretchchanged68;

/* 0x10004bd0: CopyChanged, stretched to width x height */
static void StretchChanged(HDC dst, int dx, int dy, HDC src, int left, int top, int right, int bottom,
                           BYTE *changed, int across, int down, int width, int height, BYTE *bits,
                           BITMAPINFO *bmi)
{
    BYTE *row;
    int sx;
    int y;
    int w;
    int x;
    int h;
    volatile int sy;    /* volatile: the original never hoists sy * 8 or strength-reduces (dy + y) * sy */
    int cx0;
    int cx1;
    int n;
    int rows;
    int cy0;
    int cy1;
    int cx;

    cx0 = left >> 4;
    cy0 = top >> 3;
    cx1 = (right + 15) >> 4;
    cy1 = (bottom + 7) >> 3;
    w = right - left;
    h = bottom - top;
    sx = (w / 2 + width) / w;
    sy = (h / 2 + height) / h;
#ifdef _MSC_VER
    switch (cy0 * across) { case 1: break; }
#endif
    row = changed + cy0 * across;
    if (cy1 > cy0) {
        y = 0;
        rows = cy1 - cy0;
        do {
            cx = cx0;
            while (cx < cx1) {
                if (row[cx]) {
                    n = 16;
                    x = (cx - cx0) << 4;
                    for (cx++; cx < cx1 && row[cx]; cx++)
                        n += 16;
                    StretchBlt(dst, (x + dx) * sx, (dy + y) * sy, n * sx, sy * 8, src, left + x, top + y, n, 8,
                               SRCCOPY);
                } else
                    for (cx++; cx < cx1 && !row[cx]; cx++)
                        ;
            }
            row += across;
            y += 8;
        } while (--rows);
    }
}

/* 0x10004d10 */
static void StretchAll(HDC dst, HDC src, int left, int top, int right, int bottom, BYTE *changed,
                       int across, int down, int width, int height)
{
    StretchBlt(dst, 0, 0, width, height, src, left, top, right - left, bottom - top, SRCCOPY);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_restorechanged0, pad_restorechanged1, pad_restorechanged2, pad_restorechanged3, pad_restorechanged4, pad_restorechanged5, pad_restorechanged6, pad_restorechanged7, pad_restorechanged8, pad_restorechanged9, pad_restorechanged10, pad_restorechanged11, pad_restorechanged12, pad_restorechanged13;

/* 0x10004d50: puts the background back where this frame's sprites were drawn */
static void RestoreChanged(int r, WORD slot)
{
    SLOT *s = &g_renderers[r].slots[slot];
    int w = s->width;
    int h = s->height;
    int stride = g_canvases[s->canvas].width;
    WORD cx;
    WORD across = (w + 15) >> 4;
    BYTE *canvasBits = g_canvases[s->canvas].bits;
    WORD down = (h + 7) >> 3;
    /* Keep this copy after down for the original VC 2.2 register allocation. */
    BYTE *bits = canvasBits;
    BYTE *backup = g_canvases[s->backup].bits;
    WORD y;
    BYTE *row = s->pending;
    int n;
    int x;
    int py;

    for (y = 0; y < down; y++, row += across) {
        for (cx = 0; cx < across;) {
            if (row[cx]) {
                n = 16;
                x = cx << 4;
                py = y * 8;
                for (cx++; cx < across && row[cx]; cx++)
                    n += 16;
                g_copyRect(bits, backup, x, py, n, 8, w, h, stride);
            } else
                for (cx++; cx < across && !row[cx]; cx++)
                    ;
        }
    }
}

/* 0x10004ec0: zeroed memory */
static void *RenderAlloc(DWORD size, const char *tag)
{
    void *p = VSMEM_Allocate(size, tag);

    if (p == NULL) {
        MessageBoxA(NULL, "Memory Allocation error <1>", tag, MB_OK);
        return p;
    }
    memset(p, 0, size);
    return p;
}

/* 0x10004f10 */
static void RenderFree(void *p)
{
    VSMEM_Free(p);
}

/* 0x10004f20: the shared assertion handler (unreferenced here) */
void VS_AssertFailed(const char *expr, const char *file, unsigned line)
{
    char msg[256];

    wsprintfA(msg, "%s\nin file \"%s\" at line %u\n", expr, file, line);
    MessageBoxA(NULL, msg, "Visual Sciences Assertion Failure", MB_ICONEXCLAMATION);
    ExitProcess(0xaaaa);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_dllentrypoint0, pad_dllentrypoint1;

/* 0x10004f70: unused, as in VSMEM.DLL (the runtime's entry point is the DLL's) */
BOOL WINAPI DllEntryPoint(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    if (reason == DLL_PROCESS_ATTACH)
        RENDER_LibMain(hinst);
    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    return TRUE;
}
