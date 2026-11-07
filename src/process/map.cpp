/*
 * The level's terrain as a bitmap of solid pixels (what the lemmings stand
 * on and dig through) and its grid of object effects. Digging erases the
 * sprite's pixels from the bitmap; building adds them.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3;

#include <string.h>
#include "lemming.h"

/* 0x100046d0: the frames of an object cut out of a slot */
static BYTE **GetGraphics(WORD renderer, WORD slot, int x, int y, int w, int h, int count)
{
    BYTE **frames = (BYTE **)VSMEM_Allocate(count * sizeof(BYTE *), "PROCESS: GetGraphics");
    int i;

    for (i = 0; i < count; i++) {
        frames[i] = GetDIB4Canvas(renderer, slot, x * g_scale, y * g_scale, w * g_scale, h * g_scale);
        x += w;
    }
    return frames;
}

/* Symbol padding (see docs/DECOMP.md). */
extern int pad_addobjecttype0, pad_addobjecttype1, pad_addobjecttype2, pad_addobjecttype3, pad_addobjecttype4, pad_addobjecttype5, pad_addobjecttype6, pad_addobjecttype7, pad_addobjecttype8, pad_addobjecttype9, pad_addobjecttype10, pad_addobjecttype11;

/* 0x10004760: a kind of object, its frames side by side in a slot */
int Map::AddObjectType(WORD renderer, WORD slot, int x, int y, int w, int h, int count)
{
    SPRITELIST *list;
    int i;
    int n = numTypes;
    OBJTYPE *t = &types[n];
    int fx;
    int current;

    list = NewSpriteList(count);
    /* Preserve the loop test before the sprite-list header stores. */
    int more = count > 0;
    list->count = (WORD)count;
    list->slot = slot;
    if (more) {
        for (i = 0, fx = x; i < count; i++, fx = current + (short)w) {
            { WORD v = (WORD)(fx * g_scale); list->frames[i].x = v; }
            current = (short)fx;
#ifdef _MSC_VER
            /* Retain a byte-register use for VC++ 2.2 allocation. */
            switch ((char)fx) { case 1: break; }
#endif
            { WORD v = (WORD)(y * g_scale); list->frames[i].y = v; }
            { WORD v = (WORD)(w * g_scale); list->frames[i].width = v; }
            { WORD v = (WORD)(h * g_scale); list->frames[i].height = v; }
        }
    }
    t->width = w;
    t->height = h;
    t->count = count;
    t->list = Renderer_SpriteList(renderer, list);
    t->frames = GetGraphics(renderer, slot, x, y, w, h, count);
    FreeSpriteList(list);
    numTypes++;
    return n;
}

extern int pad_s66_0,pad_s66_1,pad_s66_2,pad_s66_3,pad_s66_4,pad_s66_5;
/* 0x10004880 */
int Map::AddTrapType(WORD renderer, WORD slot, int x, int y, int w, int h, int count)
{
SPRITELIST *list;
    int i;
    int n = numTypes;
    OBJTYPE *t = &types[n];
    int fx;
    int current;

    list = NewSpriteList(count);
    int more = count > 0;
    list->count = (WORD)count;
    list->slot = slot;
    if (more) {
        for (i = 0, fx = (short)x; i < count; i++, fx = current + (short)w) {
            { WORD v = (WORD)(fx * g_scale); list->frames[i].x = v; }
            current = (short)fx;
#ifdef _MSC_VER
            switch ((char)fx) { case 1: break; }
#endif
            { WORD v = (WORD)(y * g_scale); list->frames[i].y = v; }
            { WORD v = (WORD)(w * g_scale); list->frames[i].width = v; }
            { WORD v = (WORD)(h * g_scale); list->frames[i].height = v; }
        }
    }
    t->width = w;
    t->height = h;
    t->count = count;
    t->list = Renderer_SpriteList(renderer, list);
    t->frames = GetGraphics(renderer, slot, x, y, w, h, count);
    FreeSpriteList(list);
    numTypes++;
    return n;
}

