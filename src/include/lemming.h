/*
 * PROCESS.DLL internals. A lemming has an action - one of 17 global Action
 * objects (g_walker, g_faller...) whose methods move and draw it - and a
 * chain of message handlers that decide what happens when it walks into a
 * wall, runs out of ground, meets an object or its bomb countdown runs out.
 * Handlers are called newest first until one returns nonzero.
 */
#ifndef LEMMING_H
#define LEMMING_H

#include <windows.h>
#include <string.h>
#include "render.h"
#include "vsmem.h"
#include "filehand.h"
#include "loadlvl.h"
#define PROCESS_BUILD
#include "procdll.h"

/* Messages */
#define MSG_WALL        0x100   /* walked into a wall */
#define MSG_FALL        0x101   /* nothing underfoot */
#define MSG_OVERHANG    0x102   /* a climber hit its head */
#define MSG_LAND        0x103   /* landed: splats if it fell too far */
#define MSG_FLOAT       0x104   /* fallen far enough to open an umbrella */
#define MSG_OFFTOP      0x180   /* left the map */
#define MSG_OFFBOTTOM   0x181
#define MSG_OFFLEFT     0x182
#define MSG_OFFRIGHT    0x183
#define MSG_OBJECT      0x200   /* a: the object's effect | its index << 8 */
#define MSG_COUNTDOWN   0xf00   /* the bomb countdown ran out */

/* Object effects (the object grid's cells) */
#define OBJ_NONE        0
#define OBJ_EXIT        1
#define OBJ_TURNLEFT    2       /* a blocker's left side */
#define OBJ_TURNRIGHT   3
#define OBJ_TRAP        4
#define OBJ_WATER       5
#define OBJ_FIRE        6
#define OBJ_ONEWAYLEFT  7       /* one-way walls: can't be dug from the right */
#define OBJ_ONEWAYRIGHT 8
#define OBJ_STEEL       9
#define OBJ_BLOCKER     10

/* LEMMING state */
#define LS_GONE         1       /* dead or home */
#define LS_COUNTDOWN    2       /* the bomb countdown is running */

/* LEMMING skills */
#define SK_FLOATER      1
#define SK_CLIMBER      2
#define SK_BUILDER      4
#define SK_BLOCKER      8

/* LEMMING type: what can be done to it */
#define LT_ASSIGNABLE   1       /* can be given a digging or building skill */
#define LT_BUSY         0x8000  /* doing a skill: second choice for a new one */

/* Frames of the lemmings' RLE sprite files */
#define SPR_DIGHOLE     0x14e   /* the hole a digger digs; +bash frame for bashers */
#define SPR_EXPLOSION   0x14f   /* the hole an explosion makes */
#define SPR_MINEHOLE    0x158
#define SPR_BRICK       0x15c
#define SPR_COUNTDOWN   0x15d   /* 5, 4, 3, 2, 1 */

struct LEMMING;
typedef int (*HANDLERFN)(int msg, int a, int b, LEMMING *l);

struct HANDLER {
    HANDLERFN fn;
    HANDLER *next;
};

class Action {
public:
    virtual void Process(LEMMING *l, DLENTRY **list) = 0;
    virtual void Render(LEMMING *l, DLENTRY *e) = 0;
    virtual void Leave(LEMMING *l) = 0;
    virtual void Enter(LEMMING *l) = 0;
    virtual int Assign(LEMMING *l) = 0;
    virtual void Init(int a, int b) = 0;

    int unused;
    int leftFrames;         /* added to the frame when facing left */
};

