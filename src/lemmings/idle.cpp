/*
 * The game's clock: between messages, step the level every 80 ms (three
 * times in fast forward).
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3;

#include <windows.h>
#include "winlemm.h"

DWORD g_lastTick = 0;
DWORD g_frameTime = 80;

/* 0x403e30: nonzero to wait for the next message */
int GameIdle(void)
{
    DWORD elapsed = timeGetTime() - g_lastTick;

#ifdef _MSC_VER
    if (elapsed < g_frameTime && !IsFastForward()) {
#else
    /* The original never sleeps in fast forward and never restarts the clock
       on the title screen, so both run as fast as the machine allows. The
       portable build keeps 80 ms a pass there too: fast forward is then the
       intended three steps a pass, three times normal speed. */
    if (elapsed < g_frameTime) {
#endif
        if (g_win32s)
            return 0;
        Sleep(g_frameTime - elapsed);
    }
    if (!InGame()) {
        PostMessage(g_hwnd, 0x500, 0, 0);
#ifndef _MSC_VER
        g_lastTick = timeGetTime();
#endif
        return 1;
    }
    if (IsPaused())
        return 0;
    PostMessage(g_hwndView, 0x400, 0, 0);
    if (IsFastForward()) {
        PostMessage(g_hwndView, 0x400, 0, 0);
        PostMessage(g_hwndView, 0x400, 0, 0);
    }
    PostMessage(g_hwndView, 0x500, 0, 0);
    g_lastTick = timeGetTime();
    return 1;
}

/* 0x403f10 */
void AssertFailed(char *expr, char *file, int line)
{
    char text[256];

    wsprintf(text, "%s\nin file \"%s\" at line %u\n", expr, file, line);
    MessageBox(0, text, "Visual Sciences Assertion Failure", MB_ICONEXCLAMATION);
    ExitProcess(0xaaaa);
}
