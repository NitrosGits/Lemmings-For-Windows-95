/*
 * WinMain: start-up checks, the cursors, the message loop and the error
 * boxes every other part of the game uses.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0;

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "winlemm.h"

char g_szAppName[16] = "Lemmings";
char g_szIniFile[] = "winlemm.ini";
HWND g_hwnd = 0;
int g_unused20 = 0;
HWND g_hwndView = 0;
int g_global28 = 0;
int g_global2c = 0;
int g_win32 = 1;                /* not Win32s: Windows 95 or NT 3.10+ */
int g_win32s = 0;

HINSTANCE g_hInst;
int g_nCmdShowData;
int g_unused510MainNew;              /* unused: fills a gap in the original .bss */
char g_stringBuf[256];
char g_curDir[1024];
HCURSOR g_hCrossCursorNext;
HCURSOR g_hBoxCursorSpare;
HCURSOR g_hSmaCrossCursorState;
HCURSOR g_hSmaBoxCursorGameNew;
HCURSOR g_boxHCursorLeft;
HCURSOR g_boxCursorHRight;
HCURSOR g_leftHBoxSmaCursor;
HCURSOR g_hBoxCursorRightSma;
MEMORYSTATUS g_memStatus;

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_checkcolourdepth0, pad_checkcolourdepth1, pad_checkcolourdepth2, pad_checkcolourdepth3, pad_checkcolourdepth4, pad_checkcolourdepth5, pad_checkcolourdepth6, pad_checkcolourdepth7, pad_checkcolourdepth8, pad_checkcolourdepth9, pad_checkcolourdepth10, pad_checkcolourdepth11, pad_checkcolourdepth12, pad_checkcolourdepth13, pad_checkcolourdepth14;

/* 0x401000: warn when the display isn't in 256 colours */
void CheckColourDepth(void)
{
    char num[32];
    char msg[160];
    char *s;

    {
        HDC hdc;
        int planes;
        int bits;

        hdc = GetDC(0);
        planes = GetDeviceCaps(hdc, PLANES);
        bits = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(0, hdc);
        if (bits * planes == 8)
            return;
        switch (bits) {
        case 8:
            s = GetString(0x7e03);
            break;
        case 15:
            s = GetString(0x7e02);
            break;
        case 16:
            s = GetString(0x7e01);
            break;
        case 24:
            s = GetString(0x7e00);
            break;
        default:
            itoa(1 << (bits * planes), num, 10);
            s = num;
            break;
        }
    }
    {
        char depth[160];

        strcpy(depth, s);
        wsprintf(msg, GetString(0x7e04), depth);
        MessageBox(0, msg, GetString(0x7e05), MB_ICONQUESTION);
    }
}

/* 0x401150 */
void InitSystemInfo(void)
{
    CheckColourDepth();
    g_memStatus.dwLength = sizeof(MEMORYSTATUS);
    g_memStatus.dwTotalPhys = 0x400;
    GlobalMemoryStatus(&g_memStatus);
    InitPalette();
}

/* 0x401180 */
int InitApplication(void)
{
    LoadSettings();
    ShowSplash(g_hInst);
    RandomTune();
    InitPlayback(0x8000);
    g_hCrossCursorNext = LoadCursor(g_hInst, "CROSSCURSOR");
    g_hBoxCursorSpare = LoadCursor(g_hInst, "BOXCURSOR");
    g_hSmaCrossCursorState = LoadCursor(g_hInst, "SMACROSSCURSOR");
    g_hSmaBoxCursorGameNew = LoadCursor(g_hInst, "SMABOXCURSOR");
    g_boxHCursorLeft = LoadCursor(g_hInst, "LEFTBOXCURSOR");
    g_boxCursorHRight = LoadCursor(g_hInst, "RIGHTBOXCURSOR");
    g_leftHBoxSmaCursor = LoadCursor(g_hInst, "LEFTSMABOXCURSOR");
    g_hBoxCursorRightSma = LoadCursor(g_hInst, "RIGHTSMABOXCURSOR");
    if (!g_hCrossCursorNext || !g_hSmaCrossCursorState)
        ErrorBox(0x7f02);
    if (!g_hBoxCursorSpare || !g_hSmaBoxCursorGameNew || !g_boxHCursorLeft || !g_boxCursorHRight ||
        !g_leftHBoxSmaCursor || !g_hBoxCursorRightSma)
        ErrorBox(0x7f03);
    if (!InitClasses(g_hInst))
        return 0;
    if (!InitInstance(g_hInst, g_nCmdShowData))
        return 0;
    InitialiseProcess();
    HideSplash();
    PostMessage(g_hwnd, 0x704, 0, 0);
    return 1;
}

/* 0x401300 */
void QuitApplication(void)
{
    if (g_hCrossCursorNext)
        DestroyCursor(g_hCrossCursorNext);
    if (g_hBoxCursorSpare)
        DestroyCursor(g_hBoxCursorSpare);
    if (g_hSmaBoxCursorGameNew)
        DestroyCursor(g_hSmaBoxCursorGameNew);
    if (g_hSmaCrossCursorState)
        DestroyCursor(g_hSmaCrossCursorState);
    if (g_boxHCursorLeft)
        DestroyCursor(g_boxHCursorLeft);
    if (g_boxCursorHRight)
        DestroyCursor(g_boxCursorHRight);
    if (g_leftHBoxSmaCursor)
        DestroyCursor(g_leftHBoxSmaCursor);
    if (g_hBoxCursorRightSma)
        DestroyCursor(g_hBoxCursorRightSma);
    QuitProcess();
    QuitPlayback();
    SaveSettings();
}

