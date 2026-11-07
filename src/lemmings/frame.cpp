/*
 * The outer frame window ("OuterFrame"): the status bar along its bottom,
 * the game's palette, and a window procedure that hands the client area
 * to whichever screen is showing (title or game).
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13;

#include <windows.h>
#include <string.h>
#include "winlemm.h"

int g_musicPaused = 0;
HPALETTE g_hPalette = 0;
struct {
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[256];
} g_logPal = {0x300, 256};
WNDPROC g_mainProc = 0;
char g_frameClass[] = "OuterFrame";
int g_paintCount = 0;
PANE g_mainPanes[2] = {
    {-1, 4, g_statusText},
    {0},
};
STATBAR g_mainBar;
STATBAR *g_curBar = &g_mainBar;
PANE *g_curPanes = g_mainPanes;

RECT g_frameAdjust;             /* the frame's size beyond its client area */
BYTE g_stateUnused980[256];              /* unused: fills a gap in the original .bss */
PALETTEENTRY g_sysPalSpare[256];
RGBQUAD g_tempPalette[256];
RGBQUAD g_palette2[256];
int g_clientWidth, g_valClientHeight;
int g_wViewClient, g_clientViewH;
int g_unused6d4All;                      /* unused: fills a gap in the original .bss */

/* 0x4085b0 */
int RegisterFrameClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = FrameProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, g_szAppName);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = 0;
    wc.lpszClassName = g_frameClass;
    return RegisterClass(&wc);
}

/* 0x408630 */
HWND FindPreviousInstance(void)
{
    return FindWindow(g_frameClass, 0);
}

/* 0x408640 */
HWND CreateMainWindow(HINSTANCE hInstance)
{
    char title[256];
    HWND hwnd;
    int show;
    DWORD style;
    int w;
    int h;

    g_clientWidth = 0;
    g_valClientHeight = 0;
    h = g_mainHeightOldTemp;
    w = g_mainWidthNext;
    show = g_maximised ? SW_SHOWMAXIMIZED : SW_SHOW;
    style = g_maximised ? WS_MAXIMIZE : 0;
    wsprintf(title, GetString(0x110), g_userName);
    hwnd = CreateWindowEx(0, g_frameClass, title, style | WS_VISIBLE | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT, w, h, 0, 0, hInstance, 0);
    g_global28 = (int)CreateStatBar(g_hInst, hwnd, 0, &g_rectStatus, &g_mainBar, g_mainPanes);
    g_frameAdjust.top = 0;
    g_frameAdjust.left = 0;
    g_frameAdjust.right = g_frameAdjust.bottom = 100;
    AdjustWindowRect(&g_frameAdjust, WS_OVERLAPPEDWINDOW, TRUE);
    g_frameAdjust.bottom -= 100;
    g_frameAdjust.right -= 100;
    g_frameAdjust.bottom += GetSystemMetrics(SM_CYBORDER);
    g_frameAdjust.right -= g_frameAdjust.left;
    g_frameAdjust.bottom -= g_frameAdjust.top;
    FitWindow(hwnd);
    ShowWindow(hwnd, show);
    return hwnd;
}

