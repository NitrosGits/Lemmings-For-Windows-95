/* The shrugger: a builder out of bricks. */
#include "lemming.h"

/* 0x10004140 */
void Shrugger::Process(LEMMING *l, DLENTRY **list)
{
    if (++l->frame == 8) {
        Leave(l);
        g_walker.Enter(l);
    }
}

/* 0x10004170 */
void Shrugger::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x100041e0 */
void Shrugger::Leave(LEMMING *l)
{
}

/* 0x100041f0 */
void Shrugger::Enter(LEMMING *l)
{
    l->action = &g_shrugger;
    l->name = "Shrugger";
    l->type = LT_BUSY | LT_ASSIGNABLE;
    l->base = 0xbe;
    l->frames = 8;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -13;
}

/* 0x10004230 */
int Shrugger::Assign(LEMMING *l)
{
    return 0;
}

/* 0x10004240 */
void Shrugger::Init(int a, int b)
{
    leftFrames = 8;
}
