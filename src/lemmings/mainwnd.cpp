/*
 * The main window while a level is played: the game view, the status bar,
 * the two docks the skill panel and map dock into, and the menu commands.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14, pad_top15, pad_top16, pad_top17, pad_top18;

#include <windows.h>
#include <string.h>
#include "winlemm.h"
/* The original's inline Dock members declared more locals than ours need; the extra symbols
   (see docs/DECOMP.md) are only wanted here, so they are switched on per file. */
#define MAINWND_PAD
#include "window.h"

int g_replaying = 0;
Dock *g_dockList = g_docks;
int g_numDocks = 2;
int g_quitToMenu = 0;
int g_currentSkill = -1;
IconWnd *g_iconWnd = 0;
MapWnd *g_mapWnd = 0;
int g_unused2fc = 0;            /* a scalar: arrays of 8 bytes or more are 8-aligned */
int g_unusedTable[23] = {
    0, 0, 0x15e, 0, 0x15f, 0, 0x160, 0, 0x161, 0, 0,
    0, 0x162, 0, 0x165, 0x163, 0x164, 0, 0, 0x16f, 0x171, 0, 0x173,
};
int g_paused = 0;
int g_nuking = 0;
int g_fastForward = 0;

Dock g_docks[2];
char g_helpMenu[104];
char g_savedTitle[80];
int g_unused610;              /* unused: fills a gap in the original .bss */
void (*g_ownMenuHelpCallback)(void);
int g_spareGameRunning;

/* 0x402150: the window class of an earlier design, not used */
int RegisterGameClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, g_szAppName);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = g_szAppName;
    wc.lpszClassName = g_szAppName;
    return RegisterClass(&wc);
}

/* 0x4021d0: not used */
HWND CreateGameWindow(HINSTANCE hInstance, HWND parent, HMENU menu, RECT *rect)
{
    return CreateWindowEx(0, g_szAppName, 0, WS_OVERLAPPEDWINDOW, rect->left, rect->top, rect->right, rect->bottom,
                          parent, menu, hInstance, 0);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_enablesizemenus0, pad_enablesizemenus1, pad_enablesizemenus2, pad_enablesizemenus3, pad_enablesizemenus4, pad_enablesizemenus5, pad_enablesizemenus6, pad_enablesizemenus7, pad_enablesizemenus8, pad_enablesizemenus9, pad_enablesizemenus10, pad_enablesizemenus11, pad_enablesizemenus12, pad_enablesizemenus13, pad_enablesizemenus14, pad_enablesizemenus15, pad_enablesizemenus16, pad_enablesizemenus17, pad_enablesizemenus18, pad_enablesizemenus19, pad_enablesizemenus20, pad_enablesizemenus21, pad_enablesizemenus22, pad_enablesizemenus23;

/* 0x402210: grey the sizes that won't fit on the screen */
void EnableSizeMenus(HMENU hmenu)
{
    HDC hdc = GetDC(0);
    int height = GetDeviceCaps(hdc, HORZRES);

    height -= GetSystemMetrics(SM_CYMENU);
    height -= GetSystemMetrics(SM_CYFRAME);
    height -= GetDocksHeight();
    height -= CalcStatusHeight();
    if (height < 320)
        EnableMenuItem(hmenu, 0x171, MF_DISABLED);
    if (height < 640)
        EnableMenuItem(hmenu, 0x173, MF_DISABLED);
    ReleaseDC(0, hdc);
}

/* 0x4022a0: build the play screen */
int StartGame(HINSTANCE hInstance, int nCmdShow)
{
    RECT r;

    EnableSizeMenus(LoadGameMenu(g_szAppName));
    SetMainProc(MainWndProc);
    g_rectStatus.right = g_clientWidth;
    g_viewRectSpare.right = g_clientWidth;
    g_dockList[0].SetSize(g_clientWidth, 1);
    g_dockList[1].SetSize(g_clientWidth, 1);
    SaveLayout();
    CreateStatusBar(hInstance, g_hwnd, 0x1235, &g_rectStatus);
    g_hwndView = CreateGameView(hInstance, g_hwnd, 0x1234, &g_viewRectSpare);
    if (!g_hwndView)
        return 0;
    g_iconWnd = new IconWnd;
    r.top = g_mainIconY;
    r.left = g_iconXInfo;
    r.bottom = g_textHeight + 68;
    r.right = 604;
    g_iconWnd->Create(&r, g_hwnd, "Icon Wnd");
    SendMessage(g_hwnd, 0x600, 0, 0);
    SendMessage(g_hwnd, 0x703, 0, 0);
    RebuildPalette();
    SendMessage(g_hwnd, 0x702, 0, 0);
    g_mapWnd = new MapWnd;
    r.top = g_usedMapY;
    r.left = g_mapXUsed;
    r.right = 320;
    g_mapWnd->Create(&r, g_hwnd, "Map Wnd");
    g_dockList[0].count = 0;
    g_dockList[1].count = 0;
    if (g_savedIconDock != -1) {
        g_dockList[g_savedIconDock].Add(g_iconWnd);
        g_iconWnd->m_dock = &g_dockList[g_savedIconDock];
        g_iconWnd->DockIn();
    } else
        g_iconWnd->Undock();
    if (g_mapDock != -1) {
        g_dockList[g_mapDock].Add(g_mapWnd);
        g_mapWnd->m_dock = &g_dockList[g_mapDock];
        g_mapWnd->DockIn();
    } else
        g_mapWnd->Undock();
    g_dockList[0].Layout();
    g_dockList[1].Layout();
    SetZoom(g_hwnd, g_zoom);
    SelectSkill(g_hwnd, 12);
    ClearFastForward();
    ClearPause();
    ClearNuke();
    if (g_iconWnd->m_hwnd)
        SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff00, 0);
    SaveLayout();
    FitWindow(g_hwnd);
    g_iconWnd->Show();
    g_mapWnd->Show();
    ShowWindow(g_hwndView, nCmdShow);
    return 1;
}