/* 0x408790 */
LRESULT CALLBACK FrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    HPALETTE old;
    RECT r;

    switch (msg) {
    case WM_CREATE:
        g_paintCount = 0;
        return 0;
    case WM_DESTROY:
        {
            g_musicPaused = 1;
            if (g_mainProc)
                SendMessage(hwnd, 0x700, 0, 0);
            if (g_hPalette) {
                DeleteObject(g_hPalette);
                g_hPalette = 0;
            }
            ShowStatBar(0, 0);
            GetWindowRect(g_hwnd, &r);
            g_mainWidthNext = r.right - r.left;
            g_mainHeightOldTemp = r.bottom - r.top;
            PostQuitMessage(0);
        }
        return 0;
    case WM_SIZE:
        g_clientWidth = LOWORD(lParam);
        g_valClientHeight = HIWORD(lParam);
        g_valClientHeight -= GetBarHeight(g_curBar) + 1;
        if (g_global28)
            SetWindowPos((HWND)g_global28, 0, 0, g_valClientHeight, g_clientWidth, GetBarHeight(g_curBar),
                         SWP_NOZORDER);
        switch (wParam) {
        case SIZE_RESTORED:
            g_maximised = 0;
            break;
        case SIZE_MINIMIZED:
            PauseOnDeactivate();
            g_maximised = 0;
            break;
        case SIZE_MAXIMIZED:
            g_maximised = 1;
            break;
        }
        break;
    case WM_PAINT:
        if (g_paintCount > 0) {
            PAINTSTRUCT ps;

            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            g_paintCount = 0;
            InvalidateRect(hwnd, 0, TRUE);
            return 0;
        }
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO *mm = (MINMAXINFO *)lParam;
            int scale = g_scale * g_resolutionNew;

            mm->ptMaxSize.y = GetFrameHeight() + g_frameAdjust.bottom;
            mm->ptMinTrackSize.y = GetFrameHeight() + g_frameAdjust.bottom;
            mm->ptMaxTrackSize.y = GetFrameHeight() + g_frameAdjust.bottom;
            mm->ptMinTrackSize.x = (GetStatusWidth() + g_frameAdjust.right) & ~(scale - 1);
        }
        return 0;
    case WM_COMMAND:
        if ((wParam & 0xffff) == 0x186)
            ShowWindow(hwnd, SW_MINIMIZE);
        break;
    case WM_MENUSELECT:
        MenuSelect(hwnd, msg, wParam, lParam);
        break;
    case WM_QUERYNEWPALETTE:
        {
            int realized;

            if (g_hPalette) {
                hdc = GetDC(hwnd);
                old = SelectPalette(hdc, g_hPalette, FALSE);
                realized = RealizePalette(hdc);
                SelectPalette(hdc, old, FALSE);
                RealizePalette(hdc);
                ReleaseDC(hwnd, hdc);
                if (realized) {
                    InvalidateRect(hwnd, 0, FALSE);
                    g_paintCount = 0;
                }
            }
            return realized;
        }
    case WM_PALETTEISCHANGING:
        return 0;
    case WM_PALETTECHANGED:
        if ((HWND)wParam != hwnd && g_hPalette) {
            hdc = GetDC(hwnd);
            old = SelectPalette(hdc, g_hPalette, TRUE);

            if (RealizePalette(hdc)) {
                g_paintCount++;
                if (g_paintCount < 2)
                    UpdateColors(hdc);
                else {
                    g_paintCount = 0;
                    InvalidateRect(hwnd, 0, TRUE);
                }
            }
            SelectPalette(hdc, old, TRUE);
            RealizePalette(hdc);
            ReleaseDC(hwnd, hdc);
        }
        return 0;
    case 0x702:                 /* the palette changed: make Windows take it */
        {
            HPALETTE old2;

            UnrealizeObject(g_hPalette);
            ResetSystemPalette();
            hdc = GetDC(hwnd);
            old = SelectPalette(hdc, g_hPalette, FALSE);
            old2 = SelectPalette(hdc, g_hPalette, TRUE);
            RealizePalette(hdc);
            SelectPalette(hdc, old2, FALSE);
            SelectPalette(hdc, old, FALSE);
            RealizePalette(hdc);
            ReleaseDC(hwnd, hdc);
        }
        break;
    case 0x703:                 /* clear the client area */
        {
            RECT r2;
            HBRUSH brush;

            hdc = GetDC(hwnd);
            GetClientRect(hwnd, &r2);
            brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            SelectObject(hdc, brush);
            PatBlt(hdc, r2.top, r2.left, g_clientWidth, g_valClientHeight, PATCOPY);
            DeleteObject(brush);
            ReleaseDC(hwnd, hdc);
        }
        break;
    case 0x704:
        InitFrontEnd();
        break;
    }
    if (!g_mainProc)
        return DefWindowProc(hwnd, msg, wParam, lParam);
    return g_mainProc(hwnd, msg, wParam, lParam);
}

/* 0x408cc0: hand the client area to another screen */
void SetMainProc(WNDPROC proc)
{
    if (proc) {
        if (g_mainProc)
            SendMessage(g_hwnd, 0x700, 0, 0);
        g_mainProc = proc;
        SendMessage(g_hwnd, 0x701, 0, 0);
        InvalidateRect(g_hwnd, 0, FALSE);
    }
}

/* 0x408d30 */
HMENU LoadGameMenu(char *name)
{
    HMENU menu = GetMenu(g_hwnd);

    if (menu) {
        SetMenu(g_hwnd, 0);
        DestroyMenu(menu);
    }
    menu = LoadMenu(g_hInst, name);
    SetMenu(g_hwnd, menu);
    return menu;
}

/* 0x408d90 */
void ShowStatBar(STATBAR *bar, PANE *panes)
{
    ResetStatBar((HWND)g_global28, bar, panes);
    g_curBar = bar;
    g_curPanes = panes;
}

/* 0x408dc0: change a colour until no earlier entry has it */
void MakeUnique(BYTE *e, LOGPALETTE *pal, int count)
{
    int changed;
    int i;

    do {
        changed = 0;
        for (i = 0; i < count; i++) {
            if (e[0] == 0xff && e[1] == 0xff && e[2] == 0xff && count != 255) {
                e[2]--;
                changed = 1;
                break;
            }
            if (pal->palPalEntry[i].peRed == e[0] && e[1] == pal->palPalEntry[i].peGreen &&
                pal->palPalEntry[i].peBlue == e[2]) {
                e[0]++;
                changed = 1;
                break;
            }
        }
    } while (changed);
}

