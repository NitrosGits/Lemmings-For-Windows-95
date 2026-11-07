/* The flipper: a climber hauling itself over the top. */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5;

#include "lemming.h"

/* 0x100028e0 */
void Flipper::Process(LEMMING *l, DLENTRY **list)
{
    if (!g_map->GetPixel(l->x, l->y)) {
        Message(l, MSG_FALL, 0, 0);
        return;
    }
    if (++l->frame == l->frames) {
        Leave(l);
        g_walker.Enter(l);
    }
}

/* 0x10002950 */
void Flipper::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->yoff + l->y) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x100029c0 */
void Flipper::Leave(LEMMING *l)
{
}

/* 0x100029d0 */
void Flipper::Enter(LEMMING *l)
{
    l->action = &g_flipper;
    l->name = "Flipper";
    l->base = 0x28;
    l->type = LT_ASSIGNABLE;
    l->frames = 8;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -9;
}

/* 0x10002a10 */
int Flipper::Assign(LEMMING *l)
{
    return 0;
}

/* 0x10002a20 */
void Flipper::Init(int a, int b)
{
    leftFrames = 8;
}
