/*
 * Sound effects: each is played as a wave (wave.cpp) and some also show a
 * word over the lemming for a few frames ("Let's go!", "Oh no!"...).
 */
#include "lemming.h"

#define MAX_SFX 20

struct SOUNDFX {
    int sx, sy;                 /* in the original sheet (not used) */
    int width, height;
    int frames;                 /* how long its word stays up; 0: it has none */
    WORD sprite;                /* its frame in the sprite file, or 0xffff */
    WORD unused;
};

/* A word on screen */
struct SFX {
    WORD active;
    PNT at;
    int sound;
    int timer;
};

int g_sfxEnabled = 1;
int g_sfxUnused = 0;
SOUNDFX g_sfxTable[23] = {
    {0, 0, 27, 15, 5},          /* chink */
    {0, 15, 24, 23, 10},        /* let's go */
    {122, 0, 27, 15, 5},        /* yippee */
    {92, 0, 27, 15, 5},         /* splat */
    {31, 0, 27, 15, 5},         /* oh no */
    {184, 0, 27, 15, 5},        /* glug */
    {153, 0, 27, 15, 5},        /* scrape */
};

BYTE *g_sfxSprites;
WORD g_sfxRenderer;
SFX g_sfx[MAX_SFX];
DISPLAYLIST *g_sfxList;

#ifdef _MSC_VER
/* SoundInit was built for size, not speed (docs/DECOMP.md, "Size-mode functions"). */
#pragma optimize("t", off)
#endif
/* 0x10005cc0 */
void WINAPI SoundInit(WORD renderer, WORD slot)
{
    HANDLE file;
    DWORD size;
    int i, n;

    file = FILE_ReadOpen(g_scale == 2 ? "gfx\\vsfxbig.spr" : "gfx\\vsfxsma.spr");
    size = FILE_Length(file);
    g_sfxSprites = (BYTE *)VSMEM_Allocate(size, "VSFX RLE Sprite Buffer");
    FILE_Read(file, g_sfxSprites, size);
    FILE_Close(file);
    n = 0;
    for (i = 0; i < 23; i++) {
        if (g_sfxTable[i].frames)
            g_sfxTable[i].sprite = (WORD)n++;
        else
            g_sfxTable[i].sprite = 0xffff;
    }
    g_sfxRenderer = renderer;
    for (i = 0; i < MAX_SFX; i++)
        g_sfx[i].active = 0;
    g_sfxList = NewDisplayList(MAX_SFX);
    g_sfxList->count = MAX_SFX;
    for (i = 0; i < MAX_SFX; i++)
        g_sfxList->entries[i].slot = slot;
}

#ifdef _MSC_VER
#pragma optimize("t", on)
/* A size-mode function gets no alignment padding after it, but the original has the
   9 bytes (mov edi,edi / lea esp,[esp+0]) that a speed-mode one gets, so this empty
   function supplies them. */
void __declspec(naked) SoundInitPad(void)
{
    __asm {
        _emit 0x8b
        _emit 0xff
        _emit 0x8d
        _emit 0xa4
        _emit 0x24
        _emit 0x00
        _emit 0x00
        _emit 0x00
        _emit 0x00
    }
}
#endif

/* 0x10005d80 */
void WINAPI SoundQuit(void)
{
    int i;

    for (i = 0; i < MAX_SFX; i++)
        g_sfx[i].active = 0;
    VSMEM_Free(g_sfxSprites);
    FreeDisplayList(g_sfxList);
}

/* 0x10005db0 */
static int IsActive(int i)
{
    return g_sfx[i].active & 1;
}

/* 0x10005dd0 */
static int AllocSfx(void)
{
    int i;

    for (i = 0; i < MAX_SFX; i++) {
        if (!IsActive(i)) {
            g_sfx[i].active = 1;
            return i;
        }
    }
    return -1;
}

/* 0x10005e10 */
static void FreeSfx(int i)
{
    g_sfx[i].active = 0;
}

/* 0x10005e30 */
void WINAPI SoundPlayFX(PNT at, int sound)
{
    int i;

    if (g_sfxEnabled && g_sfxTable[sound].sprite != 0xffff && (i = AllocSfx()) != -1) {
        g_sfx[i].at = at;
        g_sfx[i].sound = sound;
        g_sfx[i].timer = g_sfxTable[sound].frames;
    }
    WaveFX(at, sound);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_soundprocess0;

/* 0x10005eb0 */
void WINAPI SoundProcess(void)
{
    SFX *s;
    int i;

    for (i = 0, s = g_sfx; s < g_sfx + MAX_SFX; s++, i++) {
        if (IsActive(i)) {
            if (s->timer == 0)
                FreeSfx(i);
            else
                s->timer--;
        }
    }
}

/* 0x10005ef0 */
static void AddSfx(SFX *s, DISPLAYLIST *list)
{
    DLENTRY *e = &list->entries[list->count];
    SOUNDFX *t = &g_sfxTable[s->sound];

    if (t->sprite != 0xffff) {
        e->type = DL_RLE;
        e->u.rle.mode = 1;
        e->u.rle.x = (short)((s->at.x - t->width / 2) * g_scale);
        e->u.rle.y = (short)((s->at.y - t->height) * g_scale);
        e->u.rle.sprites = g_sfxSprites;
        e->u.rle.frame = t->sprite;
        list->count++;
    }
}

/* 0x10005f70 */
void WINAPI SoundRender(void)
{
    int i;

    g_sfxList->count = 0;
    for (i = 0; i < MAX_SFX; i++)
        if (IsActive(i))
            AddSfx(&g_sfx[i], g_sfxList);
    Renderer_RenderList(g_sfxRenderer, g_sfxList);
}
