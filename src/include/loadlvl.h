/*
 * LOADLVL.DLL: loads a level. Levels are the DOS game's 2048-byte .LVL files
 * (big-endian); the terrain is drawn from the level style's RLE sprites into an
 * 8-bit DIB at the game's scale (1 or 2), coordinates scaled by 4.
 */
#ifndef LOADLVL_H
#define LOADLVL_H

#include <windows.h>
#include "readdib.h"

#if defined(LOADLVL_BUILD) && defined(_MSC_VER)
#define LVLAPI __declspec(dllexport)
#else
#define LVLAPI
#endif

/* An object placed in the level (entrance, exit, trap, decoration...) */
typedef struct LEVELOBJECT {
    int x, y;               /* 0x00 */
    int width, height;      /* 0x08 */
    int startFrame;         /* 0x10 */
    int endFrame;           /* 0x14 */
    int trigLeft, trigTop;  /* 0x18 */
    int trigWidth;          /* 0x20 */
    int trigHeight;         /* 0x24 */
    int trigEffect;         /* 0x28 */
    int animType;           /* 0x2c */
    int frame;              /* 0x30 */
    int sound;              /* 0x34 */
    WORD drawMode;          /* 0x38: 1, 4 (no overwrite) or 5 (only on terrain) */
    WORD sprite;            /* 0x3a: first frame in the objects sprite file */
} LEVELOBJECT;

/* An area of steel */
typedef struct STEELAREA {
    int x, y;
    int width, height;
    int type;               /* always 9 */
} STEELAREA;

typedef struct LEVEL {
    int graphicSet;         /* 0x00 */
    int releaseRate;        /* 0x04 */
    int toRescue;           /* 0x08 */
    int numLemmings;        /* 0x0c */
    int timeLimit;          /* 0x10 */
    int index[8];           /* 0x14: 0..6 (7 unset) */
    int skills[8];          /* 0x34 */
    int numObjects;         /* 0x54 */
    int numSteel;           /* 0x58 */
    int startX;             /* 0x5c */
    int field60;            /* 0x60 */
    char *fileName;         /* 0x64 */
    char *name;             /* 0x68 */
    PDIB terrain;           /* 0x6c */
    BYTE *objectSprites;    /* 0x70 */
    LEVELOBJECT *objects;   /* 0x74 */
    STEELAREA *steel;       /* 0x78 */
} LEVEL;

#ifdef __cplusplus
extern "C" {
#endif

/* scale: 1 normal, 2 high resolution; hwnd is not used */
#ifdef __cplusplus
/* the EXE passes its renderer handle as a WORD (LOADLVL ignores it) */
LVLAPI BOOL WINAPI LoadLevel(const char *file, WORD renderer, LEVEL *level, int scale);
#else
LVLAPI BOOL WINAPI LoadLevel(const char *file, int hwnd, LEVEL *level, int scale);
#endif
LVLAPI void WINAPI LevelFree(LEVEL *level);

#ifdef __cplusplus
}
#endif

#endif
