/*
 * PROCESS.DLL: the lemmings themselves - their actions, the level's collision
 * map and objects (entrances, exits, traps), the sound effects and their
 * on-screen words. Written in C++: some exports keep their C++ names.
 */
#ifndef PROCDLL_H
#define PROCDLL_H

#include <windows.h>
#include "loadlvl.h"

#if defined(PROCESS_BUILD) && defined(_MSC_VER)
#define PROCAPI __declspec(dllexport)
#else
#define PROCAPI
#endif

/* Actions, as numbered by MakeLemming and CheckLemming */
#define ACT_WALKER      0
#define ACT_FALLER      1
#define ACT_CLIMBER     2
#define ACT_FLIPPER     3       /* climbing over the top */
#define ACT_FLOATER     4
#define ACT_FLAPPER     5       /* "oh no!" before exploding */
#define ACT_EXPLODER    6
#define ACT_SPLATTER    7
#define ACT_BLOCKER     8
#define ACT_DROWNER     9
#define ACT_FRIER       10
#define ACT_HOMER       11      /* going in through the exit */
#define ACT_BUILDER     12
#define ACT_SHRUGGER    13      /* out of bricks */
#define ACT_DIGGER      14
#define ACT_BASHER      15
#define ACT_MINER       16
#define NUM_ACTIONS     17

#ifdef __cplusplus
extern "C" {
#endif

PROCAPI void WINAPI InitialiseProcess(void);
PROCAPI void WINAPI QuitProcess(void);
PROCAPI void WINAPI SetSFXFlags(int sfx, int wave);
PROCAPI void WINAPI SetNumLemmings(int count);
PROCAPI void WINAPI SetLemmingDelay(int delay);
PROCAPI void WINAPI SetScale(int scale);
PROCAPI int WINAPI GetLemmingDelay(void);
PROCAPI int WINAPI GetLemmsHome(void);
PROCAPI void WINAPI SetLevelMap(BYTE *pixels, int width, int height);
PROCAPI void WINAPI InitialiseLemmings(WORD renderer, WORD slot, LEVEL *level);
PROCAPI void WINAPI InitALemming(int x, int y);
PROCAPI int WINAPI ProcessLemmings(int *active);
PROCAPI void WINAPI RenderLemmings(WORD renderer, WORD slot);
PROCAPI int WINAPI GetLemmingXY(short *xy, int max);
PROCAPI char *WINAPI CheckLemming(int x, int y, int action, int priority);
PROCAPI int WINAPI MakeLemming(int x, int y, int action, int priority);
PROCAPI void WINAPI ProcessArmageddon(void);
PROCAPI void WINAPI DeleteLemmings(void);

#ifdef __cplusplus
}

typedef struct tagPNT {
    int x, y;
} PNT;

/* These keep their C++ names (MinGW's differ: there they are plain names,
   exported under the C++ ones by process.def) */
#ifndef _MSC_VER
extern "C" {
#endif

PROCAPI void WINAPI SoundInit(WORD renderer, WORD slot);
PROCAPI void WINAPI SoundQuit(void);
PROCAPI void WINAPI SoundPlayFX(PNT at, int sound);
PROCAPI void WINAPI SoundProcess(void);
PROCAPI void WINAPI SoundRender(void);
PROCAPI int WINAPI WaveIsEnabled(void);
PROCAPI int WINAPI WaveIsVolumeEnabled(void);
PROCAPI void WINAPI WaveSetVolume(int volume);
PROCAPI void WINAPI WaveTestVolume(int volume);
PROCAPI void WINAPI InitScript(char *file);
PROCAPI void WINAPI CheckScript(int frame);
#ifndef _MSC_VER
}
#endif
#endif

#endif
