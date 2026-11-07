/*
 * The lemmings: releasing them from the entrances, moving and drawing them
 * each frame, finding the ones under the cursor and giving them skills.
 */
#include "lemming.h"

int g_armageddon = 0;           /* the nuke: no more "oh no"s */
int g_lemmsHome = 0;
int g_scale = 1;                /* 1, or 2 at high resolution */
int g_numLemmings = 0;          /* released so far */
int g_releaseCount = 0;         /* frames until the next is released */
int g_releaseDelay = 10;
int g_maxLemmings = 100;

BYTE *g_bigSprites;
BYTE *g_smallSprites;
BYTE *g_sprites;                /* the lemmings' RLE sprites at this scale */
LEMMING *g_lemmings[100];
DISPLAYLIST *g_list;            /* filled by this frame's moves, drawn next frame */
DISPLAYLIST *g_list2;
int g_listCount;                /* entries in g_list */
int g_hits[2];
HINSTANCE g_hinst;
Map *g_map;
int g_armageddonIndex;          /* the next to be nuked */

/* The actions, in their numbers' order */
Walker g_walker;
Faller g_faller;
Climber g_climber;
Flipper g_flipper;
Floater g_floater;
Flapper g_flapper;
Exploder g_exploder;
Splatter g_splatter;
Blocker g_blocker;
Drowner g_drowner;
Frier g_frier;
Homer g_homer;
Builder g_builder;
Shrugger g_shrugger;
Digger g_digger;
Basher g_basher;
Miner g_miner;

/* 0x100030d0 */
void WINAPI LibMain(HINSTANCE hinst)
{
    g_hinst = hinst;
}

/* 0x100030e0 */
void WINAPI SetSFXFlags(int sfx, int wave)
{
    g_sfxEnabled = sfx;
    g_waveEnabled = wave;
}

/* 0x10003100 */
void WINAPI SetNumLemmings(int count)
{
    g_maxLemmings = count;
}

/* 0x10003110 */
void WINAPI SetLemmingDelay(int delay)
{
    g_releaseDelay = delay;
}

/* 0x10003120 */
void WINAPI SetScale(int scale)
{
    g_scale = scale;
}

/* 0x10003130 */
int WINAPI GetLemmingDelay(void)
{
    return g_releaseDelay;
}

/* 0x10003140 */
int WINAPI GetLemmsHome(void)
{
    return g_lemmsHome;
}

