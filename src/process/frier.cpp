/* The frier: burnt up by a fire trap. */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6;

#include "lemming.h"

/* 0x10002cb0 */
void Frier::Process(LEMMING *l, DLENTRY **list)
{
    if (++l->frame == 14) {
        Leave(l);
        l->state |= LS_GONE;
    }
}

/* 0x10002cd0 */
void Frier::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->yoff + l->y) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10002d40: too late for the bomb */
static int FrierHandler(int msg, int a, int b, LEMMING *l)
{
    if (msg == MSG_COUNTDOWN)
        l->state &= ~LS_COUNTDOWN;
    return 1;
}

/* 0x10002d60 */
void Frier::Leave(LEMMING *l)
{
    RemoveHandler(l, FrierHandler);
}

/* 0x10002da0 */
void Frier::Enter(LEMMING *l)
{
    PNT at;

    l->action = &g_frier;
    l->name = "Frier";
    l->type = 2;
    l->base = 0x88;
    l->frames = 7;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -14;
    AddHandler(l, FrierHandler);
    at.x = l->x;
    at.y = l->y;
    SoundPlayFX(at, 14);
}

/* 0x10002e30 */
int Frier::Assign(LEMMING *l)
{
    return 0;
}

/* 0x10002e40 */
void Frier::Init(int a, int b)
{
    leftFrames = 0;
}