/* 0x408e30: the system's 20 colours round the given ones, all different */
HPALETTE BuildPalette(RGBQUAD *colours, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        g_logPal.palPalEntry[i].peRed = colours[i].rgbRed;
        g_logPal.palPalEntry[i].peGreen = colours[i].rgbGreen;
        g_logPal.palPalEntry[i].peBlue = colours[i].rgbBlue;
        g_logPal.palPalEntry[i].peFlags = 0;
    }
    memcpy(&g_logPal.palPalEntry[0], &g_sysPalSpare[0], 10 * sizeof(PALETTEENTRY));
    memcpy(&g_logPal.palPalEntry[246], &g_sysPalSpare[246], 10 * sizeof(PALETTEENTRY));
    for (i = 0; i < 256; i++)
        MakeUnique((BYTE *)&g_logPal.palPalEntry[i], (LOGPALETTE *)&g_logPal, i);
    for (i = 0; i < n; i++) {
        colours[i].rgbRed = g_logPal.palPalEntry[i].peRed;
        colours[i].rgbGreen = g_logPal.palPalEntry[i].peGreen;
        colours[i].rgbBlue = g_logPal.palPalEntry[i].peBlue;
    }
    return CreatePalette((LOGPALETTE *)&g_logPal);
}

/* 0x408ef0 */
HPALETTE CopyPalette(HPALETTE pal)
{
    int i;

    GetPaletteEntries(pal, 0, 256, g_logPal.palPalEntry);
    for (i = 0; i < 256; i++)
        g_logPal.palPalEntry[i].peFlags = 0;
    return CreatePalette((LOGPALETTE *)&g_logPal);
}

/* 0x408f30 */
void SetGamePalette(HPALETTE pal)
{
    if (g_hPalette) {
        DeleteObject(g_hPalette);
        g_hPalette = 0;
    }
    g_hPalette = CopyPalette(pal);
}

/* 0x408f60 */
void RebuildPalette(void)
{
    if (g_hPalette) {
        DeleteObject(g_hPalette);
        g_hPalette = 0;
    }
    g_hPalette = BuildPalette(g_palette2, 256);
}

/* 0x408fa0: realise an all-black palette, so the next one is taken whole */
void ResetSystemPalette(void)
{
    struct {
        WORD palVersion;
        WORD palNumEntries;
        PALETTEENTRY palPalEntry[256];
    } lp;
    HDC hdc;
    HPALETTE pal;
    HPALETTE old;
    int i;

    lp.palVersion = 0x300;
    lp.palNumEntries = 256;
    memset(lp.palPalEntry, 0, sizeof(lp.palPalEntry));
    for (i = 0; i < 256; i++) {
        lp.palPalEntry[i].peRed = 0;
        lp.palPalEntry[i].peGreen = 0;
        lp.palPalEntry[i].peBlue = 0;
        lp.palPalEntry[i].peFlags = PC_NOCOLLAPSE;
    }
    hdc = GetDC(0);
    pal = CreatePalette((LOGPALETTE *)&lp);
    old = SelectPalette(hdc, pal, FALSE);
    RealizePalette(hdc);
    DeleteObject(SelectPalette(hdc, old, FALSE));
    ReleaseDC(0, hdc);
}

/* 0x409040 */
void InitPalette(void)
{
    HDC hdc;
    PDIB dib;
    int i;

    ResetSystemPalette();
    hdc = GetDC(0);
    GetSystemPaletteEntries(hdc, 0, 256, g_sysPalSpare);
    ReleaseDC(0, hdc);
    for (i = 0; i < 256; i++)
        g_sysPalSpare[i].peFlags = 0;
    dib = ReadDib("gfx\\icons.bmp");
    memcpy(g_tempPalette, (BYTE *)dib + sizeof(BITMAPINFOHEADER), sizeof(g_tempPalette));
    FreeDib(dib);
    memcpy(g_palette2, g_tempPalette, sizeof(g_palette2));
    RebuildPalette();
}

/* 0x4090c0 */
int GetFrameWidth(void)
{
    int scale = g_scale * g_resolutionNew;

    if (GetStatusWidth() > g_clientWidth)
        return GetStatusWidth() & ~(scale - 1);
    return g_clientWidth;
}

/* 0x4090f0 */
int GetFrameHeight(void)
{
    int h;
    int result;

    if (g_hwndView) {
        h = GetDocksHeight() + 4;
        result = GetStatusHeight();
        result += h;
        result += g_clientViewH;
        return result;
    }
    return GetBarHeight(g_curBar) + 401;
}

/* 0x409130: size the frame round its contents */
void FitWindow(HWND hwnd)
{
    int w;
    int h;

    if (GetFrameWidth() != g_clientWidth && GetFrameHeight() != g_valClientHeight) {
        w = g_frameAdjust.right + GetFrameWidth();
        h = GetFrameHeight() + g_frameAdjust.bottom;
        SetWindowPos(hwnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
    } else if (GetFrameWidth() != g_clientWidth) {
        w = g_frameAdjust.right + GetFrameWidth();
        h = g_valClientHeight + g_frameAdjust.bottom;
        SetWindowPos(hwnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
    } else if (GetFrameHeight() != g_valClientHeight) {
        w = g_frameAdjust.right + g_clientWidth;
        h = GetFrameHeight() + g_frameAdjust.bottom;
        SetWindowPos(hwnd, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
    }
}
