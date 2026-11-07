/* The flapper: "oh no!" - stands still flapping, then explodes. */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1;

#include "lemming.h"

/* 0x10002660 */
void Flapper::Process(LEMMING *l, DLENTRY **list)
{
    int x, y;
    PNT at;

    if (++l->frame == 15) {
        if (!g_armageddon) {
            at.x = l->x;
            at.y = l->yoff + l->y;
            SoundPlayFX(at, 4);
        }
        Leave(l);
        g_exploder.Enter(l);
        return;
    }
    x = l->x;
    y = l->y;
    if (!g_map->GetPixel(x, y) && !g_map->GetPixel(x, ++y) && !g_map->GetPixel(x, ++y))
        y++;
    l->x = x;
    l->y = y;
}

/* 0x10002700 */
void Flapper::Render(LEMMING *l, DLENTRY *e)
{
    e->type = DL_RLE;
    e->u.rle.mode = 1;
    e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
    e->u.rle.y = (short)((l->yoff + l->y) * g_scale);
    e->u.rle.sprites = g_sprites;
    e->u.rle.frame = (WORD)(l->frame + l->base + (l->dir == -1 ? l->action->leftFrames : 0));
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_flapperleave0, pad_flapperleave1, pad_flapperleave2, pad_flapperleave3, pad_flapperleave4, pad_flapperleave5, pad_flapperleave6, pad_flapperleave7, pad_flapperleave8, pad_flapperleave9, pad_flapperleave10, pad_flapperleave11, pad_flapperleave12, pad_flapperleave13, pad_flapperleave14, pad_flapperleave15, pad_flapperleave16, pad_flapperleave17, pad_flapperleave18, pad_flapperleave19, pad_flapperleave20, pad_flapperleave21, pad_flapperleave22, pad_flapperleave23, pad_flapperleave24, pad_flapperleave25, pad_flapperleave26, pad_flapperleave27, pad_flapperleave28, pad_flapperleave29, pad_flapperleave30, pad_flapperleave31, pad_flapperleave32, pad_flapperleave33, pad_flapperleave34, pad_flapperleave35, pad_flapperleave36, pad_flapperleave37, pad_flapperleave38, pad_flapperleave39, pad_flapperleave40, pad_flapperleave41, pad_flapperleave42, pad_flapperleave43, pad_flapperleave44, pad_flapperleave45, pad_flapperleave46, pad_flapperleave47, pad_flapperleave48, pad_flapperleave49, pad_flapperleave50, pad_flapperleave51, pad_flapperleave52, pad_flapperleave53, pad_flapperleave54, pad_flapperleave55, pad_flapperleave56, pad_flapperleave57, pad_flapperleave58, pad_flapperleave59;

/* 0x10002770 */
void Flapper::Leave(LEMMING *l)
{
    int i;
    int j;
    int x;
    int y;
    int *cell;

    y = l->y - 6;
    cell = (int *)&l->count;
    for (j = 0; j < 3; j++, y += 4)
        for (i = 0, x = l->x - 4; i < 3; i++, x += 4)
            g_map->SetObjectAt(x, y, cell[j * 3 + i]);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_flapperenter0, pad_flapperenter1, pad_flapperenter2, pad_flapperenter3, pad_flapperenter4, pad_flapperenter5, pad_flapperenter6, pad_flapperenter7, pad_flapperenter8, pad_flapperenter9, pad_flapperenter10, pad_flapperenter11, pad_flapperenter12, pad_flapperenter13, pad_flapperenter14, pad_flapperenter15, pad_flapperenter16, pad_flapperenter17, pad_flapperenter18, pad_flapperenter19, pad_flapperenter20, pad_flapperenter21, pad_flapperenter22, pad_flapperenter23, pad_flapperenter24, pad_flapperenter25, pad_flapperenter26, pad_flapperenter27, pad_flapperenter28, pad_flapperenter29, pad_flapperenter30, pad_flapperenter31, pad_flapperenter32, pad_flapperenter33, pad_flapperenter34, pad_flapperenter35, pad_flapperenter36, pad_flapperenter37, pad_flapperenter38, pad_flapperenter39, pad_flapperenter40, pad_flapperenter41, pad_flapperenter42, pad_flapperenter43, pad_flapperenter44, pad_flapperenter45, pad_flapperenter46, pad_flapperenter47, pad_flapperenter48, pad_flapperenter49, pad_flapperenter50, pad_flapperenter51, pad_flapperenter52, pad_flapperenter53, pad_flapperenter54, pad_flapperenter55, pad_flapperenter56, pad_flapperenter57, pad_flapperenter58, pad_flapperenter59, pad_flapperenter60, pad_flapperenter61, pad_flapperenter62, pad_flapperenter63, pad_flapperenter64, pad_flapperenter65, pad_flapperenter66, pad_flapperenter67, pad_flapperenter68, pad_flapperenter69, pad_flapperenter70, pad_flapperenter71, pad_flapperenter72, pad_flapperenter73, pad_flapperenter74, pad_flapperenter75, pad_flapperenter76, pad_flapperenter77, pad_flapperenter78, pad_flapperenter79, pad_flapperenter80, pad_flapperenter81, pad_flapperenter82, pad_flapperenter83, pad_flapperenter84, pad_flapperenter85, pad_flapperenter86, pad_flapperenter87;

/* 0x100027d0: stands as a blocker while it flaps */
void Flapper::Enter(LEMMING *l)
{
    int x;
    int y;
    int *cell;
    int i;
    int j;

    l->action = &g_flapper;
    l->name = "Flapper";
    l->type = 2;
    l->base = 0x48;
    l->frames = 16;
    l->loop = 0;
    l->frame = 0;
    l->xoff = -8;
    l->yoff = -10;
    l->state &= ~LS_COUNTDOWN;
    y = l->y - 6;
    cell = (int *)&l->count;
    for (j = 0; j < 3; j++, y += 4)
        for (i = 0, x = l->x - 4; i < 3; i++, x += 4)
            cell[j * 3 + i] = g_map->GetObjectAt(x, y);
    for (j = 0, y = l->y - 6; j < 3; j++, y += 4) {
        x = l->x - 4;
        g_map->SetObjectAt(x, y, OBJ_TURNLEFT);
        x += 4;
        g_map->SetObjectAt(x, y, OBJ_BLOCKER);
        x += 4;
        g_map->SetObjectAt(x, y, OBJ_TURNRIGHT);
    }
}

/* 0x100028c0 */
int Flapper::Assign(LEMMING *l)
{
    return 0;
}

/* 0x100028d0 */
void Flapper::Init(int a, int b)
{
    leftFrames = 0;
}
