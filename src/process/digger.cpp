/* The digger: digs straight down. */
#include "lemming.h"

/* 0x10001df0 */
static int IsSteel(LEMMING *l, int x, int y)
{
    return (g_map->GetObjectAt(x, y) & 0xff) == OBJ_STEEL;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_diggerprocess0, pad_diggerprocess1, pad_diggerprocess2, pad_diggerprocess3, pad_diggerprocess4, pad_diggerprocess5, pad_diggerprocess6, pad_diggerprocess7, pad_diggerprocess8, pad_diggerprocess9, pad_diggerprocess10;

/* 0x10001e20 */
void Digger::Process(LEMMING *l, DLENTRY **list)
{
    int x, y;
    PNT at;

    if (++l->frame >= l->frames)
        l->frame = l->loop - l->frames + l->frame;
    if (l->frame == 0 || l->frame == 8) {
        g_map->EraseSprite(list, g_sprites, SPR_DIGHOLE, l->xoff + l->x, l->y - 2);
        x = l->x & ~3;
        y = l->y & ~3;
        if (IsSteel(l, x, y)) {
            at.x = l->x;
            at.y = l->yoff + l->y;
            SoundPlayFX(at, 6);
            Leave(l);
            g_walker.Enter(l);
            return;
        }
        x = l->x;
        y = ++l->y;
        if (!g_map->GetPixel(x, y) && !g_map->GetPixel(x, y + 1) && !g_map->GetPixel(x, y + 2) &&
            !g_map->GetPixel(x, y + 3)) {
            Leave(l);
            g_faller.Enter(l);
        }
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_diggerrender0, pad_diggerrender1, pad_diggerrender2;

/* 0x10001f50 */
void Digger::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10001fc0 */
void Digger::Leave(LEMMING *l)
{
}

/* 0x10001fd0 */
void Digger::Enter(LEMMING *l)
{
    l->action = &g_digger;
    l->name = "Digger";
    l->type = LT_BUSY | LT_ASSIGNABLE;
    l->base = 0xce;
    l->frames = 16;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -12;
}

/* 0x10002010 */
int Digger::Assign(LEMMING *l)
{
    int x;
    int y;

    if (!CanAssign(l, LT_ASSIGNABLE))
        return 0;
    x = l->x & ~3;
    y = l->y & ~3;
    if (IsSteel(l, x, y))
        return 0;
    l->action->Leave(l);
    Enter(l);
    return 1;
}

/* 0x10002070 */
void Digger::Init(int a, int b)
{
    leftFrames = 0;
}
