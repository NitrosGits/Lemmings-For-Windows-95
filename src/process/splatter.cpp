/* The splatter: fell too far. */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5;

#include "lemming.h"

/* 0x10004250 */
void Splatter::Process(LEMMING *l, DLENTRY **list)
{
    if (++l->frame >= l->frames)
        l->frame = l->loop - l->frames + l->frame;
    if (l->frame == 15) {
        l->state |= LS_GONE;
        Leave(l);
    }
}

/* 0x10004290 */
void Splatter::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->yoff + l->y) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10004300 */
static int SplatterHandler(int msg, int a, int b, LEMMING *l)
{
    switch (msg) {
    case MSG_COUNTDOWN:
        l->state &= ~LS_COUNTDOWN;
        return 1;
    }
    return 0;
}

/* 0x10004320 */
void Splatter::Leave(LEMMING *l)
{
    RemoveHandler(l, SplatterHandler);
}

/* 0x10004360 */
void Splatter::Enter(LEMMING *l)
{
    PNT at;

    l->action = &g_splatter;
    l->name = "Splatter";
    l->type = 2;
    l->base = 0x58;
    l->frames = 16;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -10;
    AddHandler(l, SplatterHandler);
    at.x = l->x;
    at.y = l->y + l->yoff;
    SoundPlayFX(at, 3);
}

/* 0x10004400 */
int Splatter::Assign(LEMMING *l)
{
    return 0;
}

/* 0x10004410 */
void Splatter::Init(int a, int b)
{
    leftFrames = 0;
}
