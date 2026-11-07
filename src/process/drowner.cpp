/* The drowner: thrashes about in the water, then is gone. */
#include "lemming.h"

/* 0x10002080 */
void Drowner::Process(LEMMING *l, DLENTRY **list)
{
    int y;
    int x;

    if (++l->frame == 15) {
        Leave(l);
        l->state |= LS_GONE;
    }
    y = l->y;
    x = l->x + l->dir * 8;
    if (!g_map->GetPixel(x, y))
        l->x += l->dir;
}

/* 0x100020c0 */
void Drowner::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10002130: a drowning lemming takes no notice of anything but its bomb */
static int DrownerHandler(int msg, int a, int b, LEMMING *l)
{
    if (msg != MSG_COUNTDOWN)
        return 1;
    l->action->Leave(l);
    g_exploder.Enter(l);
    return 1;
}

/* 0x10002170 */
void Drowner::Leave(LEMMING *l)
{
    RemoveHandler(l, DrownerHandler);
}

/* 0x100021b0 */
void Drowner::Enter(LEMMING *l)
{
    PNT at;

    l->action = &g_drowner;
    l->name = "Drowner";
    l->type = 8;
    l->base = 0x78;
    l->frames = 16;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -10;
    AddHandler(l, DrownerHandler);
    at.x = l->x;
    at.y = l->y + l->yoff;
    SoundPlayFX(at, 5);
}

/* 0x10002250 */
int Drowner::Assign(LEMMING *l)
{
    return 0;
}

/* 0x10002260 */
void Drowner::Init(int a, int b)
{
    leftFrames = 0;
}
