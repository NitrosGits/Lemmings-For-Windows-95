/* The climber: goes up walls, and is a climber from then on. */
#include "lemming.h"

/* 0x10001940 */
void Climber::Process(LEMMING *l, DLENTRY **list)
{
    int y;
    int x;

    if (++l->frame >= l->frames)
        l->frame = l->loop - l->frames + l->frame;
    if (l->frame >= 4) {
        l->y--;
        y = l->y - 8;
        x = l->x - l->dir;
        if (g_map->GetPixel(x, y))
            Message(l, MSG_OVERHANG, 0, 0);
        return;
    }
    if (!g_map->GetPixel(l->x, l->y - l->frame - 7)) {
        l->y -= 6;
        l->y -= l->frame;
        Leave(l);
        g_flipper.Enter(l);
    }
}

/* 0x10001a00 */
void Climber::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10001a70 */
void Climber::Leave(LEMMING *l)
{
}

/* 0x10001a80 */
void Climber::Enter(LEMMING *l)
{
    l->action = &g_climber;
    l->name = "Climber";
    l->type = 4;
    l->base = 0x18;
    l->frames = 8;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -12;
}

/* 0x10001ac0: turns a walker that meets a wall into a climber */
static int ClimberHandler(int msg, int a, int b, LEMMING *l)
{
    switch (msg) {
    case MSG_WALL:
        l->action->Leave(l);
        g_climber.Enter(l);
        return 1;
    case MSG_OVERHANG:
        l->dir = -l->dir;
        l->x += l->dir * 2;
        l->action->Leave(l);
        g_faller.Enter(l);
        return 1;
    case MSG_COUNTDOWN:
        if (l->action == &g_climber) {
            l->action->Leave(l);
            g_exploder.Enter(l);
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x10001b50 */
int Climber::Assign(LEMMING *l)
{
    l->skills |= SK_CLIMBER;
    return AddHandler(l, ClimberHandler);
}

/* 0x10001ba0 */
void Climber::Init(int a, int b)
{
    leftFrames = 8;
}
