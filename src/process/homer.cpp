/* The homer: going in through the exit. */
#include "lemming.h"

/* 0x10002e50 */
void Homer::Process(LEMMING *l, DLENTRY **list)
{
    if (++l->frame == 7) {
        g_lemmsHome++;
        Leave(l);
        l->state |= LS_GONE;
    }
}

/* 0x10002e80 */
void Homer::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10002ef0: safe now */
static int HomerHandler(int msg, int a, int b, LEMMING *l)
{
    if (msg == MSG_COUNTDOWN)
        l->state &= ~LS_COUNTDOWN;
    return 1;
}

/* 0x10002f10 */
void Homer::Leave(LEMMING *l)
{
    RemoveHandler(l, HomerHandler);
}

/* 0x10002f50 */
void Homer::Enter(LEMMING *l)
{
    PNT at;

    l->action = &g_homer;
    l->name = "Homer";
    l->type = 2;
    l->base = 0x96;
    l->frames = 8;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -13;
    AddHandler(l, HomerHandler);
    at.x = l->x;
    at.y = l->y + l->yoff;
    SoundPlayFX(at, 2);
}

/* 0x10002ff0 */
int Homer::Assign(LEMMING *l)
{
    return 0;
}

/* 0x10003000 */
void Homer::Init(int a, int b)
{
    leftFrames = 0;
}