#define ACTION(name) \
    class name : public Action { \
    public: \
        void Process(LEMMING *l, DLENTRY **list); \
        void Render(LEMMING *l, DLENTRY *e); \
        void Leave(LEMMING *l); \
        void Enter(LEMMING *l); \
        int Assign(LEMMING *l); \
        void Init(int a, int b);

ACTION(Walker) };
ACTION(Faller) };
ACTION(Climber) };
ACTION(Flipper) };
ACTION(Floater) };
ACTION(Flapper) };
ACTION(Exploder) };
ACTION(Splatter) };
ACTION(Blocker) };
ACTION(Drowner) };
ACTION(Frier) };
ACTION(Homer) };
ACTION(Builder) };
ACTION(Shrugger) };
ACTION(Digger)
    Digger() { unused2 = 0; }
    int unused2;
};
ACTION(Basher) };
ACTION(Miner) };

extern Walker g_walker;
extern Faller g_faller;
extern Climber g_climber;
extern Flipper g_flipper;
extern Floater g_floater;
extern Flapper g_flapper;
extern Exploder g_exploder;
extern Splatter g_splatter;
extern Blocker g_blocker;
extern Drowner g_drowner;
extern Frier g_frier;
extern Homer g_homer;
extern Builder g_builder;
extern Shrugger g_shrugger;
extern Digger g_digger;
extern Basher g_basher;
extern Miner g_miner;

struct LEMMING {
    int x, y;               /* 0x00: feet */
    int xoff, yoff;         /* 0x08: the sprite's position from the feet */
    int dir;                /* 0x10: 1 right, -1 left */
    int frame;              /* 0x14 */
    int base;               /* 0x18: the action's first sprite frame */
    int frames;             /* 0x1c */
    int loop;               /* 0x20: the frame the animation loops back to */
    int countdown;          /* 0x24: bomb countdown, in frames */
    int state;              /* 0x28: LS_ */
    int skills;             /* 0x2c: SK_ */
    int type;               /* 0x30: LT_ */
    short count;            /* 0x34: pixels fallen; bricks left */
    short unused36;
    int step;               /* 0x38: floater: position in its table */
    int more[8];            /* 0x3c: blocker: the object cells it covers, from 0x34 */
    Action *action;         /* 0x5c */
    char *name;             /* 0x60 */
    HANDLER *handlers;      /* 0x64 */

    LEMMING(int x, int y)
    {
        this->x = x;
        this->y = y;
        action = &g_faller;
        handlers = NULL;
        action->Enter(this);
        dir = 1;
        countdown = 0;
        state = 0;
        skills = 0;
    }
    void CheckMap();
    void CountDown();
};

/* Sends a message to a lemming's handlers until one takes it */
inline void Message(LEMMING *l, int msg, int a, int b)
{
    HANDLER *h = l->handlers;

    while (h) {
        if (h->fn(msg, a, b, l))
            h = NULL;
        else
            h = h->next;
    }
}

inline HANDLER *FindHandler(LEMMING *l, HANDLERFN fn)
{
    HANDLER *h = l->handlers;

    while (h) {
        if (h->fn == fn)
            return h;
        h = h->next;
    }
    return NULL;
}

/* Adds a handler unless the lemming has it already; returns whether it was added */
inline int AddHandler(LEMMING *l, HANDLERFN fn)
{
    HANDLER *h;

    if (FindHandler(l, fn))
        return 0;
    h = new HANDLER;
    h->next = l->handlers;
    h->fn = fn;
    l->handlers = h;
    return 1;
}

inline void RemoveHandler(LEMMING *l, HANDLERFN fn)
{
    HANDLER *prev = NULL;
    HANDLER *h;

    for (h = l->handlers; h; prev = h, h = h->next) {
        if (h->fn == fn) {
            if (prev)
                prev->next = h->next;
            else
                l->handlers = h->next;
            delete h;
            return;
        }
    }
}

/* The object grid: an effect for every 4x4 pixels */
struct OBJGRID {
    int width, height;      /* pixels */
    int cols, rows;
    int *cells;

    OBJGRID(int w, int h)
    {
        int n;

        width = w;
        height = h;
        cols = w = (w + 3) / 4;    /* reusing w and h keeps cols in a register (Map::Map) */
        rows = h = (h + 3) / 4;
        n = w * h;
        cells = new int[n];
        memset(cells, 0, n * sizeof(int));
    }
};