/* 0x100049a0: clips a rectangle to the given bounds; returns whether it was cut */
static int ClipRect(int *x, int *y, int *width, int *height, int *skipX, int *skipY,
                    int left, int top, int right, int bottom)
{
    int clipped;
    int h;
    int w;

    *x -= left;
    *y -= top;
    w = right - left;
    clipped = 0;
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

/* 0x10004a50: an RLE sprite's pixels at half size (not used) */
BYTE *UncompressRLESpriteHalfed(BYTE *spr, int frame)
{
    int w = (WORD)(((WORD *)spr)[2] >> 1);
    int h = (WORD)(((WORD *)spr)[3] >> 1);
    DWORD size = h * w;
    BYTE *buf = (BYTE *)VSMEM_Allocate(size, "MAP.CPP : UncompressRLESpriteHalfed");
    BYTE *src = spr + 12;
    BYTE *row, *d;
    int y, done, x;
    BYTE c;

    memset(buf, 0, size);
    row = buf + (h - 1) * w;
    for (y = 0; y < ((WORD *)spr)[3]; y++, row -= w) {
        if (y & 1) {
            while (*src != 0x80)
                src++;
            src++;
            continue;
        }
        d = row;
        done = 0;
        x = 0;
        do {
            c = *src++;
            if (c & 0x80) {
                if (c == 0x80) {
                    done = 1;
                } else {
                    for (c &= 0x7f; c > 0; c--, x++) {
                        if (!(x & 1))
                            *d++ = *src++;
                        else
                            src++;
                    }
                }
            } else {
                d += c >> 1;
                x += c;
            }
        } while (!done);
    }
    return buf;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_uncompressrlesprite0, pad_uncompressrlesprite1, pad_uncompressrlesprite2, pad_uncompressrlesprite3, pad_uncompressrlesprite4, pad_uncompressrlesprite5, pad_uncompressrlesprite6, pad_uncompressrlesprite7, pad_uncompressrlesprite8, pad_uncompressrlesprite9, pad_uncompressrlesprite10, pad_uncompressrlesprite11, pad_uncompressrlesprite12, pad_uncompressrlesprite13, pad_uncompressrlesprite14, pad_uncompressrlesprite15, pad_uncompressrlesprite16, pad_uncompressrlesprite17;

/* 0x10004b60: an RLE sprite's pixels, bottom row first */
static BYTE *UncompressRLESprite(BYTE *spr, int frame)
{
    BYTE *row;
    BYTE *d;
    DWORD size = ((WORD *)spr)[2] * ((WORD *)spr)[3];
    int done;
    BYTE *buf = (BYTE *)VSMEM_Allocate(size, "MAP.CPP : UncompressRLESprite");
    BYTE *src = spr + 12;
    int y;
    BYTE c;

    memset(buf, 0, size);
    row = buf + (((WORD *)spr)[3] - 1) * ((WORD *)spr)[2];
    for (y = 0; y < ((WORD *)spr)[3]; y++, row -= ((WORD *)spr)[2]) {
        d = row;
        done = 0;
        do {
            c = *src++;
            if (c & 0x80) {
                if (c == 0x80) {
                    done = 1;
                } else {
                    for (c &= 0x7f; c > 0; c--)
                        *d++ = *src++;
                }
            } else {
                d += c;
            }
        } while (!done);
    }
    return buf;
}

/* 0x10004c20: clears the pixels of a frame of an RLE sprite file */
void Map::RemoveSprite(int x, int y, BYTE *sprites, int frame)
{
    WORD *spr = (WORD *)(sprites + *(DWORD *)(sprites + 6 + frame * 4));
    int w, h;
    BYTE *pixels;

    x += spr[0] / g_scale;
    y += spr[1] / g_scale;
    w = spr[2] / g_scale;
    h = spr[3] / g_scale;
    pixels = UncompressRLESprite((BYTE *)spr, frame);
    RemoveBits(x, y, w, h, pixels);
    VSMEM_Free(pixels);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_mapremovebits0, pad_mapremovebits1, pad_mapremovebits2, pad_mapremovebits3, pad_mapremovebits4, pad_mapremovebits5, pad_mapremovebits6, pad_mapremovebits7, pad_mapremovebits8;

/* 0x10004cb0: clears the bits under a sprite's pixels (at the game's scale) */
void Map::RemoveBits(int x, int y, int w, int h, BYTE *pixels)
{
    int skipX = 0;
    int skipY = 0;
    BYTE m;
    int stride = g_scale * w;
    BYTE mask;
    int sh = g_scale * h;
    BYTE *src;
    BYTE *s;
    BYTE *d;
    BYTE *dst;

    ClipRect(&x, &y, &w, &h, &skipX, &skipY, 0, 0, width, height);
    y = height - y - 1;
    dst = bits + width * y / 8 + x / 8;
    skipY *= g_scale;
    skipX *= g_scale;
    src = pixels + (sh - g_scale - skipY) * stride + skipX;
    mask = (BYTE)(0x80 >> (x & 7));
    for (y = 0; y < h; y++) {
        s = src;
        d = dst;
        m = mask;
        for (x = 0; x < w; x++) {
            if (*s)
                *d &= ~m;
            m >>= 1;
            if (!m) {
                d++;
                m = 0x80;
            }
            s += g_scale;
        }
        dst -= width / 8;
        src -= stride * g_scale;
    }
}

/* 0x10004e10: erases a frame of an RLE sprite file from the terrain and the map */
void Map::EraseSprite(DLENTRY **list, BYTE *sprites, int frame, int x, int y)
{
    DLENTRY *e = *list;

    e->type = DL_RLE;
    e->u.rle.mode = 2;
    e->u.rle.x = (short)(x * g_scale);
    e->u.rle.y = (short)(y * g_scale);
    e->u.rle.sprites = sprites;
    e->u.rle.frame = (WORD)frame;
    (*list)++;
    RemoveSprite(x, y, sprites, frame);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_maperaseobject0, pad_maperaseobject1, pad_maperaseobject2, pad_maperaseobject3, pad_maperaseobject4, pad_maperaseobject5, pad_maperaseobject6, pad_maperaseobject7, pad_maperaseobject8, pad_maperaseobject9, pad_maperaseobject10;

/* 0x10004e70: the same with an object type's frame (not used) */
void Map::EraseObject(int type, DLENTRY **list, int x, int y, int frame)
{
    OBJTYPE *t = &types[type];
    DLENTRY *e = *list;

    e->type = DL_TILE;
    e->u.tile.mode = 2;
    e->u.tile.x = (short)(x * g_scale);
    e->u.tile.y = (short)(y * g_scale);
    e->u.tile.list = t->list;
    e->u.tile.frame = (WORD)frame;
    (*list)++;
    RemoveBits(x, y, t->width, t->height, t->frames[frame]);
}

/* 0x10004ef0: sets the pixels of a frame of an RLE sprite file */
void Map::AddSpriteBits(int x, int y, BYTE *sprites, int frame)
{
    WORD *spr = (WORD *)(sprites + *(DWORD *)(sprites + 6 + frame * 4));
    int w, h;
    BYTE *pixels;

    x += spr[0] / g_scale;
    y += spr[1] / g_scale;
    w = spr[2] / g_scale;
    h = spr[3] / g_scale;
    pixels = UncompressRLESprite((BYTE *)spr, frame);
    AddBits(x, y, w, h, pixels);
    VSMEM_Free(pixels);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_mapaddbits0, pad_mapaddbits1, pad_mapaddbits2, pad_mapaddbits3, pad_mapaddbits4, pad_mapaddbits5, pad_mapaddbits6, pad_mapaddbits7, pad_mapaddbits8, pad_mapaddbits9, pad_mapaddbits10, pad_mapaddbits11, pad_mapaddbits12, pad_mapaddbits13, pad_mapaddbits14, pad_mapaddbits15, pad_mapaddbits16, pad_mapaddbits17, pad_mapaddbits18, pad_mapaddbits19, pad_mapaddbits20, pad_mapaddbits21, pad_mapaddbits22, pad_mapaddbits23, pad_mapaddbits24, pad_mapaddbits25, pad_mapaddbits26, pad_mapaddbits27, pad_mapaddbits28, pad_mapaddbits29, pad_mapaddbits30, pad_mapaddbits31, pad_mapaddbits32;

/* 0x10004f80 */
void Map::AddBits(int x, int y, int w, int h, BYTE *pixels)
{
    int skipX = 0;
    BYTE m;
    BYTE mask;
    int skipY = 0;
    BYTE *s;
    BYTE *d;
    BYTE *dst;
    BYTE *src;
    int stride = w * g_scale;
    int sh = g_scale * h;

    ClipRect(&x, &y, &w, &h, &skipX, &skipY, 0, 0, width, height);
    y = height - y - 1;
    dst = bits + width * y / 8 + x / 8;
    skipY *= g_scale;
    skipX *= g_scale;
    src = pixels + (sh - g_scale - skipY) * stride + skipX;
    mask = (BYTE)(0x80 >> (x & 7));
    for (y = 0; y < h; y++) {
        s = src;
        d = dst;
        m = mask;
        for (x = 0; x < w; x++) {
            if (*s)
                *d |= m;
            m >>= 1;
            if (!m) {
                d++;
                m = 0x80;
            }
            s += g_scale;
        }
        dst -= width / 8;
        src -= stride * g_scale;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_mapaddsprite0, pad_mapaddsprite1, pad_mapaddsprite2, pad_mapaddsprite3, pad_mapaddsprite4;

/* 0x100050e0: draws a frame of an RLE sprite file into the terrain and the map */
void Map::AddSprite(DLENTRY **list, BYTE *sprites, int frame, int x, int y)
{
    DLENTRY *e = *list;

    e->type = DL_RLE;
    e->u.rle.mode = 3;
    e->u.rle.x = (short)(g_scale * x);
    e->u.rle.y = (short)(g_scale * y);
    e->u.rle.sprites = sprites;
    e->u.rle.frame = (WORD)frame;
    (*list)++;
    AddSpriteBits(x, y, sprites, frame);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_mapaddobject0, pad_mapaddobject1;

/* 0x10005140: the same with an object type's frame (not used) */
void Map::AddObject(int type, DLENTRY **list, int x, int y, int frame)
{
    OBJTYPE *t = &types[type];
    DLENTRY *e = *list;

    e->type = DL_TILE;
    e->u.tile.mode = 3;
    e->u.tile.x = (short)(x * g_scale);
    e->u.tile.y = (short)(y * g_scale);
    e->u.tile.list = t->list;
    e->u.tile.frame = (WORD)frame;
    (*list)++;
    AddBits(x, y, t->width, t->height, t->frames[frame]);
}

/* 0x100051c0 */
Map::Map()
{
    int i;

    bits = NULL;
    grid = NULL;
    numTypes = 0;
    for (i = 0; i < 10; i++)
        types[i].frames = NULL;
}

/* 0x100051f0: a bit for each pixel that isn't 0 (every other pixel and row at scale 2) */
static BYTE *BuildCollision(BYTE *pixels, int w, int h)
{
    BYTE *bits;
    BYTE *d;
    BYTE *s;
    int x, y;

#ifdef _MSC_VER
    switch (w - h) { case 1: break; }
#endif
    bits = (BYTE *)VSMEM_Allocate((w + 7) / 8 * h, "PROCESS(map.cpp): Collision map array");
    d = bits;
    s = pixels;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x += 8, s++, d++) {
            if (g_scale == 2) {
                *d = 0;
                *d |= *s++ ? 0x80 : 0;
                s++;
                *d |= *s++ ? 0x40 : 0;
                s++;
                *d |= *s++ ? 0x20 : 0;
                s++;
                *d |= *s++ ? 0x10 : 0;
                s++;
                *d |= *s++ ? 0x08 : 0;
                s++;
                *d |= *s++ ? 0x04 : 0;
                s++;
                *d |= *s++ ? 0x02 : 0;
                s++;
                *d |= *s++ ? 0x01 : 0;
            } else {
                *d = 0;
                *d |= *s++ ? 0x80 : 0;
                *d |= *s++ ? 0x40 : 0;
                *d |= *s++ ? 0x20 : 0;
                *d |= *s++ ? 0x10 : 0;
                *d |= *s++ ? 0x08 : 0;
                *d |= *s++ ? 0x04 : 0;
                *d |= *s++ ? 0x02 : 0;
                { BYTE b = *s ? 0x01 : 0; *d |= b; }
            }
        }
        if (g_scale == 2)
            s += w * 2;
    }
    return bits;
}

/* 0x10005370 */
Map::Map(BYTE *pixels, int width, int height)
{
    bits = BuildCollision(pixels, width, height);
    this->height = height;
    this->width = (width + 3) & ~3;
    grid = new OBJGRID(width, height);
    numTypes = 0;
    for (width = 0; width < 10; width++)
        types[width].frames = NULL;
}

/* 0x10005430 */
Map::~Map()
{
    int i, j;
    OBJGRID *g = grid;

    if (g) {
        if (g->cells) {
            delete g->cells;
            g->cells = NULL;
        }
        delete g;
        grid = NULL;
    }
    if (bits) {
        VSMEM_Free(bits);
        bits = NULL;
    }
    for (i = 0; i < 10; i++) {
        if (types[i].frames) {
            for (j = 0; j < types[i].count; j++)
                VSMEM_Free(types[i].frames[j]);
            VSMEM_Free(types[i].frames);
        }
    }
}

/* 0x100054c0: whether the terrain is solid there */
BYTE Map::GetPixel(int x, int y)
{
    if (width <= x || x < 0 || height <= y || y < 0)
        return 0;
    y = height - y - 1;
    return bits[width / 8 * y + x / 8] & (0x80 >> (x & 7));
}

/* 0x10005530: the object effect there */
int Map::GetObjectAt(int x, int y)
{
    x /= 4;
    y /= 4;
    if (y < 0)
        return 0;
    if (grid->rows <= y)
        return 0;
    return grid->cells[grid->cols * y + x];
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_mapsetobjectat0, pad_mapsetobjectat1, pad_mapsetobjectat2, pad_mapsetobjectat3;

/* 0x10005580: sets the object effect there; returns the old one */
int Map::SetObjectAt(int x, int y, int value)
{
    int old;
    OBJGRID *g = grid;
    int *cell;

    x /= 4;
    y /= 4;
    if (y < 0)
        return 0;
    if (g->rows <= y)
        return 0;
    cell = &g->cells[g->cols * y + x];
    old = *cell;
    *cell = value;
    return old;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_mapfillobjects0, pad_mapfillobjects1, pad_mapfillobjects2;

/* 0x100055e0: sets the object effect over a rectangle */
void Map::FillObjects(int x, int y, int w, int h, int value)
{
    int i;
    int y1;
    int x0;
    int y0;
    OBJGRID *g = grid;
    int x1;
    int j;

    x0 = x / 4;
    y0 = y / 4;
    x1 = x0 + w / 4;
    y1 = y0 + h / 4;
    if (y1 < 0 || x1 < 0 || g->rows <= y0 || g->cols <= x0)
        return;
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > g->cols)
        x1 = g->cols;
    if (y1 > g->rows)
        y1 = g->rows;
    for (j = y0; j < y1; j++)
        for (i = x0; i < x1; i++)
            g->cells[g->cols * j + i] = value;
}

/* 0x100056b0: a new terrain bitmap (the size and objects stay) */
void Map::SetMap(BYTE *pixels, int width, int height)
{
    if (bits) {
        VSMEM_Free(bits);
        bits = NULL;
    }
    bits = BuildCollision(pixels, width, height);
}
