/* The builder: lays twelve bricks, a step up each. */
#include "lemming.h"

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_builderprocess0, pad_builderprocess1, pad_builderprocess2;

/* 0x10001690 */
void Builder::Process(LEMMING *l, DLENTRY **list)
{
    int x, y;
    PNT at;
    int n;
    int f;

    f = ++l->frame;
    n = l->frames;
    if (f >= n)
        l->frame = l->loop - n + f;
    switch (l->frame) {
    case 0:
        x = l->x + l->dir;
        y = l->y - 1;
        l->x = x;
        l->y = y;
        /* `l->dir ? 1 : 1` (always 1) makes VC++ 2.2 load l->dir into a register before the add,
           so the second y - 1 is renamed to ecx as in the original (folded back to add ebx,[mem]). */
        if (g_map->GetPixel(x, y - 1) || g_map->GetPixel(x += l->dir, y - (l->dir ? 1 : 1))) {
            l->x = x;
            l->y = y;
            l->dir = -l->dir;
            Leave(l);
            g_walker.Enter(l);
            return;
        }
        if (--l->count == 0) {
            l->x = x;
            l->y = y;
            Leave(l);
            g_shrugger.Enter(l);
            return;
        }
        if (g_map->GetPixel(x + l->dir * 2, y - 9)) {
            l->x = x;
            l->y = y;
            l->dir = -l->dir;
            Leave(l);
            g_walker.Enter(l);
            return;
        }
        l->x = x;
        l->y = y;
        break;
    case 9:
        x = l->xoff + l->x;
        y = l->y - 1;
        if (y >= 0) {
            if (l->dir == -1)
                x -= 4;
            g_map->AddSprite(list, g_sprites, SPR_BRICK, x, y);
        }
        break;
    case 10:
        if (l->count <= 3) {
            at.x = l->x;
            at.y = l->y + l->yoff;
            SoundPlayFX(at, 0);
        }
        break;
    }
}

/* 0x10001820 */
void Builder::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10001890 */
void Builder::Leave(LEMMING *l)
{
    l->skills &= ~SK_BUILDER;
}

/* 0x100018a0 */
void Builder::Enter(LEMMING *l)
{
    l->action = &g_builder;
    l->name = "Builder";
    l->type = LT_BUSY | LT_ASSIGNABLE;
    l->base = 0x9e;
    l->frames = 16;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -10;
    l->yoff = -13;
    l->count = 12;
    l->skills |= SK_BUILDER;
}

/* 0x100018f0 */
int Builder::Assign(LEMMING *l)
{
    if (!CanAssign(l, LT_ASSIGNABLE))
        return 0;
    l->action->Leave(l);
    Enter(l);
    return 1;
}

/* 0x10001930 */
void Builder::Init(int a, int b)
{
    leftFrames = 16;
}
