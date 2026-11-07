/*
 * LOADLVL.DLL - loads a level: its .LVL file (the DOS game's format, big-
 * endian), the style's object definitions (GROUNDnO.DAT), and the terrain,
 * drawn from the style's RLE sprites into a new 8-bit DIB.
 */
#include <windows.h>

#define LOADLVL_BUILD
#include "loadlvl.h"
#include "readdib.h"
#include "filehand.h"
#include "vsmem.h"
#include "vscrt.h"

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12;

#define MAX_OBJTYPES 16
#define MAX_OBJECTS  32
#define MAX_TERRAIN  400
#define MAX_STEEL    32

/* An object definition from the style's ground file, scaled */
typedef struct OBJTYPE {
    int startFrame;
    int endFrame;
    int width;
    int height;
    int trigLeft;
    int trigTop;
    int trigWidth;
    int trigHeight;
    int trigEffect;
    int animType;
    int sound;
    WORD sprite;
} OBJTYPE;

/* The style and level files' layouts, byte-packed */
#pragma pack(1)

/* The object definitions as stored in the ground file (28 bytes each) */
typedef struct GROUNDOBJ {
    BYTE flags;
    BYTE flags2;
    BYTE startFrame;
    BYTE endFrame;
    BYTE width;
    BYTE height;
    BYTE other[8];
    WORD trigLeft;
    WORD trigTop;
    BYTE trigWidth;
    BYTE trigHeight;
    BYTE trigEffect;
    BYTE other2[6];
    BYTE sound;
} GROUNDOBJ;

/* A .LVL file */
typedef struct LVLFILE {
    WORD releaseRate;
    WORD numLemmings;
    WORD toRescue;
    WORD timeLimit;
    WORD skills[8];
    WORD startX;
    WORD graphicSet;
    WORD extGraphicSet;
    WORD unused;
    BYTE objects[MAX_OBJECTS][8];
    BYTE terrain[MAX_TERRAIN][4];
    BYTE steel[MAX_STEEL][4];
    char name[32];
} LVLFILE;

/* An RLE sprite file: a count, then each sprite's offset */
typedef struct SPRITEFILE {
    DWORD size;
    WORD count;
    DWORD offset[1];
} SPRITEFILE;

typedef struct SPRITE {
    WORD x, y;
    WORD width, height;
    DWORD size;
    BYTE data[1];       /* runs: 0x80 ends a row, 0x81-0xff that many bytes, 0-0x7f skip */
} SPRITE;

typedef struct PALFILE {
    DWORD size;
    WORD count;
    BYTE rgb[1][4];
} PALFILE;

#pragma pack()

/* frames per object type */
typedef struct FRAMES {
    int count;
    BYTE pad[0x200];
} FRAMES;

BYTE *g_objSprites = 0;
PALFILE *g_palette = 0;

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_topscale0, pad_topscale1, pad_topscale2;

/* The .bss order follows a hash of these names (docs/DECOMP.md): these names give the original's layout. */
static int g_pixelScale;
static int g_unused;
static OBJTYPE g_types[MAX_OBJTYPES];
static LVLFILE g_levelFile;
extern GROUNDOBJ *g_ground;

static void NumberSprites(int a, int b, FRAMES *frames);
static void LoadSteel(LEVEL *level);
static void AddObject(LEVEL *level, WORD *obj);
static void ReadObjType(OBJTYPE *t, GROUNDOBJ *g);
static void CountFrames(int a, int b);
static void SwapObject(WORD *obj);
static int LoadObjects(int a, LEVEL *level);
static int LoadTerrain(LEVEL *level);
static void DecodeTerrain(DWORD v, int *id, int *x, int *y, int *flags);
static void BlitClipped(BYTE *src, int width, int height, int srcStep, BYTE *dst, int dstStep);
static void BlitRLE(SPRITE *spr, BYTE *dst, int dstStep);
static void EraseClipped(BYTE *src, int width, int height, int srcStep, BYTE *dst, int dstStep);
static void EraseRLE(SPRITE *spr, BYTE *dst, int dstStep);
static void UnderClipped(BYTE *src, int width, int height, int srcStep, BYTE *dst, int dstStep);
static void UnderRLE(SPRITE *spr, BYTE *dst, int dstStep);
static BYTE *UnpackRLE(SPRITE *spr);
static void DrawPiece(SPRITEFILE *sprites, PDIB dib, int flags, int x, int y, int id);
static PDIB NewDib(int width, int height, PALFILE *pal);
static PDIB DrawTerrain(LVLFILE *lvl, SPRITEFILE *sprites);
static void LoaderError(const char *msg);
static void *LoaderAlloc(DWORD size, BYTE fill);
static void LoaderFree(void *p);
static DWORD LoaderFileLength(HANDLE file);
static void LoaderRead(HANDLE file, BYTE *buf, long size);
static DWORD ReadLong(BYTE *p);
static WORD ReadWord(BYTE *p);
static void SwapHeader(WORD *w);
static int LoadLevelDesc(const char *file, LEVEL *level);
static void StyleGraphicsFiles(int set, char *sprites, char *palette);
static void *LoadFile(const char *name, const char *tag);
static int LoadStyleGraphics(LEVEL *level, SPRITEFILE **sprites);
static void StyleGroundFiles(int set, char *ground, char *objects);
static int LoadGroundData(LEVEL *level, BYTE **objSprites);

