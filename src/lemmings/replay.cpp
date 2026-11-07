/*
 * Recording and replaying a level: a buffer of events (skills given,
 * scrolling, release rate, nuke) stamped with the frame they happened in.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1;

#include <windows.h>
#include <string.h>
#include "winlemm.h"

REPLAYEVENT *g_replayPtrVal;
int g_recordingMain;
REPLAYEVENT *g_replayEnd;
int g_replaySize;
REPLAYEVENT *g_mainReplayBuf;
int g_newReplayActive;

/* 0x40d4a0 */
void InitPlayback(int size)
{
    g_replayEnd = 0;
    g_replayPtrVal = 0;
    g_mainReplayBuf = (REPLAYEVENT *)VSMEM_Allocate(size, "PlayBack buffer");
    if (g_mainReplayBuf) {
        g_replaySize = size;
        g_replayPtrVal = g_mainReplayBuf;
        g_newReplayActive = 0;
        g_replayEnd = (REPLAYEVENT *)((char *)g_mainReplayBuf + size);
    }
}

/* 0x40d4f0 */
void QuitPlayback(void)
{
    if (g_mainReplayBuf) {
        VSMEM_Free(g_mainReplayBuf);
        g_mainReplayBuf = 0;
        g_replayPtrVal = 0;
        g_replayEnd = 0;
    }
}

/* 0x40d520 */
int IsRecording(void)
{
    if (g_newReplayActive && g_recordingMain)
        return 1;
    return 0;
}

/* 0x40d540 */
int IsReplaying(void)
{
    if (g_newReplayActive && !g_recordingMain)
        return 1;
    return 0;
}

/* 0x40d560 */
int RewindReplay(void)
{
    g_replayPtrVal = g_mainReplayBuf;
    g_newReplayActive = 1;
    return 1;
}

/* 0x40d580 */
int StopReplayBuffer(void)
{
    g_replayPtrVal = g_mainReplayBuf;
    g_newReplayActive = 0;
    return 1;
}

/* 0x40d5a0 */
int StartRecord(void)
{
    memset(g_mainReplayBuf, 0, g_replaySize);
    RewindReplay();
    g_recordingMain = 1;
    return 1;
}

/* 0x40d5e0: carry on recording from here */
void StopReplay(void)
{
    g_recordingMain = 1;
    memset(g_replayPtrVal, 0, (char *)g_mainReplayBuf - (char *)g_replayPtrVal + g_replaySize);
}

/* 0x40d620 */
int EndRecord(void)
{
    StopReplayBuffer();
    return 1;
}

/* 0x40d630 */
int StartReplay(void)
{
    RewindReplay();
    g_recordingMain = 0;
    return 1;
}

/* 0x40d650 */
int EndReplay(void)
{
    StopReplayBuffer();
    return 1;
}

/* 0x40d660 */
int RecordEvent(int time, int type, int x, int y, int value, int priority, int unused18, int unused1c)
{
    if (!time)
        return 1;
    g_replayPtrVal->time = time;
    g_replayPtrVal->type = type;
    g_replayPtrVal->x = x;
    g_replayPtrVal->y = y;
    g_replayPtrVal->value = value;
    g_replayPtrVal->priority = priority;
    g_replayPtrVal->unused18 = unused18;
    g_replayPtrVal->unused1c = unused1c;
    g_replayPtrVal++;
    return g_replayPtrVal < g_replayEnd;
}

/* 0x40d6f0 */
int RecordSkill(int time, int x, int y, int skill, int priority)
{
    return RecordEvent(time, 0, x, y, skill, priority, 0, 0);
}

/* 0x40d720 */
int RecordScroll(int time, int x, int y)
{
    return RecordEvent(time, 1, x, y, 0, 0, 0, 0);
}

/* 0x40d750 */
int RecordRate(int time, int rate)
{
    return RecordEvent(time, 2, 0, 0, rate, 0, 0, 0);
}

/* 0x40d770 */
int RecordNuke(int time)
{
    return RecordEvent(time, 3, 0, 0, 0, 0, 0, 0);
}

/* 0x40d790 */
int GetReplayEvent(int time, REPLAYEVENT *ev)
{
    if (!time)
        return 0;
    if (g_replayPtrVal->time == time) {
        *ev = *g_replayPtrVal;
        g_replayPtrVal++;
        return g_replayPtrVal < g_replayEnd;
    }
    return 0;
}
