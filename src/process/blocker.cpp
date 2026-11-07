/* The blocker: stands still and turns the others round. */
#include "lemming.h"

/* 0x10001380 */
void Blocker::Process(LEMMING *l, DLENTRY **list)
{
    if (++l->frame >= l->frames)
        l->frame = l->loop - l->frames + l->frame;
    if (!g_map->GetPixel(l->x, l->y))
        Message(l, MSG_FALL, 0, 0);
}

/* 0x100013e0 */
void Blocker::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->y + l->yoff) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* 0x10001450: a blocker ignores objects */
static int BlockerHandler(int msg, int a, int b, LEMMING *l)
{
    switch (msg) {
    case MSG_OBJECT:
        return 1;
    }
    return 0;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_blockerleave0, pad_blockerleave1, pad_blockerleave2, pad_blockerleave3, pad_blockerleave4, pad_blockerleave5, pad_blockerleave6, pad_blockerleave7, pad_blockerleave8, pad_blockerleave9, pad_blockerleave10, pad_blockerleave11, pad_blockerleave12, pad_blockerleave13, pad_blockerleave14, pad_blockerleave15, pad_blockerleave16, pad_blockerleave17, pad_blockerleave18, pad_blockerleave19, pad_blockerleave20, pad_blockerleave21, pad_blockerleave22, pad_blockerleave23, pad_blockerleave24, pad_blockerleave25, pad_blockerleave26, pad_blockerleave27, pad_blockerleave28, pad_blockerleave29, pad_blockerleave30, pad_blockerleave31, pad_blockerleave32, pad_blockerleave33, pad_blockerleave34, pad_blockerleave35, pad_blockerleave36, pad_blockerleave37, pad_blockerleave38, pad_blockerleave39, pad_blockerleave40, pad_blockerleave41, pad_blockerleave42, pad_blockerleave43, pad_blockerleave44, pad_blockerleave45, pad_blockerleave46, pad_blockerleave47, pad_blockerleave48, pad_blockerleave49, pad_blockerleave50, pad_blockerleave51, pad_blockerleave52, pad_blockerleave53, pad_blockerleave54, pad_blockerleave55, pad_blockerleave56, pad_blockerleave57, pad_blockerleave58, pad_blockerleave59, pad_blockerleave60, pad_blockerleave61, pad_blockerleave62, pad_blockerleave63, pad_blockerleave64, pad_blockerleave65, pad_blockerleave66, pad_blockerleave67, pad_blockerleave68, pad_blockerleave69, pad_blockerleave70, pad_blockerleave71;

/* 0x10001470 */
void Blocker::Leave(LEMMING *l)
{
    int i;
    int y;
    int *cell;
    int j;
    int x;

    l->skills &= ~SK_BLOCKER;
    RemoveHandler(l, BlockerHandler);
    y = l->y - 6;
    cell = (int *)&l->count;
    for (j = 0; j < 3; j++, y += 4)
        for (i = 0, x = l->x - 4; i < 3; i++, x += 4)
            g_map->SetObjectAt(x, y, cell[j * 3 + i]);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_blockerenter0;

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_blockerenter0, pad_blockerenter1, pad_blockerenter2, pad_blockerenter3, pad_blockerenter4, pad_blockerenter5, pad_blockerenter6, pad_blockerenter7, pad_blockerenter8, pad_blockerenter9, pad_blockerenter10, pad_blockerenter11, pad_blockerenter12, pad_blockerenter13, pad_blockerenter14, pad_blockerenter15, pad_blockerenter16, pad_blockerenter17, pad_blockerenter18, pad_blockerenter19, pad_blockerenter20, pad_blockerenter21, pad_blockerenter22, pad_blockerenter23, pad_blockerenter24;

/* 0x10001510 */
void Blocker::Enter(LEMMING *l)
{
    int i;
    int *cell;
    int j;
    int y;
    int x;

    l->action = &g_blocker;
    l->name = "Blocker";
    l->type = LT_BUSY;
    l->base = 0x68;
    l->frames = 16;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -10;
    AddHandler(l, BlockerHandler);
    l->skills |= SK_BLOCKER;
    for (j = 0, y = l->y - 6, cell = (int *)&l->count; j < 3; j++, y += 4)
        for (i = 0, x = l->x - 4; i < 3; i++, x += 4)
            *cell++ = g_map->GetObjectAt(x, y);
    for (j = 0, y = l->y - 6; j < 3; j++, y += 4) {
        x = l->x - 4;
        g_map->SetObjectAt(x, y, OBJ_TURNLEFT);
        x += 4;
        g_map->SetObjectAt(x, y, OBJ_BLOCKER);
        x += 4;
        g_map->SetObjectAt(x, y, OBJ_TURNRIGHT);
    }
}

/* 0x10001640 */
int Blocker::Assign(LEMMING *l)
{
    if (!CanAssign(l, LT_ASSIGNABLE))
        return 0;
    l->action->Leave(l);
    Enter(l);
    return 1;
}

/* 0x10001680 */
void Blocker::Init(int a, int b)
{
    leftFrames = 0;
}