/* 0x10003150: releases a lemming from the next entrance when it is time */
void WINAPI InitALemming(int x, int y)
{
    if (g_armageddonIndex == 0 && g_numLemmings != g_maxLemmings && --g_releaseCount <= 0) {
        GetEntrance(&x, &y);
        if (x != -1 && y != -1) {
            g_lemmings[g_numLemmings] = new LEMMING(x, y);
            g_lemmings[g_numLemmings]->skills = 0;
            g_numLemmings++;
            g_releaseCount = g_releaseDelay;
        }
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_processlemmings0, pad_processlemmings1, pad_processlemmings2, pad_processlemmings3;

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_processlemmings0, pad_processlemmings1, pad_processlemmings2, pad_processlemmings3, pad_processlemmings4, pad_processlemmings5, pad_processlemmings6, pad_processlemmings7, pad_processlemmings8, pad_processlemmings9, pad_processlemmings10, pad_processlemmings11, pad_processlemmings12, pad_processlemmings13, pad_processlemmings14, pad_processlemmings15, pad_processlemmings16, pad_processlemmings17, pad_processlemmings18, pad_processlemmings19;

/* 0x10003240: moves every lemming a frame; returns 0 when they are all gone */
int WINAPI ProcessLemmings(int *active)
{
    int i;
    int alive = 0;
    int gone = 0;
    DLENTRY *e, *start;

    e = start = &g_list->entries[g_listCount];
    for (i = 0; i < g_numLemmings; i++) {
        if (!(g_lemmings[i]->state & LS_GONE)) {
            g_lemmings[i]->CheckMap();
            g_lemmings[i]->CountDown();
            if (!(g_lemmings[i]->state & LS_GONE)) {
                alive++;
                g_lemmings[i]->action->Process(g_lemmings[i], &e);
            }
        } else {
            gone++;
        }
    }
    ProcessObjects();
    SoundProcess();
    g_listCount += e - start;
    *active = alive;
    return (!g_armageddonIndex && gone != g_maxLemmings) || (g_armageddonIndex && alive);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_getlemmingxy0, pad_getlemmingxy1, pad_getlemmingxy2, pad_getlemmingxy3, pad_getlemmingxy4, pad_getlemmingxy5, pad_getlemmingxy6, pad_getlemmingxy7, pad_getlemmingxy8, pad_getlemmingxy9, pad_getlemmingxy10;

/* 0x10003480: the positions of the lemmings still about */
int WINAPI GetLemmingXY(short *xy, int max)
{
    int n = 0;
    int i;

    for (i = 0; i < g_numLemmings; i++) {
        if (!(g_lemmings[i]->state & LS_GONE) && ++n < max) {
            *xy++ = (short)g_lemmings[i]->x;
            *xy++ = (short)g_lemmings[i]->y;
        }
    }
    return n;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_renderlemmings0, pad_renderlemmings1, pad_renderlemmings2, pad_renderlemmings3, pad_renderlemmings4, pad_renderlemmings5, pad_renderlemmings6, pad_renderlemmings7, pad_renderlemmings8, pad_renderlemmings9, pad_renderlemmings10, pad_renderlemmings11, pad_renderlemmings12;

/* 0x100034e0: draws the objects and lemmings, with the countdown over bombers */
void WINAPI RenderLemmings(WORD renderer, WORD slot)
{
    int n;
    DISPLAYLIST *list;
    LEMMING *l;
    DLENTRY *e;
    int i;

    for (i = 0; i < 0x1000; i++)
        g_list->entries[i].slot = slot;
    g_list->count = (WORD)g_listCount;
    RenderObjects(g_list);
    n = g_list->count;
    e = &g_list->entries[n];
    g_listCount = 0;
    for (i = 0; i < g_numLemmings; i++) {
        if (!(g_lemmings[i]->state & LS_GONE)) {
            g_lemmings[i]->action->Render(g_lemmings[i], e);
            n++;
            e++;
            l = g_lemmings[i];
            if (l->state & LS_COUNTDOWN) {
                e->type = DL_RLE;
                e->u.rle.mode = 1;
                e->u.rle.x = (short)((l->xoff + l->x) * g_scale);
                e->u.rle.y = (short)((l->y + l->yoff - 10) * g_scale);
                e->u.rle.sprites = g_sprites;
                e->u.rle.frame = (WORD)(l->countdown / 15 + SPR_COUNTDOWN);
                n++;
                e++;
            }
        }
    }
    g_list->count = (WORD)n;
    list = g_list;
    g_list = g_list2;
    g_list2 = list;
    Renderer_RenderList(renderer, list);
    SoundRender();
}

/* 0x10003610 */
static BYTE *LoadSprites(char *name)
{
    HANDLE file;
    DWORD size;
    BYTE *buf;

    file = FILE_ReadOpen(name);
    size = FILE_Length(file);
    buf = (BYTE *)VSMEM_Allocate(size, "Lemmings RLE Sprite Buffer");
    FILE_Read(file, buf, size);
    FILE_Close(file);
    return buf;
}

/* 0x10003650 */
void WINAPI InitialiseProcess(void)
{
    g_bigSprites = LoadSprites("gfx\\lemmbig.spr");
    g_smallSprites = LoadSprites("gfx\\lemmsma.spr");
    WaveInit();
}

/* 0x10003680 */
void WINAPI QuitProcess(void)
{
    if (g_bigSprites) {
        VSMEM_Free(g_bigSprites);
        g_bigSprites = NULL;
    }
    if (g_smallSprites) {
        VSMEM_Free(g_smallSprites);
        g_smallSprites = NULL;
    }
    WaveQuit();
}

/* 0x100036c0 */
void WINAPI InitialiseLemmings(WORD renderer, WORD slot, LEVEL *level)
{
    int i;

    g_sprites = g_scale == 2 ? g_bigSprites : g_smallSprites;
    for (i = 0; i < NUM_ACTIONS; i++)
        g_actions[i]->Init(0, 0);
    g_listCount = 0;
    g_list = NewDisplayList(0x1000);
    g_list2 = NewDisplayList(0x1000);
    g_list->count = 100;
    g_list2->count = 100;
    for (i = 0; i < 0x1000; i++) {
        g_list->entries[i].slot = slot;
        g_list2->entries[i].slot = slot;
    }
    g_releaseDelay = 10;
    g_numLemmings = 0;
    g_listCount = 0;
    g_lemmsHome = 0;
    g_releaseCount = 0;
    g_armageddonIndex = 0;
    g_armageddon = 0;
    InitObjects(renderer, level);
    SoundInit(renderer, slot);
}

/* 0x100037a0 */
void WINAPI DeleteLemmings(void)
{
    HANDLER *h;
    int i;
    HANDLER *next;
    LEMMING *l;

    for (i = 0; i < g_numLemmings; i++) {
        l = g_lemmings[i];
        if (l) {
            for (h = l->handlers; h; h = next) {
                next = h->next;
                delete h;
            }
            delete l;
        }
    }
    if (g_map) {
        delete g_map;
        g_map = NULL;
    }
    if (g_list)
        FreeDisplayList(g_list);
    if (g_list2)
        FreeDisplayList(g_list2);
    QuitObjects();
    SoundQuit();
}

/* 0x10003850 */
void WINAPI SetLevelMap(BYTE *pixels, int width, int height)
{
    Map *map;

    if (!g_map)
        map = new Map(pixels, width, height);
    else {
        g_map->SetMap(pixels, width, height);
        map = g_map;
    }
    g_map = map;
}

/* 0x100038c0: a climber, floater, bomber, blocker or builder can't be one twice */
static int CanTakeSkill(LEMMING *l, int action)
{
    switch (action) {
    case ACT_CLIMBER:
        if (l->skills & SK_CLIMBER)
            return 0;
        break;
    case ACT_FLOATER:
        if (l->skills & SK_FLOATER)
            return 0;
        break;
    case ACT_EXPLODER:
        if (l->state & LS_COUNTDOWN)
            return 0;
        break;
    case ACT_BLOCKER:
        if (l->skills & SK_BLOCKER)
            return 0;
        break;
    case ACT_BUILDER:
        if (l->skills & SK_BUILDER)
            return 0;
        break;
    }
    return 1;
}

/*
 * The lemmings under the cursor (facing the way the arrow keys say, if
 * held): one busy with another skill if priority is asked for, and the last
 * one found.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_findlemmings0, pad_findlemmings1, pad_findlemmings2, pad_findlemmings3, pad_findlemmings4, pad_findlemmings5, pad_findlemmings6, pad_findlemmings7, pad_findlemmings8, pad_findlemmings9, pad_findlemmings10, pad_findlemmings11, pad_findlemmings12, pad_findlemmings13, pad_findlemmings14, pad_findlemmings15, pad_findlemmings16, pad_findlemmings17, pad_findlemmings18, pad_findlemmings19, pad_findlemmings20, pad_findlemmings21, pad_findlemmings22, pad_findlemmings23, pad_findlemmings24, pad_findlemmings25, pad_findlemmings26, pad_findlemmings27, pad_findlemmings28, pad_findlemmings29, pad_findlemmings30, pad_findlemmings31, pad_findlemmings32, pad_findlemmings33, pad_findlemmings34, pad_findlemmings35, pad_findlemmings36, pad_findlemmings37, pad_findlemmings38, pad_findlemmings39, pad_findlemmings40, pad_findlemmings41, pad_findlemmings42, pad_findlemmings43, pad_findlemmings44, pad_findlemmings45, pad_findlemmings46, pad_findlemmings47, pad_findlemmings48, pad_findlemmings49, pad_findlemmings50, pad_findlemmings51, pad_findlemmings52, pad_findlemmings53, pad_findlemmings54, pad_findlemmings55, pad_findlemmings56, pad_findlemmings57, pad_findlemmings58, pad_findlemmings59, pad_findlemmings60, pad_findlemmings61, pad_findlemmings62, pad_findlemmings63, pad_findlemmings64, pad_findlemmings65, pad_findlemmings66, pad_findlemmings67, pad_findlemmings68, pad_findlemmings69, pad_findlemmings70, pad_findlemmings71, pad_findlemmings72, pad_findlemmings73, pad_findlemmings74, pad_findlemmings75, pad_findlemmings76, pad_findlemmings77, pad_findlemmings78, pad_findlemmings79, pad_findlemmings80, pad_findlemmings81, pad_findlemmings82, pad_findlemmings83, pad_findlemmings84, pad_findlemmings85, pad_findlemmings86, pad_findlemmings87;

/* 0x10003950 */
static int *FindLemmings(int x, int y, int action, int priority)
{
    int lx;
    int ly;
    int any;
    int left;
    int bottom;
    BYTE keys;
    int right;
    int top;
    int busy;
    int i;

    busy = -1;
    any = -1;
    g_hits[0] = -1;
    g_hits[1] = -1;
    left = x - g_scale * 4;
    right = x + g_scale * 4;
    top = y - g_scale * 4;
    bottom = y + g_scale * 4;
    keys = (GetAsyncKeyState(VK_LEFT) & 0x8000 ? 1 : 0) | (GetAsyncKeyState(VK_RIGHT) & 0x8000 ? 2 : 0);
    for (i = 0; i < g_numLemmings; i++) {
        if (g_lemmings[i]->state & LS_GONE)
            continue;
        if (keys && !((keys & 1) && g_lemmings[i]->dir == -1) && !((keys & 2) && g_lemmings[i]->dir == 1))
            continue;
        lx = g_lemmings[i]->x;
        ly = g_lemmings[i]->yoff / 2 + g_lemmings[i]->y;
        if (left <= lx && right >= lx && top <= ly && bottom >= ly) {
            if ((g_lemmings[i]->type & LT_BUSY) && priority && g_actions[action] != g_lemmings[i]->action)
                busy = i;
            else
                any = i;
        }
    }
    if (busy != -1) {
        g_hits[0] = busy;
        g_hits[1] = any;
    } else {
        g_hits[1] = -1;
        g_hits[0] = any;
    }
    return g_hits;
}

/* 0x10003aa0: the name of what the lemming under the cursor is doing */
char *WINAPI CheckLemming(int x, int y, int action, int priority)
{
    int *hits = FindLemmings(x, y, action, priority);
    int i;
    int h;

    for (i = 0; i < 2; hits++, i++) {
        h = *hits;
        if (h != -1 && g_lemmings[h]->action != g_actions[action])
            return g_lemmings[h]->name;
    }
    return NULL;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_makelemming0, pad_makelemming1;

/* 0x10003b00: gives the lemming under the cursor a skill */
int WINAPI MakeLemming(int x, int y, int action, int priority)
{
    int i;
    int *hits = FindLemmings(x, y, action, priority);
    int h;

    for (i = 0; i < 2; hits++, i++) {
        h = *hits;
        if (h != -1 && g_actions[action] != g_lemmings[h]->action && CanTakeSkill(g_lemmings[h], action))
            return g_actions[action]->Assign(g_lemmings[h]);
    }
    return 0;
}

/* 0x10003b80: lights the next lemming's fuse */
void WINAPI ProcessArmageddon(void)
{
    g_armageddon = 1;
    while (g_armageddonIndex != g_numLemmings && (g_lemmings[g_armageddonIndex]->state & LS_GONE))
        g_armageddonIndex++;
    if (g_armageddonIndex != g_numLemmings) {
        g_exploder.Assign(g_lemmings[g_armageddonIndex]);
        g_armageddonIndex++;
    }
}

/* 0x10003bf0 */
void *operator new(size_t size)
{
    return VSMEM_Allocate(size, "PROCESS: new");
}

/* 0x10003c00 */
void operator delete(void *p)
{
    VSMEM_Free(p);
}

#ifndef _MSC_VER
/* Newer compilers' array new and delete; Visual C++ 2.x used the plain ones */
void *operator new[](size_t size)
{
    return operator new(size);
}

void operator delete[](void *p)
{
    operator delete(p);
}
#endif
