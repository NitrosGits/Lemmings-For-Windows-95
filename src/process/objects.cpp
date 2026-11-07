/*
 * The level's objects - entrances, the exit, traps, water, fire - as the
 * level describes them: where they are, their animation, and the effect
 * they put in the map's object grid.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14, pad_top15, pad_top16, pad_top17, pad_top18, pad_top19, pad_top20, pad_top21, pad_top22, pad_top23, pad_top24;

#include "lemming.h"

#define OF_ACTIVE       1       /* an entrance opening, a trap springing */
#define OF_ENTRANCE     2
#define OF_TRAP         4
#define OF_REDRAW       0x8000  /* drawn once more over the terrain */

struct OBJECT;

/* How a kind of object animates and draws */
class ObjectClass {
public:
    virtual void Process(OBJECT *o);
    virtual void Render(OBJECT *o, DISPLAYLIST *list);
};

class EntranceClass : public ObjectClass {
public:
    void Process(OBJECT *o);
    void Render(OBJECT *o, DISPLAYLIST *list);
};

class TrapClass : public ObjectClass {
public:
    void Process(OBJECT *o);
    void Render(OBJECT *o, DISPLAYLIST *list);
};

struct OBJECT {
    int x, y;                   /* 0x00 */
    int width, height;          /* 0x08 */
    int frame;                  /* 0x10 */
    int frames;                 /* 0x14 */
    int sprite;                 /* 0x18: its first frame in the sprite file */
    int flags;                  /* 0x1c: OF_ */
    int counter;                /* 0x20: an entrance's frames until it opens */
    int sound;                  /* 0x24: a trap's sound */
    BYTE *sprites;              /* 0x28 */
    WORD mode;                  /* 0x2c: the display list mode */
    ObjectClass *cls;           /* 0x30 */
};

TrapClass g_trap;
int g_numEntrances;
int g_numObjects;
OBJECT *g_entrances[64];
ObjectClass g_object;
int g_nextEntrance;             /* the entrance the next lemming comes out of */
EntranceClass g_entrance;
OBJECT g_objects[64];

/* 0x10005710 */
static void AddToList(OBJECT *o, DISPLAYLIST *list)
{
    DLENTRY *e = &list->entries[list->count];

    e->type = DL_RLE;
    if (o->flags & OF_REDRAW) {
        e->u.rle.mode = 3;
        o->flags &= ~OF_REDRAW;
    } else {
        e->u.rle.mode = o->mode;
    }
    e->u.rle.x = (short)(o->x * g_scale);
    e->u.rle.y = (short)(o->y * g_scale);
    e->u.rle.sprites = o->sprites;
    e->u.rle.frame = (WORD)(o->sprite + o->frame);
    list->count++;
}

/* 0x10005780: animates for ever */
void ObjectClass::Process(OBJECT *o)
{
    if (++o->frame >= o->frames)
        o->frame = 0;
}

/* 0x100057a0 */
void ObjectClass::Render(OBJECT *o, DISPLAYLIST *list)
{
    AddToList(o, list);
}

/* 0x100057c0: opens once, after a pause and a sound */
void EntranceClass::Process(OBJECT *o)
{
    PNT at;

    if (o->flags & OF_ACTIVE) {
        if (o->counter != 10) {
            if (++o->counter == 1) {
                at.x = o->x + o->width / 2;
                at.y = o->y + 40;
                SoundPlayFX(at, 1);
            }
            return;
        }
        if (++o->frame >= o->frames) {
            o->frame = 0;
            o->flags &= ~OF_ACTIVE;
            o->flags |= OF_REDRAW;
        }
    }
}

/* 0x10005840 */
void EntranceClass::Render(OBJECT *o, DISPLAYLIST *list)
{
    if (o->flags & (OF_ACTIVE | OF_REDRAW))
        AddToList(o, list);
}

/* 0x10005860: animates once each time it is sprung */
void TrapClass::Process(OBJECT *o)
{
    if (o->flags & OF_ACTIVE) {
        if (++o->frame >= o->frames) {
            o->frame = 0;
            o->flags &= ~OF_ACTIVE;
        }
    }
}

/* 0x10005890 */
void TrapClass::Render(OBJECT *o, DISPLAYLIST *list)
{
    AddToList(o, list);
}

/* 0x100058b0 */
static void InitObject(OBJECT *o, LEVELOBJECT *src, int index, BYTE *sprites)
{
    o->x = src->x;
    o->y = src->y;
    o->frame = src->startFrame;
    o->frames = src->endFrame;
    o->sprite = src->sprite;
    o->sprites = sprites;
    o->width = src->width;
    o->height = src->height;
    o->flags = src->animType;
    o->mode = src->drawMode;
    o->sound = src->sound;
    o->counter = 0;
    o->cls = &g_object;
    if (src->animType & OF_ENTRANCE) {
        o->cls = &g_entrance;
        g_entrances[g_numEntrances++] = o;
    } else if (src->animType & OF_TRAP) {
        o->cls = &g_trap;
    }
    g_map->FillObjects(src->trigLeft + o->x, src->trigTop + o->y, src->trigWidth, src->trigHeight,
                       src->trigEffect | index << 8);
}

/* 0x10005970 */
static void InitSteel(LEVEL *level)
{
    int i;

    for (i = 0; i < level->numSteel; i++)
        g_map->FillObjects(level->steel[i].x, level->steel[i].y, level->steel[i].width,
                           level->steel[i].height, level->steel[i].type);
}