/* 0x402760 */
void EndGame(void)
{
    SaveLayout();
    g_spareGameRunning = 0;
    DestroyWindow(g_hwndView);
    g_iconWnd->Destroy();
    delete g_iconWnd;
    g_mapWnd->Destroy();
    delete g_mapWnd;
    g_iconWnd = 0;
    g_mapWnd = 0;
    g_global2c = 0;
    g_hwndView = 0;
    ShowEndScreen();
}

/* 0x4027f0 */
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_MOVE:
        if (g_spareGameRunning)
            LayoutWindows(hwnd, g_clientWidth, g_valClientHeight);
        return 0;
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            if (g_spareGameRunning)
                LayoutWindows(hwnd, g_clientWidth, g_valClientHeight);
            FitWindow(hwnd);
        }
        if (g_iconWnd && g_mapWnd) {
            switch (wParam) {
            case SIZE_RESTORED:
                g_iconWnd->OnRestore();
                g_mapWnd->OnRestore();
                break;
            case SIZE_MINIMIZED:
                g_iconWnd->OnMinimize();
                g_mapWnd->OnMinimize();
                break;
            case SIZE_MAXIMIZED:
                g_iconWnd->OnMaximize();
                g_mapWnd->OnMaximize();
                break;
            }
        }
        break;
    case WM_SETFOCUS:
        SendMessage(g_hwnd, WM_NCACTIVATE, 1, 0);
        return 0;
    case WM_KILLFOCUS:
        break;
    case WM_ENDSESSION:
        SaveLayout();
        WriteIniSettings(g_szIniFile);
        break;
    case WM_ACTIVATEAPP:
        SendMessage(g_hwnd, WM_NCACTIVATE, wParam, 0);
        if (!wParam && g_pauseFocus) {
            SetMusic(0);
            PauseOnDeactivate();
        }
        if (wParam == 1 && g_pauseFocus && !g_musicPaused)
            SetMusic(g_music);
        return 0;
    case WM_COMMAND:
        return OnCommand(hwnd, msg, wParam, lParam);
    case WM_MENUSELECT:
        break;
    case 0x403:
        SendMessage(g_hwndView, 0x403, wParam, lParam);
        break;
    case 0x600:                 /* a level starts */
        g_spareGameRunning = 1;
        g_quitToMenu = 0;
    case 0x603:                 /* restart */
        CloseMidi();
        SetZoom(hwnd, g_zoom);
        SelectSkill(hwnd, 12);
        ClearFastForward();
        ClearPause();
        ClearNuke();
        SendMessage(g_hwndView, 0x402, 0, 0);
        SendMessage(g_hwndView, 0x401, 0, 0);
        if (g_iconWnd && g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff00, 0);
        break;
    case 0x601:                 /* a tool window docked or undocked */
        if (g_spareGameRunning)
            LayoutWindows(hwnd, g_clientWidth, g_valClientHeight);
        FitWindow(hwnd);
        break;
    case 0x602:
        EndGame();
        break;
    case 0x700:
        SaveLayout();
        if (g_iconWnd) {
            g_iconWnd->Destroy();
            delete g_iconWnd;
            g_iconWnd = 0;
        }
        if (g_mapWnd) {
            g_mapWnd->Destroy();
            delete g_mapWnd;
            g_mapWnd = 0;
        }
        break;
    case 0x701:
        g_spareGameRunning = 0;
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* 0x402bf0 */
LRESULT OnCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam)) {
    case 0x104:
        StartNuke(hwnd);
        break;
    case 0x113:
        SelectLevelDialog(hwnd);
        break;
    case 0x115:
        EndGame();
        DestroyWindow(hwnd);
        break;
    case 0x116:
        EndGame();
        break;
    case 0x118:
        SendMessage(g_hwnd, 0x603, 0, 0);
        break;
    case 0x13a:
        if (!WinHelp(hwnd, "LEMMINGS.HLP", HELP_KEY, (DWORD)"CONTENTS"))
            ErrorBox(0x7f01);
        break;
    case 0x13c:
        if (!WinHelp(hwnd, "LEMMINGS.HLP", HELP_PARTIALKEY, (DWORD)""))
            ErrorBox(0x7f01);
        break;
    case 0x13e:
        if (!WinHelp(hwnd, 0, HELP_HELPONHELP, 0))
            ErrorBox(0x7f01);
        break;
    case 0x140:
        DialogBoxParam(g_hInst, "AboutBox", hwnd, (DLGPROC)AboutDlgProc, 0);
        break;
    case 0x15e:
        SelectSkill(hwnd, ACT_CLIMBER);
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 0);
        break;
    case 0x15f:
        SelectSkill(hwnd, ACT_FLOATER);
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 1);
        break;
    case 0x160:
        SelectSkill(hwnd, ACT_EXPLODER);
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 2);
        break;
    case 0x161:
        SelectSkill(hwnd, ACT_BLOCKER);
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 3);
        break;
    case 0x162:
        SelectSkill(hwnd, ACT_BUILDER);
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 4);
        break;
    case 0x163:
        SelectSkill(hwnd, ACT_BASHER);
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 5);
        break;
    case 0x164:
        SelectSkill(hwnd, ACT_MINER);
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 6);
        break;
    case 0x165:
        SelectSkill(hwnd, ACT_DIGGER);
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 7);
        break;
    case 0x168:
        TogglePause(hwnd);
        break;
    case 0x169:
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 13);
        break;
    case 0x16a:
        ToggleFastForward(hwnd);
        break;
    case 0x16b:
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 9);
        if (g_replaying) {
            StopReplay();
            g_replaying = 0;
        }
        break;
    case 0x16c:
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 8);
        if (g_replaying) {
            StopReplay();
            g_replaying = 0;
        }
        break;
    case 0x177:
        DialogBoxParam(g_hInst, "OptionsDlg", hwnd, (DLGPROC)OptionsDlgProc, 0);
        break;
    case 0x188:
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 11);
        break;
    case 0x189:
        if (g_iconWnd->m_hwnd)
            SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 12);
        break;
    case 0x9c45:
        g_replaying = 1;
        SendMessage(g_hwnd, 0x603, 0, 0);
        break;
    case 0x9c46:
    case 0x9c48:
        g_quitToMenu = 1;
        EndGame();
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* 0x403130 */
void SelectSkill(HWND hwnd, int skill)
{
    if (skill >= 0 && skill < NUM_ACTIONS) {
        SendMessage(g_hwndView, 0x502, skill, 0);
        g_currentSkill = skill;
    }
}

