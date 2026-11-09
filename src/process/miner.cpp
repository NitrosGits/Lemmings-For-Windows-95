/* The miner: digs diagonally down. */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0;

#include "lemming.h"

/* 0x10003c20: steel, or a one-way wall facing the wrong way */
static int CantMine(LEMMING *l, int x, int y)
{
    int oneway;
    int obj = g_map->GetObjectAt(x, y) & 0xff;

    oneway = l->dir == 1 ? OBJ_ONEWAYLEFT : OBJ_ONEWAYRIGHT;
    if ((BYTE)obj == OBJ_STEEL || obj == oneway)
        return 1;
    return 0;
}

/* 0x10003c60 */
void Miner::Process(LEMMING *l, DLENTRY **list)
{
    int x, y;
    int n;
    PNT at;

    if (++l->frame >= l->frames)
        l->frame = l->loop - l->frames + l->frame;
    switch (l->frame) {
    case 0:
        l->y++;
        break;
    case 1: case 2:
        x = l->xoff + l->x;
        y = l->yoff + l->y - 3;
        n = l->frame - 1;
        if (n == 0) {
            x += l->dir;
            y++;
        }
        n += l->dir == -1 ? 2 : 0;
        g_map->EraseSprite(list, g_sprites, n + SPR_MINEHOLE, x, y + 1);
        break;
    case 3:
        l->y++;
    case 15:
        l->x += l->dir * 2;
        if (!g_map->GetPixel(l->x, l->y))
            Message(l, MSG_FALL, 0, 0);
        x = l->x & ~3;
        y = l->y & ~3;
        if (CantMine(l, x, y)) {
            l->dir = -l->dir;
            at.x = l->x;
            at.y = l->yoff + l->y;
            SoundPlayFX(at, 6);
            Leave(l);
            g_walker.Enter(l);
        }
        /* falls into the odd frames, which do nothing */
    case 5: case 7: case 9: case 11: case 13:
    case 17: case 19: case 21: case 23:
        /* A statement here (removed later as a dead store) keeps VC++ 2.2 from dropping these
           labels from the jump table (its pruner drops break-only cases, skipping every second one);
           falling through from case 15 lets the empty block share the exit, as in the original. */
        x = 0;
        break;
    }
}

/* 0x10003df0 */
void Miner::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10003e60 */
void Miner::Leave(LEMMING *l)
{
}

/* 0x10003e70 */
void Miner::Enter(LEMMING *l)
{
    l->action = &g_miner;
    l->name = "Miner";
    l->type = LT_BUSY | LT_ASSIGNABLE;
    l->base = 0x11e;
    l->frames = 24;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -13;
}

/* 0x10003eb0 */
int Miner::Assign(LEMMING *l)
{
    int x;
    int y;

    if (!CanAssign(l, LT_ASSIGNABLE))
        return 0;
    x = l->x & ~3;
    y = l->y & ~3;
    if (CantMine(l, x, y))
        return 0;
    l->action->Leave(l);
    Enter(l);
    return 1;
}

/* 0x10003f10 */
void Miner::Init(int a, int b)
{
    leftFrames = 24;
}
