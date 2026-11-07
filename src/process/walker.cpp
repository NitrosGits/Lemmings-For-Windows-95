/* The walker: walks, up steps of up to six pixels and down slopes. */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12;

#include "lemming.h"

/* 0x10004420 */
void Walker::Process(LEMMING *l, DLENTRY **list)
{
    int x, y;

    if (++l->frame >= l->frames)
        l->frame = l->loop - l->frames + l->frame;
    x = l->x += l->dir;
    y = l->y;
    if (g_map->GetPixel(x, y - 1)) {
        /* a step up, if no higher than six pixels */
        y--;
        if (g_map->GetPixel(x, y - 1)) {
            y--;
            if (g_map->GetPixel(x, y - 1)) {
                y--;
                if (g_map->GetPixel(x, y - 1)) {
                    y--;
                    if (g_map->GetPixel(x, y - 1)) {
                        y--;
                        if (g_map->GetPixel(x, y - 1)) {
                            y--;
                            if (g_map->GetPixel(x, y - 1)) {
                                        Message(l, MSG_WALL, 0, 0);
                                        return;
                            }
                        }
                    }
                }
            }
        }
        l->x = x;
        l->y = y;
        return;
    }
    /* down a slope of up to three pixels, or a fall */
    if (!g_map->GetPixel(x, y)) {
        y++;
        if (!g_map->GetPixel(x, y)) {
            y++;
            if (!g_map->GetPixel(x, y)) {
                y++;
                if (!g_map->GetPixel(x, y)) {
                    l->x = x;
                    l->y = y;
                    Message(l, MSG_FALL, 0, 0);
                    return;
                }
            }
        }
    }
    l->x = x;
    l->y = y;
}

/* 0x100045b0 */
void Walker::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10004620 */
void Walker::Leave(LEMMING *l)
{
}

/* 0x10004630 */
void Walker::Enter(LEMMING *l)
{
    l->action = &g_walker;
    l->name = "Walker";
    l->type = LT_ASSIGNABLE;
    l->base = 0;
    l->frames = 8;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -10;
    if (!l->handlers)
        AddHandler(l, DefaultHandler);
}

/* 0x100046b0 */
int Walker::Assign(LEMMING *l)
{
    return 0;
}

/* 0x100046c0 */
void Walker::Init(int a, int b)
{
    leftFrames = 8;
}