/* 0x100059b0 */
static void InitAllObjects(LEVEL *level)
{
    int i;

    g_numObjects = level->numObjects;
    for (i = 0; i < g_numObjects; i++)
        InitObject(&g_objects[i], &level->objects[i], i, level->objectSprites);
}

/* 0x10005a00 */
void InitObjects(WORD renderer, LEVEL *level)
{
    g_numObjects = 0;
    g_numEntrances = 0;
    g_nextEntrance = 0;
    InitSteel(level);
    InitAllObjects(level);
}

/* 0x10005a30 */
void QuitObjects(void)
{
    int i;

    for (i = 0; i < g_numObjects; i++) {
        g_objects[i].x = -1;
        g_objects[i].y = -1;
        g_objects[i].frame = -1;
        g_objects[i].frames = -1;
        g_objects[i].cls = NULL;
    }
    g_numObjects = 0;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_processobjects0, pad_processobjects1, pad_processobjects2, pad_processobjects3, pad_processobjects4, pad_processobjects5;

/* 0x10005a70 */
void ProcessObjects(void)
{
    int i;

    for (i = 0; i < g_numObjects; i++)
        g_objects[i].cls->Process(&g_objects[i]);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_renderobjects0, pad_renderobjects1, pad_renderobjects2, pad_renderobjects3, pad_renderobjects4, pad_renderobjects5, pad_renderobjects6, pad_renderobjects7, pad_renderobjects8, pad_renderobjects9, pad_renderobjects10, pad_renderobjects11, pad_renderobjects12, pad_renderobjects13, pad_renderobjects14, pad_renderobjects15, pad_renderobjects16, pad_renderobjects17, pad_renderobjects18, pad_renderobjects19, pad_renderobjects20, pad_renderobjects21, pad_renderobjects22, pad_renderobjects23, pad_renderobjects24, pad_renderobjects25, pad_renderobjects26, pad_renderobjects27, pad_renderobjects28, pad_renderobjects29, pad_renderobjects30, pad_renderobjects31, pad_renderobjects32, pad_renderobjects33, pad_renderobjects34, pad_renderobjects35, pad_renderobjects36, pad_renderobjects37, pad_renderobjects38, pad_renderobjects39, pad_renderobjects40, pad_renderobjects41, pad_renderobjects42, pad_renderobjects43, pad_renderobjects44, pad_renderobjects45, pad_renderobjects46, pad_renderobjects47, pad_renderobjects48, pad_renderobjects49, pad_renderobjects50, pad_renderobjects51, pad_renderobjects52, pad_renderobjects53, pad_renderobjects54, pad_renderobjects55, pad_renderobjects56, pad_renderobjects57, pad_renderobjects58, pad_renderobjects59, pad_renderobjects60, pad_renderobjects61, pad_renderobjects62, pad_renderobjects63, pad_renderobjects64, pad_renderobjects65, pad_renderobjects66, pad_renderobjects67, pad_renderobjects68, pad_renderobjects69, pad_renderobjects70, pad_renderobjects71, pad_renderobjects72, pad_renderobjects73, pad_renderobjects74, pad_renderobjects75, pad_renderobjects76, pad_renderobjects77, pad_renderobjects78, pad_renderobjects79, pad_renderobjects80, pad_renderobjects81, pad_renderobjects82, pad_renderobjects83, pad_renderobjects84, pad_renderobjects85, pad_renderobjects86, pad_renderobjects87, pad_renderobjects88, pad_renderobjects89, pad_renderobjects90, pad_renderobjects91, pad_renderobjects92, pad_renderobjects93, pad_renderobjects94;

/* 0x10005aa0 */
void RenderObjects(DISPLAYLIST *list)
{
    int i;

    for (i = 0; i < g_numObjects; i++)
        g_objects[i].cls->Render(&g_objects[i], list);
}

/* 0x10005ae0: where the next lemming comes out, or -1 while the entrances open */
void GetEntrance(int *x, int *y)
{
    if (g_entrances[g_nextEntrance]->flags & OF_ACTIVE) {
        *x = -1;
        *y = -1;
        return;
    }
    *x = g_entrances[g_nextEntrance]->x + 25;
    *y = g_entrances[g_nextEntrance]->y + 14;
    if (++g_nextEntrance == g_numEntrances)
        g_nextEntrance = 0;
}

/* 0x10005b50: springs a trap; returns 0 if it is busy */
int TriggerObject(int index)
{
    int sound;

    if (!(g_objects[index].flags & OF_ACTIVE)) {
        switch (g_objects[index].sound) {
        case 6:
            sound = 12;
            break;
        case 7:
        case 8:
            sound = 20;
            break;
        case 9:
            sound = 8;
            break;
        case 11:
            sound = 18;
            break;
        case 13:
            sound = 8;
            break;
        case 14:
            sound = 19;
            break;
        case 15:
        case 17:
            sound = 15;
            break;
        default: {
            char msg[128];

            wsprintf(msg, "Unknown Sound Number=%d", g_objects[index].sound);
            MessageBox(NULL, msg, "BUG!!! Object Trigger BUG!!!", MB_OK);
            sound = -1;
            break;
        }
        }
        if (sound != -1) {
            PNT at;

            at.x = at.y = 0;
            SoundPlayFX(at, sound);
        }
        g_objects[index].flags |= OF_ACTIVE;
        return 1;
    }
    return 0;
}
