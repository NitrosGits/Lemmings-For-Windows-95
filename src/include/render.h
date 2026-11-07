/*
 * RENDER.DLL: an off-screen renderer. A renderer (handle = index ^ 0x55aa)
 * owns up to 50 slots; a slot is a canvas (an 8-bit DIB section) with
 * optional back-up copy, plus grids of 16x8-pixel cells that record which
 * parts changed and need copying to the screen. Display lists draw sprites
 * (from sprite lists: rectangles of a sprite-sheet slot), RLE sprites and
 * single pixels into a slot.
 */
#ifndef RENDER_H
#define RENDER_H

#include <windows.h>
#include "readdib.h"

#if defined(RENDER_BUILD) && defined(_MSC_VER)
#define RENDERAPI __declspec(dllexport)
#else
#define RENDERAPI
#endif

/* Slot flags */
#define SLOT_SCREEN     2       /* shown on screen: has a DC and a back-up copy */

/* A rectangle of a sprite sheet */
typedef struct SPRITEREC {
    WORD x, y, width, height;
} SPRITEREC;

/* A sprite list: frames in the sprite sheet held in a slot */
typedef struct SPRITELIST {
    WORD count;
    WORD slot;
    WORD unused;
    SPRITEREC frames[1];
} SPRITELIST;

/* Display list entries (24 bytes): a type, the slot drawn into, its data */
#define DL_TILE     1       /* a frame of a sprite list */
#define DL_PIXELS   2       /* points of random colour */
#define DL_RLE      3       /* a frame of an RLE sprite file */

typedef struct TILEDATA {      /* DL_TILE */
    WORD mode;                  /* 1-5: how it is drawn */
    WORD frame;
    WORD list;                  /* sprite list */
    short x, y;
} TILEDATA;

typedef struct PIXELDATA {     /* DL_PIXELS */
    short x, y;
    WORD count;
    WORD unused;
    short *points;              /* x, y pairs; -128 skips */
    WORD colour, colours;       /* not used: the colours are random */
    WORD size;
} PIXELDATA;

typedef struct RLEDATA {       /* DL_RLE */
    WORD mode;                  /* 1-5 */
    WORD frame;
    BYTE *sprites;              /* RLE sprite file: a size, a count, then offsets */
    short x, y;
} RLEDATA;

typedef struct DLENTRY {
    WORD type;
    WORD slot;
    union {
        TILEDATA tile;
        PIXELDATA pixels;
        RLEDATA rle;
        BYTE data[20];
    } u;
} DLENTRY;

typedef struct DISPLAYLIST {
    WORD count;
    WORD unused;
    DLENTRY entries[1];
} DISPLAYLIST;

#ifdef __cplusplus
extern "C" {
#endif

RENDERAPI WORD WINAPI Renderer_Initialise(WORD param);
RENDERAPI void WINAPI Renderer_Quit(WORD renderer);
RENDERAPI WORD WINAPI Renderer_LoadCanvas_BitMap(WORD renderer, PDIB dib, WORD flags, RGBQUAD *colors);
RENDERAPI void WINAPI Renderer_Render(WORD renderer, HDC dc, WORD slot, int left, int top, int right,
                                      int bottom, int width, int height, int mode);
RENDERAPI WORD WINAPI Renderer_RenderList(WORD renderer, DISPLAYLIST *list);
RENDERAPI WORD WINAPI Renderer_SpriteList(WORD renderer, SPRITELIST *list);
RENDERAPI void WINAPI Renderer_Revealed(WORD renderer, WORD slot, int left, int top, int right, int bottom);
RENDERAPI DISPLAYLIST *WINAPI NewDisplayList(int count);
RENDERAPI SPRITELIST *WINAPI NewSpriteList(int count);
RENDERAPI void WINAPI FreeDisplayList(DISPLAYLIST *list);
RENDERAPI void WINAPI FreeSpriteList(SPRITELIST *list);
RENDERAPI HBITMAP WINAPI GetBMP4Canvas(WORD renderer, WORD slot);
RENDERAPI BYTE *WINAPI GetDIB4Canvas(WORD renderer, WORD slot, int x, int y, int width, int height);
RENDERAPI BYTE *WINAPI GetSurfacePointer(WORD renderer, WORD slot);
RENDERAPI int WINAPI GetSurfaceModulo(WORD renderer, WORD slot);
RENDERAPI void WINAPI SetClipRegion(WORD renderer, WORD slot, int x, int y, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