/* 0x10001000 */
void WINAPI LVL_LibMain(HINSTANCE hinst)
{
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_numbersprites0, pad_numbersprites1, pad_numbersprites2, pad_numbersprites3, pad_numbersprites4, pad_numbersprites5, pad_numbersprites6, pad_numbersprites7, pad_numbersprites8, pad_numbersprites9, pad_numbersprites10, pad_numbersprites11, pad_numbersprites12, pad_numbersprites13, pad_numbersprites14, pad_numbersprites15, pad_numbersprites16, pad_numbersprites17, pad_numbersprites18, pad_numbersprites19, pad_numbersprites20, pad_numbersprites21;

/* 0x10001010: each object type's first frame in the objects sprite file */
static void NumberSprites(int a, int b, FRAMES *frames)
{
    WORD next = 0;
    int i;
    int n;

    for (i = 0; i < MAX_OBJTYPES; i++) {
        n = frames[i].count;
        if (n > 0)
            g_types[i].sprite = next;
        next += n;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_loadsteel0, pad_loadsteel1, pad_loadsteel2, pad_loadsteel3, pad_loadsteel4, pad_loadsteel5, pad_loadsteel6, pad_loadsteel7, pad_loadsteel8, pad_loadsteel9, pad_loadsteel10, pad_loadsteel11, pad_loadsteel12, pad_loadsteel13;

/* 0x10001040 */
static void LoadSteel(LEVEL *level)
{
    BYTE *p = g_levelFile.steel[0];
    DWORD v;
    int i;
    STEELAREA *s;

    level->numSteel = 0;
    level->steel = (STEELAREA *)LoaderAlloc(MAX_STEEL * sizeof(STEELAREA), 0);
    for (i = 0; i < MAX_STEEL; i++, p += 4) {
        v = ReadLong(p);
        if (v) {
            s = &level->steel[level->numSteel];
            s->x = (v & 0xff9fffff) >> 21;
            s->y = ((v & 0x7f0000) >> 16) << 2;
            s->width = ((v & 0xf000) >> 10) + 4;
            s->height = ((v & 0xf00) >> 6) + 4;
            s->type = 9;
            level->numSteel++;
        }
    }
}

/* 0x100010e0 */
static void AddObject(LEVEL *level, WORD *obj)
{
    LEVELOBJECT *o;
    OBJTYPE *t;

    if (obj[0] == 0 && obj[1] == 0 && obj[2] == 0)
        return;
    o = &level->objects[level->numObjects];
    t = &g_types[obj[2]];
    o->x = obj[0];
    o->y = obj[1];
    o->width = t->width;
    o->height = t->height;
    o->startFrame = t->startFrame;
    o->endFrame = t->endFrame;
    o->frame = 0;
    o->sprite = t->sprite;
    o->animType = t->animType;
    o->trigLeft = t->trigLeft;
    o->trigTop = t->trigTop;
    o->trigWidth = t->trigWidth;
    o->trigHeight = t->trigHeight;
    o->trigEffect = t->trigEffect;
    o->sound = t->sound;
    o->drawMode = 1;
    if (obj[3] & 0x4000)
        o->drawMode = 5;
    else if (obj[3] & 0x8000)
        o->drawMode = 4;
    level->numObjects++;
}

/* 0x100011b0 */
static void ReadObjType(OBJTYPE *t, GROUNDOBJ *g)
{
    t->startFrame = g->startFrame;
    t->endFrame = g->endFrame;
    t->width = g->width;
    t->height = g->height;
    t->trigLeft = g->trigLeft * 4;
    t->trigTop = g->trigTop * 4 - 4;
    t->trigWidth = g->trigWidth * 4;
    t->trigHeight = g->trigHeight * 4;
    t->trigEffect = g->trigEffect;
    t->sound = g->sound;
    t->animType = 1;
    if (g->flags & 2)
        t->animType = 1;
    if (g->flags & 1)
        t->animType = 4;
    if (g->flags & 4)
        t->animType = 3;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_countframes0, pad_countframes1, pad_countframes2;

/* 0x10001250 */
static void CountFrames(int a, int b)
{
    FRAMES frames[MAX_OBJTYPES];
    FRAMES *f;
    BYTE n;
    int i;

    memset(frames, 0, sizeof frames);
    for (f = frames, i = 0; f < frames + MAX_OBJTYPES; i++, f++) {
        n = g_ground[i].endFrame;
        if (n >= 0x40)
            n = 0x40;
        f->count = n;
    }
    NumberSprites(a, b, frames);
}

/* 0x100012d0 */
static void SwapObject(WORD *obj)
{
    obj[0] = ReadWord((BYTE *)&obj[0]);
    obj[1] = ReadWord((BYTE *)&obj[1]);
    obj[2] = ReadWord((BYTE *)&obj[2]);
    obj[3] = ReadWord((BYTE *)&obj[3]);
}

/* 0x10001320 */
static int LoadObjects(int a, LEVEL *level)
{
    int ok = 0;
    OBJTYPE *t;
    WORD *obj;
    int i;

    if (LoadGroundData(level, &g_objSprites)) {
        t = g_types;
        for (i = 0; i < MAX_OBJTYPES; i++, t++) {
            if (i == 1)
                g_ground[i].flags |= 4;
            ReadObjType(t, &g_ground[i]);
        }
        CountFrames(a, (int)level);
        LoaderFree(g_ground);
        level->objects = (LEVELOBJECT *)LoaderAlloc(MAX_OBJECTS * sizeof(LEVELOBJECT), 0);
        level->numObjects = 0;
        obj = (WORD *)g_levelFile.objects;
        for (i = 0; i < MAX_OBJECTS; i++, obj += 4) {
            SwapObject(obj);
            AddObject(level, obj);
        }
        LoadSteel(level);
        ok = 1;
        level->objectSprites = g_objSprites;
    }
    return ok;
}

/* 0x100013f0 */
static int LoadTerrain(LEVEL *level)
{
    int ok = 0;
    SPRITEFILE *sprites;

    level->terrain = NULL;
    if (LoadLevelDesc(level->fileName, level)) {
        sprites = NULL;
        if (LoadStyleGraphics(level, &sprites)) {
            level->terrain = DrawTerrain(&g_levelFile, sprites);
            if (level->terrain != NULL)
                ok = 1;
        }
        if (sprites != NULL)
            LoaderFree(sprites);
        if (g_palette != NULL) {
            LoaderFree(g_palette);
            g_palette = NULL;
        }
    }
    return ok;
}

/* 0x10001480 */
BOOL WINAPI LoadLevel(const char *file, int hwnd, LEVEL *level, int scale)
{
    BOOL ok = FALSE;
    char *name;

    name = (char *)LoaderAlloc(strlen(file) + 1, 0);
    strcpy(name, file);
    level->fileName = name;
    g_pixelScale = scale;
    if (LoadTerrain(level))
        ok = LoadObjects(hwnd, level);
    return ok;
}

/* 0x10001500 */
void WINAPI LevelFree(LEVEL *level)
{
    if (level->terrain != NULL) {
        LoaderFree(level->terrain);
        level->terrain = NULL;
    }
    if (level->objectSprites != NULL) {
        LoaderFree(level->objectSprites);
        level->objectSprites = NULL;
    }
    LoaderFree(level->objects);
    LoaderFree(level->steel);
    LoaderFree(level->name);
    LoaderFree(level->fileName);
    level->objects = NULL;
    level->steel = NULL;
    level->name = NULL;
    level->fileName = NULL;
}

/* 0x10001590: a terrain piece: sprite, x, y (9-bit signed), flags */
static void DecodeTerrain(DWORD v, int *id, int *x, int *y, int *flags)
{
    *id = v & 0x3f;
    *x = (v & 0x1fff0000) >> 16;
    *y = (v & 0xff80) >> 7;
    *flags = 0;
    if (v & 0x80000000)
        *flags = 1;
    if (v & 0x40000000)
        *flags |= 4;
    if (v & 0x20000000)
        *flags |= 2;
    if (*y & 0x100)
        *y |= 0xffffff00;
}

/* 0x10001600: an unpacked sprite, clipped */
static void BlitClipped(BYTE *src, int width, int height, int srcStep, BYTE *dst, int dstStep)
{
    int x;
    BYTE *s;
    int y;
    BYTE *d;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++, d++, s++)
            if (*s)
                *d = *s | 0x80;
        dst -= dstStep;
        src -= srcStep;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_blitrle0, pad_blitrle1, pad_blitrle2, pad_blitrle3, pad_blitrle4, pad_blitrle5, pad_blitrle6, pad_blitrle7, pad_blitrle8, pad_blitrle9, pad_blitrle10, pad_blitrle11, pad_blitrle12, pad_blitrle13, pad_blitrle14;

/* 0x10001650 */
static void BlitRLE(SPRITE *spr, BYTE *dst, int dstStep)
{
    int y;
    BYTE *d;
    int end;
    BYTE c;
    BYTE *p = spr->data;

    for (y = 0; y < spr->height; y++, dst -= dstStep) {
        d = dst;
        end = 0;
        do {
            c = *p++;
            if (c & 0x80) {
                if (c == 0x80)
                    end = 1;
                else
                    for (c &= 0x7f; c > 0; c--) {
                        *d = *p | 0x80;
                        d++;
                        p++;
                    }
            } else
                d += c;
        } while (!end);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_eraseclipped0, pad_eraseclipped1, pad_eraseclipped2, pad_eraseclipped3, pad_eraseclipped4, pad_eraseclipped5, pad_eraseclipped6, pad_eraseclipped7, pad_eraseclipped8, pad_eraseclipped9, pad_eraseclipped10, pad_eraseclipped11, pad_eraseclipped12, pad_eraseclipped13;

/* 0x100016c0: erases where the sprite has pixels */
static void EraseClipped(BYTE *src, int width, int height, int srcStep, BYTE *dst, int dstStep)
{
    int y, unused;      /* an unused local: the original had one more symbol here */

    for (y = 0; y < height; y++) {
        BYTE *s = src;
        BYTE *d = dst;
        int x;

        for (x = 0; x < width; x++, d++, s++)
            if (*s)
                *d = 0;
        dst -= dstStep;
        src -= srcStep;
    }
}

/* 0x10001700 */
static void EraseRLE(SPRITE *spr, BYTE *dst, int dstStep)
{
    BYTE *p = spr->data;
    int y;
    BYTE c;
    BYTE *d;
    int end;

    for (y = 0; y < spr->height; y++, dst -= dstStep) {
        d = dst;
        end = 0;
        do {
            c = *p++;
            if (c & 0x80) {
                if (c == 0x80)
                    end = 1;
                else {
                    for (c &= 0x7f; c > 0; c--) {
                        if (*d)
                            *d = 0;
                        d++;
                        p++;
                    }
                }
            } else
                d += c;
        } while (!end);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_underclipped0, pad_underclipped1, pad_underclipped2;

/* 0x10001770: only where the terrain is empty */
static void UnderClipped(BYTE *src, int width, int height, int srcStep, BYTE *dst, int dstStep)
{
    BYTE *s;
    int x;
    int y;
    BYTE *d;

    for (y = 0; y < height; y++) {
        s = src;
        d = dst;
        for (x = 0; x < width; x++, d++, s++)
            if (*d == 0 && *s)
                *d = *s | 0x80;
        dst -= dstStep;
        src -= srcStep;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_underrle0;

/* 0x100017c0 */
static void UnderRLE(SPRITE *spr, BYTE *dst, int dstStep)
{
    BYTE *p = spr->data;
    int y;
    BYTE c;
    BYTE *d;
    int end;

    for (y = 0; y < spr->height; y++, dst -= dstStep) {
        d = dst;
        end = 0;
        do {
            c = *p++;
            if (c & 0x80) {
                if (c == 0x80)
                    end = 1;
                else
                    for (c &= 0x7f; c > 0; c--) {
                        if (*d == 0 && *p)
                            *d = *p | 0x80;
                        d++;
                        p++;
                    }
            } else
                d += c;
        } while (!end);
    }
}

/* 0x10001830: the sprite unpacked, bottom row first */
static BYTE *UnpackRLE(SPRITE *spr)
{
    BYTE c;
    DWORD size = spr->width * spr->height;
    BYTE *row;
    BYTE *buf = (BYTE *)VSMEM_Allocate(size, "Loader: UncompressRLESprite");
    int y;
    BYTE *d;
    int end;
    BYTE *p = spr->data;

    memset(buf, 0, size);
    row = buf + (spr->height - 1) * spr->width;
    for (y = 0; y < spr->height; y++, row -= spr->width) {
        d = row;
        end = 0;
        do {
            c = *p++;
            if (c & 0x80) {
                if (c == 0x80)
                    end = 1;
                else
                    for (c &= 0x7f; c > 0; c--)
                        *d++ = *p++;
            } else
                d += c;
        } while (!end);
    }
    return buf;
}

/*
 * Matches (session 5). What decided it, all register allocation and scheduling:
 * - width and skip are DWORD; sprWidth is read from spr->width again, not
 *   copied from width (otherwise Visual C++ merges the two).
 * - "dir = 1" stays unconditional, and in the flip block "dir = -1" comes
 *   first, then row, then clipped: in any other order the compiler reuses
 *   dir's register for the constant in "clipped = 1", dir gets ebp and
 *   dibWidth loses it (the original keeps dir on the stack).
 * - step is set after the x/y adjustments; "row -= dir * y" comes before
 *   "height += y".
 * - 18 lines of symbol padding before the function.
 */

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_dp0, pad_dp1, pad_dp2, pad_dp3, pad_dp4, pad_drawpiece0, pad_drawpiece1, pad_drawpiece2,
    pad_drawpiece3, pad_drawpiece4, pad_drawpiece5, pad_drawpiece6, pad_drawpiece7, pad_drawpiece8,
    pad_drawpiece9, pad_drawpiece10, pad_drawpiece11, pad_drawpiece12;

/* 0x100018f0: draws terrain sprite id at (x, y) with flags 1 (behind), 2 (erase), 4 (upside down) */
static void DrawPiece(SPRITEFILE *sprites, PDIB dib, int flags, int x, int y, int id)
{
    BYTE *dst;
    int clipped = 0;
    int step;
    DWORD skip = 0;
    DWORD width;
    BYTE *src;
    int dibHeight;
    int height;
    SPRITE *spr = (SPRITE *)((BYTE *)sprites + sprites->offset[id]);
    int sprWidth;
    int row = 0;
    int dir;
    BYTE *unpacked;
    int dibWidth;
    int rowBytes;

    width = spr->width;
    height = spr->height;
    sprWidth = spr->width;
    x += spr->x;
    y += spr->y;
    step = width;
    rowBytes = GetDibWidth(dib);
    dir = 1;
    if (flags & 4) {
        dir = -1;
        row = height - 1;
        clipped = 1;
        step = -sprWidth;
    }
    dibWidth = GetDibWidth(dib);
    dibHeight = GetDibHeight(dib);
    if (sprWidth + x >= dibWidth) {
        clipped = 1;
        width = dibWidth - x;
    }
    if (x < 0) {
        clipped = 1;
        skip = -x;
        width += x;
        x = 0;
    }
    if (y + height >= dibHeight) {
        clipped = 1;
        height = dibHeight - y;
    }
    if (y < 0) {
        row -= dir * y;
        height += y;
        y = 0;
        clipped = 1;
    }
    if (width == 0 || height == 0)
        return;
    if (clipped) {
        unpacked = UnpackRLE(spr);
        src = unpacked + (spr->height - row - 1) * spr->width + skip;
    }
    dst = GetDibBitsAddr(dib) + (GetDibHeight(dib) - y - 1) * GetDibWidth(dib) + x;
    if (clipped) {
        switch (flags) {
        case 0:
        case 4:
            BlitClipped(src, width, height, step, dst, rowBytes);
            break;
        case 1:
        case 3:
        case 5:
        case 7:
            UnderClipped(src, width, height, step, dst, rowBytes);
            break;
        case 2:
        case 6:
            EraseClipped(src, width, height, step, dst, rowBytes);
            break;
        }
        VSMEM_Free(unpacked);
    } else {
        switch (flags) {
        case 0:
        case 4:
            BlitRLE(spr, dst, rowBytes);
            break;
        case 1:
        case 3:
        case 5:
        case 7:
            UnderRLE(spr, dst, rowBytes);
            break;
        case 2:
        case 6:
            EraseRLE(spr, dst, rowBytes);
            break;
        }
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_newdib0, pad_newdib1, pad_newdib2, pad_newdib3, pad_newdib4, pad_newdib5, pad_newdib6;

/* 0x10001ba0: a blank 8-bit DIB with the style's colours from 128 on */
static PDIB NewDib(int width, int height, PALFILE *pal)
{
    int stride, h;
    DWORD size;
    PDIB dib;
    RGBQUAD *rgb;
    BYTE *src;
    int i;

    stride = (width * g_pixelScale + 3) & ~3;
    h = height * g_pixelScale;
    size = h * stride + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
    dib = (PDIB)LoaderAlloc(size, 0);
    if (dib != NULL) {
        dib->biSize = sizeof(BITMAPINFOHEADER);
        dib->biWidth = stride;
        dib->biHeight = h;
        dib->biPlanes = 1;
        dib->biBitCount = 8;
        dib->biCompression = 0;
        dib->biSizeImage = 0;
        dib->biXPelsPerMeter = 0;
        dib->biYPelsPerMeter = 0;
        dib->biClrUsed = 0;
        dib->biClrImportant = 0;
        memset(((BITMAPINFO *)dib)->bmiColors, 0, 256 * sizeof(RGBQUAD));
        rgb = &((BITMAPINFO *)dib)->bmiColors[128];
        src = pal->rgb[0];
        for (i = 0; i < pal->count; i++, rgb++, src += 4) {
            rgb->rgbRed = src[0];
            rgb->rgbGreen = src[1];
            rgb->rgbBlue = src[2];
            rgb->rgbReserved = 0;
        }
    }
    return dib;
}

/* 0x10001c60: the DIB's colours as 6-bit VGA values (unreferenced) */
void DibToVGA(PDIB dib, BYTE *vga)
{
    int i;

    for (i = 0; i < 256; i++) {
        *vga++ = ((BITMAPINFO *)dib)->bmiColors[i].rgbRed >> 2;
        *vga++ = ((BITMAPINFO *)dib)->bmiColors[i].rgbGreen >> 2;
        *vga++ = ((BITMAPINFO *)dib)->bmiColors[i].rgbBlue >> 2;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawterrain0, pad_drawterrain1, pad_drawterrain2, pad_drawterrain3, pad_drawterrain4;

/* 0x10001ca0 */
static PDIB DrawTerrain(LVLFILE *lvl, SPRITEFILE *sprites)
{
    PDIB dib;
    DWORD v;
    int id, x, y, flags;
    int i;

    dib = NewDib(1664, 160, g_palette);
    if (dib != NULL) {
        for (i = 0; i < MAX_TERRAIN; i++) {
            v = ReadLong(lvl->terrain[i]);
            if (v != 0xffffffff) {
                DecodeTerrain(v, &id, &x, &y, &flags);
                if (sprites->count >= (WORD)id)
                    DrawPiece(sprites, dib, flags, g_pixelScale * x, (y - 4) * g_pixelScale, id);
                else {
                    LoaderFree(dib);
                    dib = NULL;
                }
            }
        }
    }
    return dib;
}

/* 0x10001d60 */
static void LoaderError(const char *msg)
{
    MessageBoxA(NULL, msg, "Error", MB_OK);
}

/* 0x10001d80 */
static void *LoaderAlloc(DWORD size, BYTE fill)
{
    void *p = VSMEM_Allocate(size, "LOADER: VSAlloc");

    if (p != NULL)
        memset(p, fill, size);
    return p;
}

/* 0x10001dc0 */
static void LoaderFree(void *p)
{
    VSMEM_Free(p);
}

/* 0x10001dd0 */
static DWORD LoaderFileLength(HANDLE file)
{
    return FILE_Length(file);
}

/* 0x10001de0 */
static void LoaderRead(HANDLE file, BYTE *buf, long size)
{
    long got;

    while (size > 0) {
        got = FILE_Read(file, buf, size < 0x4000 ? size : 0x4000);
        if (got <= 0)
            break;
        buf += got;
        size -= got;
    }
    if (size)
        LoaderError("File read error\n");
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_loaderwrite0, pad_loaderwrite1, pad_loaderwrite2, pad_loaderwrite3, pad_loaderwrite4, pad_loaderwrite5, pad_loaderwrite6;

/* 0x10001e30 (unreferenced) */
void LoaderWrite(HFILE file, BYTE *buf, long size)
{
    long done;

    while (size > 0) {
        done = _lwrite(file, (LPCSTR)buf, size < 0x4000 ? size : 0x4000);
        if (done <= 0)
            break;
        buf += done;
        size -= done;
    }
    if (size)
        LoaderError("File write error\n");
}

/* 0x10001e80 (unreferenced) */
void LoaderFill(void *p, BYTE c, DWORD size)
{
    memset(p, c, size);
}

char g_iniFile[] = "winlemm.ini";

/* each style's files, at normal and high resolution */
char *g_terrainSprites[] = {
    "styles\\level0s.spr", "styles\\level1s.spr", "styles\\level2s.spr", "styles\\level3s.spr",
    "styles\\level4s.spr", "styles\\bricks.spr", "styles\\rocks.spr", "styles\\snows.spr",
    "styles\\bubbles.spr",
};
char *g_terrainSpritesHi[] = {
    "styles\\level0b.spr", "styles\\level1b.spr", "styles\\level2b.spr", "styles\\level3b.spr",
    "styles\\level4b.spr", "styles\\brickb.spr", "styles\\rockb.spr", "styles\\snowb.spr",
    "styles\\bubbleb.spr",
};
char *g_objectSprites[] = {
    "styles\\level0os.spr", "styles\\level1os.spr", "styles\\level2os.spr", "styles\\level3os.spr",
    "styles\\level4os.spr", "styles\\brickos.spr", "styles\\rockos.spr", "styles\\snowos.spr",
    "styles\\bubbleos.spr",
};
char *g_objectSpritesHi[] = {
    "styles\\level0ob.spr", "styles\\level1ob.spr", "styles\\level2ob.spr", "styles\\level3ob.spr",
    "styles\\level4ob.spr", "styles\\brickob.spr", "styles\\rockob.spr", "styles\\snowob.spr",
    "styles\\bubbleob.spr",
};
char *g_palettes[] = {
    "styles\\level0.pal", "styles\\level1.pal", "styles\\level2.pal", "styles\\level3.pal",
    "styles\\level4.pal", "styles\\brick.pal", "styles\\rock.pal", "styles\\snow.pal",
    "styles\\bubble.pal",
};
char *g_groundFiles[] = {
    "styles\\Ground0o.dat", "styles\\Ground1o.dat", "styles\\Ground2o.dat", "styles\\Ground3o.dat",
    "styles\\Ground4o.dat", "styles\\brick.dat", "styles\\rock.dat", "styles\\snow.dat",
    "styles\\bubbles.dat",
};
int g_unused2 = 0;
GROUNDOBJ *g_ground = 0;

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_readlong0, pad_readlong1, pad_readlong2;

/* 0x10001eb0 */
static DWORD ReadLong(BYTE *p)
{
    return ((((((DWORD)p[0] << 8) + p[1]) << 8) + p[2]) << 8) + p[3];
}

/* 0x10001ee0 */
static WORD ReadWord(BYTE *p)
{
    return (WORD)(((WORD)p[0] << 8) + (WORD)p[1]);
}

/* 0x10001f00: the .LVL header's words, from big-endian */
static void SwapHeader(WORD *w)
{
    w[0] = ReadWord((BYTE *)&w[0]);
    w[1] = ReadWord((BYTE *)&w[1]);
    w[2] = ReadWord((BYTE *)&w[2]);
    w[3] = ReadWord((BYTE *)&w[3]);
    w[4] = ReadWord((BYTE *)&w[4]);
    w[5] = ReadWord((BYTE *)&w[5]);
    w[6] = ReadWord((BYTE *)&w[6]);
    w[7] = ReadWord((BYTE *)&w[7]);
    w[8] = ReadWord((BYTE *)&w[8]);
    w[9] = ReadWord((BYTE *)&w[9]);
    w[10] = ReadWord((BYTE *)&w[10]);
    w[11] = ReadWord((BYTE *)&w[11]);
    w[12] = ReadWord((BYTE *)&w[12]);
    w[13] = ReadWord((BYTE *)&w[13]);
    w[14] = ReadWord((BYTE *)&w[14]);
}

/* 0x10001ff0 */
static int LoadLevelDesc(const char *file, LEVEL *level)
{
    char msg[128];
    HANDLE f;
    int seen;
    int i;
    char c;

    wsprintfA(msg, "LoadLevelDesc = %08lx...\n", level);
    f = FILE_ReadOpen(file);
    level->graphicSet = -1;
    LoaderRead(f, (BYTE *)&g_levelFile, LoaderFileLength(f));
    SwapHeader((WORD *)&g_levelFile);
    level->graphicSet = g_levelFile.graphicSet;
    level->releaseRate = g_levelFile.releaseRate;
    level->toRescue = g_levelFile.toRescue;
    level->numLemmings = g_levelFile.numLemmings;
    level->timeLimit = g_levelFile.timeLimit;
    level->startX = g_levelFile.startX + 160;
    level->field60 = 0;
    for (i = 0; i < 7; i++)
        level->index[i] = i;
    level->skills[0] = g_levelFile.skills[0];
    level->skills[1] = g_levelFile.skills[1];
    level->skills[2] = g_levelFile.skills[2];
    level->skills[3] = g_levelFile.skills[3];
    level->skills[4] = g_levelFile.skills[4];
    level->skills[5] = g_levelFile.skills[5];
    level->skills[6] = g_levelFile.skills[6];
    level->skills[7] = g_levelFile.skills[7];
    seen = 0;
    level->name = (char *)LoaderAlloc(33, 0);
    level->name[32] = 0;
    for (i = 31; i >= 0; i--) {
        c = g_levelFile.name[i];
        if (c == ' ' && !seen)
            level->name[i] = 0;
        else {
            level->name[i] = c;
            seen = 1;
        }
    }
    FILE_Close(f);
    return 1;
}

/* 0x10002150 */
static void StyleGraphicsFiles(int set, char *sprites, char *palette)
{
    char *pal = set > 8 ? "none" : g_palettes[set];
    char *spr = set > 8 ? "none" : g_pixelScale == 2 ? g_terrainSpritesHi[set] : g_terrainSprites[set];

    strcpy(sprites, spr);
    strcpy(palette, pal);
}

/* 0x100021e0 */
static void *LoadFile(const char *name, const char *tag)
{
    HANDLE f = FILE_ReadOpen(name);
    DWORD size = FILE_Length(f);
    void *p = VSMEM_Allocate(size, tag);

    FILE_Read(f, p, size);
    FILE_Close(f);
    return p;
}

/* 0x10002220 */
static int LoadStyleGraphics(LEVEL *level, SPRITEFILE **sprites)
{
    char palette[80];
    char spr[80];

    StyleGraphicsFiles(level->graphicSet, spr, palette);
    *sprites = (SPRITEFILE *)LoadFile(spr, "RLE sprite level graphics");
    g_palette = (PALFILE *)LoadFile(palette, "Palette file");
    return 1;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_stylegroundfiles0, pad_stylegroundfiles1, pad_stylegroundfiles2, pad_stylegroundfiles3;

/* 0x10002280 */
static void StyleGroundFiles(int set, char *ground, char *objects)
{
    char *dat = set > 8 ? "none" : g_groundFiles[set];
    char *obj = set > 8 ? "none" : g_pixelScale == 2 ? g_objectSpritesHi[set] : g_objectSprites[set];

    strcpy(ground, dat);
    strcpy(objects, obj);
}

/* 0x10002310 */
static int LoadGroundData(LEVEL *level, BYTE **objSprites)
{
    char objects[80];
    char ground[80];

    StyleGroundFiles(level->graphicSet, ground, objects);
    g_ground = (GROUNDOBJ *)LoadFile(ground, "Ground data file");
    *objSprites = (BYTE *)LoadFile(objects, "Objects RLE Sprite file");
    return 1;
}

/* 0x10002370: the shared assertion handler (unreferenced here) */
void VS_AssertFailed(const char *expr, const char *file, unsigned line)
{
    char msg[256];

    wsprintfA(msg, "%s\nin file \"%s\" at line %u\n", expr, file, line);
    MessageBoxA(NULL, msg, "Visual Sciences Assertion Failure", MB_ICONEXCLAMATION);
    ExitProcess(0xaaaa);
}

/* 0x100023c0: unused, as in VSMEM.DLL (the runtime's entry point is the DLL's) */
BOOL WINAPI DllEntryPoint(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH || reason == DLL_THREAD_ATTACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    if (reason == DLL_PROCESS_ATTACH)
        LVL_LibMain(hinst);
    if (reason == DLL_PROCESS_DETACH || reason == DLL_THREAD_DETACH)
        if (!_CRT_INIT(hinst, reason, reserved))
            return FALSE;
    return TRUE;
}
