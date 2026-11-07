/* The floater: drifts down under its umbrella. */
#include "lemming.h"

/* 0x10011298: the animation frame and the move (0 up a pixel, 1 down a
   pixel, 2-4 down that many pixels unless it lands) of each step; it loops
   from the 16th step back to the 8th */
signed char g_floatTable[32] = {
    1, 4, 2, 4, 3, 4, 5, 4, 5, 0, 5, 1, 5, 2, 5, 2,
    5, 3, 6, 3, 7, 3, 7, 3, 6, 3, 5, 3, 4, 3, 4, 3,
};

/* 0x10002a30 */
void Floater::Process(LEMMING *l, DLENTRY **list)
{
    int x, y, move;
    int step = l->step;

    move = g_floatTable[step * 2 + 1];
    l->frame = g_floatTable[step * 2];
    if (step == 15)
        step = 7;
    l->step = step + 1;
    x = l->x;
    y = l->y;
    switch (move) {
    case 0:
        y--;
        break;
    case 4:
        if (g_map->GetPixel(x, y)) {
            Message(l, MSG_LAND, 0, 0);
            break;
        }
        y++;
    case 3:
        if (g_map->GetPixel(x, y)) {
            Message(l, MSG_LAND, 0, 0);
            break;
        }
        y++;
    case 2:
        if (g_map->GetPixel(x, y)) {
            Message(l, MSG_LAND, 0, 0);
            break;
        }
    case 1:
        y++;
        break;
    }
    l->x = x;
    l->y = y;
}

/* 0x10002b50 */
void Floater::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10002bc0 */
void Floater::Leave(LEMMING *l)
{
}

/* 0x10002bd0 */
void Floater::Enter(LEMMING *l)
{
    l->action = &g_floater;
    l->name = "Floater";
    l->type = 0;
    l->base = 0x38;
    l->frames = 8;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -16;
    l->step = 0;
}

/* 0x10002c10: opens the umbrella once a floater has fallen far enough */
static int FloaterHandler(int msg, int a, int b, LEMMING *l)
{
    switch (msg) {
    case MSG_FLOAT:
        l->count = 0;
        l->action->Leave(l);
        g_floater.Enter(l);
        return 1;
    }
    return 0;
}

/* 0x10002c50 */
int Floater::Assign(LEMMING *l)
{
    l->skills |= SK_FLOATER;
    return AddHandler(l, FloaterHandler);
}

/* 0x10002ca0 */
void Floater::Init(int a, int b)
{
    leftFrames = 8;
}