/* A kind of object with its own sprites (not used by the game) */
struct OBJTYPE {
    WORD list;              /* sprite list */
    int count;
    int width, height;
    BYTE **frames;          /* each frame's pixels */
};

/* The level's terrain as a bitmap of solid pixels, and its object grid */
class Map {
public:
    BYTE *bits;             /* 0x00: a bit per pixel, bottom row first */
    int width, height;      /* 0x04 */
    OBJGRID *grid;          /* 0x0c */
    OBJTYPE types[10];      /* 0x10 */
    int numTypes;           /* 0xd8 */

    Map();
    Map(BYTE *pixels, int width, int height);
    ~Map();
    int AddObjectType(WORD renderer, WORD slot, int x, int y, int w, int h, int count);
    int AddTrapType(WORD renderer, WORD slot, int x, int y, int w, int h, int count);
    void RemoveSprite(int x, int y, BYTE *sprites, int frame);
    void RemoveBits(int x, int y, int w, int h, BYTE *pixels);
    void EraseSprite(DLENTRY **list, BYTE *sprites, int frame, int x, int y);
    void EraseObject(int type, DLENTRY **list, int x, int y, int frame);
    void AddSpriteBits(int x, int y, BYTE *sprites, int frame);
    void AddBits(int x, int y, int w, int h, BYTE *pixels);
    void AddSprite(DLENTRY **list, BYTE *sprites, int frame, int x, int y);
    void AddObject(int type, DLENTRY **list, int x, int y, int frame);
    BYTE GetPixel(int x, int y);
    int GetObjectAt(int x, int y);
    int SetObjectAt(int x, int y, int value);
    void FillObjects(int x, int y, int w, int h, int value);
    void SetMap(BYTE *pixels, int width, int height);
};

/* lemmings.cpp */
extern int g_maxLemmings;
extern int pad_hdr_numlemmings;
extern int g_armageddon;
extern int g_lemmsHome;
extern int g_scale;
extern BYTE *g_sprites;
extern Map *g_map;
extern LEMMING *g_lemmings[100];

/* default.cpp */
int DefaultHandler(int msg, int a, int b, LEMMING *l);
int CanAssign(LEMMING *l, int type);

/* Off the edge of the map, or on an object? */
inline void LEMMING::CheckMap()
{
    int obj;

    if (y < 0)
        Message(this, MSG_OFFTOP, 0, 0);
    if (y >= g_map->height)
        Message(this, MSG_OFFBOTTOM, 0, 0);
    if (x < 0)
        Message(this, MSG_OFFLEFT, 0, 0);
    if (x >= g_map->width)
        Message(this, MSG_OFFRIGHT, 0, 0);
    obj = g_map->GetObjectAt(x, y);
    if (obj)
#ifdef PORT_SDL                 /* 64-bit: the handlers ignore this argument (PORTING.md) */
        Message(this, MSG_OBJECT, obj, (int)(INT_PTR)this);
#else
        Message(this, MSG_OBJECT, obj, (int)this);
#endif
}

inline void LEMMING::CountDown()
{
    if (state & LS_COUNTDOWN) {
        if (countdown < 0)
            MessageBox(NULL, name, "CountDown Underflow", MB_OK);
        if (countdown >= 0 && --countdown <= 0)
            Message(this, MSG_COUNTDOWN, 0, 0);
    }
}

/* script.cpp */
extern char *g_actionNames[NUM_ACTIONS];
extern Action *g_actions[NUM_ACTIONS];

/* objects.cpp */
void InitObjects(WORD renderer, LEVEL *level);
void QuitObjects(void);
void ProcessObjects(void);
void RenderObjects(DISPLAYLIST *list);
void GetEntrance(int *x, int *y);
int TriggerObject(int index);

/* sound.cpp */
extern int g_sfxEnabled;

/* wave.cpp */
extern int g_waveEnabled;
void WaveInit(void);
void WaveQuit(void);
void WaveFX(PNT at, int sound);

#endif