/* 0x401380 */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    LRESULT (WINAPI *dispatch)(const MSG *);
    BOOL (WINAPI *wait)(void);
    BOOL (WINAPI *peek)(LPMSG, HWND, UINT, UINT, UINT);
    MSG msg;
    HACCEL hAccel;
    HWND hwnd;
    int len;

    hwnd = FindPreviousInstance();
    if (hwnd) {
        SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        return 0;
    }
    GetCurrentDirectory(sizeof(g_curDir), g_curDir);
    len = strlen(g_curDir) - 1;
    if (g_curDir[len] == '\\')
        g_curDir[len] = 0;
    g_hInst = hInstance;
    g_nCmdShowData = nCmdShow;
    InitSystemInfo();
    if (!InitApplication())
        return 0;
    hAccel = LoadAccelerators(hInstance, g_szAppName);
    dispatch = DispatchMessageA;
    wait = WaitMessage;
    peek = PeekMessageA;
    for (;;) {
        if (peek(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                break;
            if (!TranslateAccelerator(g_hwnd, hAccel, &msg)) {
                TranslateMessage(&msg);
                dispatch(&msg);
            }
        } else if (GameIdle())
            wait();
    }
    QuitApplication();
    return msg.wParam;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_waittime0, pad_waittime1, pad_waittime2, pad_waittime3, pad_waittime4, pad_waittime5, pad_waittime6, pad_waittime7, pad_waittime8, pad_waittime9, pad_waittime10, pad_waittime11, pad_waittime12, pad_waittime13, pad_waittime14, pad_waittime15, pad_waittime16;

/* 0x4014a0: run the message loop for a while; nonzero if the game was quit */
int WaitTime(DWORD ms)
{
    MSG msg;
    DWORD start = timeGetTime();
    int done = 0;
    int quit = 0;

    do {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                quit = done = 1;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else
            WaitMessage();
        if (timeGetTime() - start >= ms) {
            done = 1;
            quit = 0;
        }
    } while (!done);
    return quit;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_initclasses0, pad_initclasses1, pad_initclasses2, pad_initclasses3, pad_initclasses4, pad_initclasses5, pad_initclasses6, pad_initclasses7, pad_initclasses8, pad_initclasses9, pad_initclasses10, pad_initclasses11, pad_initclasses12, pad_initclasses13, pad_initclasses14, pad_initclasses15, pad_initclasses16, pad_initclasses17, pad_initclasses18, pad_initclasses19, pad_initclasses20, pad_initclasses21;

/* 0x401520 */
int InitClasses(HINSTANCE hInstance)
{
    int main = RegisterViewClass(hInstance);
    int status = RegisterStatusClass(hInstance);
    int map = RegisterFrameClass(hInstance);
    int icons = RegisterIconClasses(hInstance);

    if (main && status && map && icons)
        return 1;
    return 0;
}

/* 0x401580 */
int InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    DWORD version = GetVersion();

    if (LOWORD(version) >= 3) {
        g_win32 = 1;
        if (HIWORD(version) < 10)
            goto s;
    }
    g_win32 = 0;
s:
    if (version & 0x80000000) {
        g_win32s = 1;
        if (LOBYTE(version) < 4)
            goto t;
    }
    g_win32s = 0;
t:
    g_hwnd = CreateMainWindow(hInstance);
    return 1;
}

/* 0x4015f0 */
char *GetString(UINT id)
{
    char buf[128];

    if (!LoadString(g_hInst, id, g_stringBuf, sizeof(g_stringBuf))) {
        DWORD err = GetLastError();

        wsprintf(buf, "Error while getting string=%08lx\n String=%s\n", err, g_stringBuf);
        MessageBox(0, buf, "Error", 0);
    }
    return g_stringBuf;
}

/* 0x401660 */
void ErrorBox(UINT id)
{
    char buf[256];

    strcpy(buf, GetString(id));
    MessageBox(0, buf, GetString(0x7f00), 0);
}

/* 0x4016c0 */
EXPORT void WINAPI FatalErrorCleanUp(void)
{
    if (g_hCrossCursorNext)
        DestroyCursor(g_hCrossCursorNext);
    if (g_hBoxCursorSpare)
        DestroyCursor(g_hBoxCursorSpare);
    QuitProcess();
    QuitPlayback();
    VSMEM_FreeAllMemory();
}

/* 0x401700 */
void FatalError(UINT id)
{
    ErrorBox(id);
    FatalErrorCleanUp();
    ExitProcess(0xffff);
}

/* 0x401720: does a window at this size fit on the screen? */
int WindowFits(int size, int scale)
{
    int height;

    if (size != 1)
        scale += scale;
    height = GetSystemMetrics(SM_CYSIZE) + GetSystemMetrics(SM_CYMENU) + GetDocksHeight() +
             CalcStatusHeight() + scale * 160;
    return GetSystemMetrics(SM_CYSCREEN) >= height;
}
