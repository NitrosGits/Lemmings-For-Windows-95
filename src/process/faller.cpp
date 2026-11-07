/* The faller: falls until it lands, and splats if it fell too far. */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14, pad_top15, pad_top16, pad_top17, pad_top18, pad_top19;

#include "lemming.h"

/* 0x100023e0 */
void Faller::Process(LEMMING *l, DLENTRY **list)
{
    int x, y;

    if (++l->frame >= l->frames)
        l->frame = l->loop - l->frames + l->frame;
    l->count++;
    x = l->x;
    y = l->y + 1;
    if (!g_map->GetPixel(x, y)) {
        l->count++;
        y++;
        if (!g_map->GetPixel(x, y)) {
            l->count++;
            y++;
            if (!g_map->GetPixel(x, y)) {
                if (l->count >= 16)
                    Message(l, MSG_FLOAT, 0, 0);
                l->y = y;
                return;
            }
        }
    }
    l->y = y;
    Message(l, MSG_LAND, 0, 0);
}

/* 0x100024b0 */
void Faller::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10002520 */
static int FallerHandler(int msg, int a, int b, LEMMING *l)
{
    switch (msg) {
    case MSG_COUNTDOWN:
        l->action->Leave(l);
        g_exploder.Enter(l);
        return 1;
    }
    return 0;
}

/* 0x10002550 */
void Faller::Leave(LEMMING *l)
{
    RemoveHandler(l, FallerHandler);
}

/* 0x10002590 */
void Faller::Enter(LEMMING *l)
{
    l->action = &g_faller;
    l->name = "Faller";
    l->type = 0;
    l->base = 0x10;
    l->frames = 4;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -4;
    l->yoff = -10;
    if (!l->handlers)
        AddHandler(l, DefaultHandler);
    AddHandler(l, FallerHandler);
    l->count = 0;
}

/* 0x10002640 */
int Faller::Assign(LEMMING *l)
{
    return 0;
}

/* 0x10002650 */
void Faller::Init(int a, int b)
{
    leftFrames = 4;
}
