/* The exploder: the bomb goes off, leaving a hole unless it is on steel. */
#include "lemming.h"

extern short g_explosion[53][80][2];

/* 0x10002270 */
void Exploder::Process(LEMMING *l, DLENTRY **list)
{
    PNT at;

    switch (++l->frame) {
    case 1:
        at.x = l->x;
        at.y = l->yoff + l->y;
        SoundPlayFX(at, 13);
        if ((BYTE)g_map->GetObjectAt(l->x, l->y) != OBJ_STEEL)
            g_map->EraseSprite(list, g_sprites, SPR_EXPLOSION, l->x - 8, l->y - 14);
        break;
    case 53:
        l->state |= LS_GONE;
        Leave(l);
        break;
    }
}

/* 0x10002300 */
void Exploder::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_PIXELS;
    e->u.pixels.x = (short)(l->x * g_scale);
    e->u.pixels.y = (short)(l->y * g_scale);
    e->u.pixels.count = 80;
    e->u.pixels.points = g_explosion[l->frame][0];
    e->u.pixels.colour = 0;
    e->u.pixels.colours = 16;
    e->u.pixels.size = 1;
}

/* 0x10002360 */
void Exploder::Leave(LEMMING *l)
{
}

/* 0x10002370 */
void Exploder::Enter(LEMMING *l)
{
    l->action = &g_exploder;
    l->name = "Exploder";
    l->type = 2;
    l->frames = 53;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -16;
    l->yoff = -25;
    l->state &= ~LS_COUNTDOWN;
}

/* 0x100023b0: starts the bomb countdown */
int Exploder::Assign(LEMMING *l)
{
    l->state |= LS_COUNTDOWN;
    l->countdown = 74;
    return 1;
}

/* 0x100023d0 */
void Exploder::Init(int a, int b)
{
}