/* 0x403160 */
void SetZoom(HWND hwnd, int zoom)
{
    if (zoom <= 4 && zoom >= 0) {
        g_zoom = zoom;
        SendMessage(g_hwndView, 0x501, zoom, 0);
    }
}

/* 0x403190 */
int FindDock(Dock *dock)
{
    int i;

    for (i = 0; i < g_numDocks; i++) {
        if (&g_dockList[i] == dock)
            return i;
    }
    return 0;
}

/* 0x4031c0: remember where everything is, for the ini file */
void SaveLayout(void)
{
    RECT window;
    POINT origin;
    RECT r;

    GetWindowRect(g_hwnd, &window);
    origin.x = 0;
    origin.y = 0;
    ClientToScreen(g_hwnd, &origin);
    window.left = origin.x;
    window.top = origin.y;
    g_rectStatus.left = 0;
    g_rectStatus.top = 0;
    g_rectStatus.right = g_clientWidth;
    g_rectStatus.bottom = GetStatusHeight();
    g_viewRectSpare.left = 0;
    g_viewRectSpare.top = 0;
    g_viewRectSpare.right = g_clientWidth;
    g_viewRectSpare.bottom = GetViewHeight(g_zoom);
    g_dockList[0].SetPos(0, 0);
    g_dockList[1].SetPos(0, g_valClientHeight - GetStatusHeight());
    if (g_mapWnd) {
        if (!g_mapWnd->m_docked)
            g_mapDock = -1;
        else
            g_mapDock = FindDock(g_mapWnd->m_dock);
    }
    if (g_iconWnd) {
        if (!g_iconWnd->m_docked)
            g_savedIconDock = -1;
        else
            g_savedIconDock = FindDock(g_iconWnd->m_dock);
    }
    g_dockList[0].Layout();
    g_dockList[1].Layout();
    if (g_iconWnd) {
        r = g_iconWnd->m_rect;
        g_iconXInfo = r.left;
        g_mainIconY = r.top;
    }
    if (g_mapWnd) {
        r = g_mapWnd->m_rect;
        g_mapXUsed = r.left;
        g_usedMapY = r.top;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_layoutwindows0, pad_layoutwindows1, pad_layoutwindows2, pad_layoutwindows3, pad_layoutwindows4, pad_layoutwindows5, pad_layoutwindows6, pad_layoutwindows7, pad_layoutwindows8, pad_layoutwindows9, pad_layoutwindows10, pad_layoutwindows11, pad_layoutwindows12, pad_layoutwindows13, pad_layoutwindows14, pad_layoutwindows15, pad_layoutwindows16, pad_layoutwindows17, pad_layoutwindows18, pad_layoutwindows19, pad_layoutwindows20, pad_layoutwindows21, pad_layoutwindows22, pad_layoutwindows23;

/* 0x4034f0 */
void LayoutWindows(HWND hwnd, int width, int height)
{
    SaveLayout();
    g_viewRectSpare.top += g_dockList[0].h + 2;
    SetWindowPos(g_hwndView, 0, g_viewRectSpare.left, g_viewRectSpare.top, g_viewRectSpare.right, g_viewRectSpare.bottom, SWP_NOZORDER);
    g_dockList[1].SetPos(0, g_viewRectSpare.top + g_viewRectSpare.bottom);
    g_dockList[1].Layout();
}

/* 0x403620 */
int GetDocksHeight(void)
{
    return g_dockList[1].h + g_dockList[0].h;
}

/* 0x403640 */
void ClearPause(void)
{
    g_paused = 0;
}

/* 0x403650 */
void TogglePause(HWND hwnd)
{
    if (g_paused ^= 1) {
        GetWindowText(hwnd, g_savedTitle, sizeof(g_savedTitle));
        SetWindowText(hwnd, GetString(0x112));
    } else
        SetWindowText(hwnd, g_savedTitle);
}

/* 0x4036a0 */
int IsPaused(void)
{
    return g_paused;
}

/* 0x4036b0 */
void PauseOnDeactivate(void)
{
    if (!g_paused && g_iconWnd && g_iconWnd->m_hwnd)
        SendMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff01, 11);
}

/* 0x4036f0 */
void ClearNuke(void)
{
    g_nuking = 0;
}

/* 0x403700 */
void StartNuke(HWND hwnd)
{
    g_nuking = 1;
    if (!g_replaying)
        RecordNuke(g_recordTime);
}

/* 0x403730 */
int IsNuking(void)
{
    return g_nuking;
}

/* 0x403740 */
void ClearFastForward(void)
{
    g_fastForward = 0;
}

/* 0x403750 */
void ToggleFastForward(HWND hwnd)
{
    g_fastForward ^= 1;
}

/* 0x403760 */
int IsFastForward(void)
{
    return g_fastForward;
}

/* 0x403770 */
void SetMenuHelpCallback(void (*callback)(void))
{
    g_ownMenuHelpCallback = callback;
}

/* 0x403780 */
void CallMenuHelp(void)
{
    if (g_ownMenuHelpCallback)
        g_ownMenuHelpCallback();
}

/* 0x403790: the status bar's help for the highlighted menu item */
void MenuSelect(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam)) {
    case 0x104:
        strcpy(g_helpMenu, GetString(0x130));
        break;
    case 0x10f:
        strcpy(g_helpMenu, GetString(0x11e));
        break;
    case 0x111:
        strcpy(g_helpMenu, GetString(0x11f));
        break;
    case 0x113:
        strcpy(g_helpMenu, GetString(0x11f));
        break;
    case 0x115:
        strcpy(g_helpMenu, GetString(0x120));
        break;
    case 0x116:
        strcpy(g_helpMenu, GetString(0x121));
        break;
    case 0x117:
        strcpy(g_helpMenu, GetString(0x122));
        break;
    case 0x13a:
        strcpy(g_helpMenu, GetString(0x123));
        break;
    case 0x13c:
        strcpy(g_helpMenu, GetString(0x124));
        break;
    case 0x13e:
        strcpy(g_helpMenu, GetString(0x125));
        break;
    case 0x140:
        strcpy(g_helpMenu, GetString(0x126));
        break;
    case 0x15e:
        strcpy(g_helpMenu, GetString(0x127));
        break;
    case 0x15f:
        strcpy(g_helpMenu, GetString(0x128));
        break;
    case 0x160:
        strcpy(g_helpMenu, GetString(0x129));
        break;
    case 0x161:
        strcpy(g_helpMenu, GetString(0x12a));
        break;
    case 0x162:
        strcpy(g_helpMenu, GetString(0x12b));
        break;
    case 0x163:
        strcpy(g_helpMenu, GetString(0x12c));
        break;
    case 0x164:
        strcpy(g_helpMenu, GetString(0x12d));
        break;
    case 0x165:
        strcpy(g_helpMenu, GetString(0x12e));
        break;
    case 0x168:
        strcpy(g_helpMenu, GetString(0x12f));
        break;
    case 0x16a:
        strcpy(g_helpMenu, GetString(0x131));
        break;
    case 0x16f:
        strcpy(g_helpMenu, GetString(0x133));
        break;
    case 0x171:
        strcpy(g_helpMenu, GetString(0x134));
        break;
    case 0x173:
        strcpy(g_helpMenu, GetString(0x135));
        break;
    default:
        strcpy(g_helpMenu, "");
        break;
    }
    CallMenuHelp();
}
