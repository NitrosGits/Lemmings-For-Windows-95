/* The basher: digs sideways until there is nothing left to bash. */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1;

#include "lemming.h"

/* 0x10001000: steel, or a one-way wall facing the wrong way */
static int CantBash(LEMMING *l, int x, int y)
{
    int oneway;
    int obj = g_map->GetObjectAt(x, y) & 0xff;

    oneway = l->dir == 1 ? OBJ_ONEWAYLEFT : OBJ_ONEWAYRIGHT;
    if ((BYTE)obj == OBJ_STEEL || obj == oneway)
        return 1;
    return 0;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_basherprocess0, pad_basherprocess1;

/* 0x10001040 */
void Basher::Process(LEMMING *l, DLENTRY **list)
{
    int f;
    int x, y;
    PNT at;

    if ((f = ++l->frame) >= l->frames)
        l->frame = l->loop - l->frames + f;
    switch (l->frame) {
    case 2: case 3: case 4: case 5:
    case 18: case 19: case 20: case 21:
        x = l->xoff + l->x;
        y = l->y + l->yoff - 6;
        f = (l->dir == -1 ? 4 : 0) + (l->frame & 15);
        g_map->EraseSprite(list, g_sprites, f + SPR_DIGHOLE, x, y);
        x = (l->x & ~3) + l->dir * 8;
        y = (l->y & ~3) - 8;
        if (CantBash(l, x, y)) {
            at.x = l->x;
            at.y = l->y + l->yoff;
            SoundPlayFX(at, 6);
            Leave(l);
            g_walker.Enter(l);
            return;
        }
        if (l->frame == 5) {
            x = l->x + l->dir * 8;
            y = l->y - 6;
            if (!g_map->GetPixel(x, y) && !g_map->GetPixel(x += l->dir, y) && !g_map->GetPixel(x += l->dir, y)) {
                Leave(l);
                g_walker.Enter(l);
                return;
            }
        }
        break;
    case 11: case 12: case 13: case 14: case 15:
    case 27: case 28: case 29: case 30: case 31:
        x = l->x += l->dir;
        y = l->y;
        if (!g_map->GetPixel(x, y) && !g_map->GetPixel(x, ++y) && !g_map->GetPixel(x, ++y))
            Message(l, MSG_FALL, 0, 0);
        /* falls into the frames that do nothing */
    case 7: case 9: case 17: case 23: case 25:
        /* Dead store: keeps these labels in the VC++ 2.2 jump table (see miner.cpp). */
        x = 0;
        break;
    }
}

/* 0x10001250 */
void Basher::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x100012c0 */
void Basher::Leave(LEMMING *l)
{
}

/* 0x100012d0 */
void Basher::Enter(LEMMING *l)
{
    l->action = &g_basher;
    l->type = LT_BUSY | LT_ASSIGNABLE;
    l->name = "Basher";
    l->base = 0xde;
    l->frames = 32;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -10;
}

/* 0x10001310 */
int Basher::Assign(LEMMING *l)
{
    if (!CanAssign(l, LT_ASSIGNABLE))
        return 0;
    if (CantBash(l, l->x, l->y - 9))
        return 0;
    l->action->Leave(l);
    Enter(l);
    return 1;
}

/* 0x10001370 */
void Basher::Init(int a, int b)
{
    leftFrames = 32;
}
