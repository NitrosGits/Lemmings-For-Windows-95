/*
 * The splash window: the logo while the game loads, with the registered
 * user's name.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14, pad_top15, pad_top16, pad_top17, pad_top18, pad_top19;

#include <windows.h>
#include <string.h>
#include "winlemm.h"

int g_unusedf4 = 0;
HFONT g_splashFont = 0;
HWND g_splashWnd = 0;
int g_unused00 = 0;
HBITMAP g_splashBitmap = 0;
HPALETTE g_splashPal = 0;
char g_splashClass[] = "SplashClass";

int g_splashH;
int g_splashW;
char g_splashTextGame[128];

/* 0x40d010 */
LRESULT CALLBACK SplashProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    HDC mem;
    HGDIOBJ oldBitmap;
    HGDIOBJ oldFont;

    switch (msg) {
    case WM_PAINT:
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        hdc = GetWindowDC(hwnd);
        mem = CreateCompatibleDC(hdc);
        oldBitmap = SelectObject(mem, g_splashBitmap);
        oldFont = SelectObject(hdc, g_splashFont);
        SelectPalette(hdc, g_splashPal, FALSE);
        RealizePalette(hdc);
        SetBkMode(hdc, TRANSPARENT);
        BitBlt(hdc, 0, 0, g_splashW, g_splashH, mem, 0, 0, SRCCOPY);
        TextOut(hdc, 18, 272, g_splashTextGame, strlen(g_splashTextGame));
        SelectObject(mem, oldFont);
        SelectObject(mem, oldBitmap);
        DeleteDC(mem);
        ReleaseDC(hwnd, hdc);
        return 0;
    case WM_NCCALCSIZE:         /* no caption */
        DefWindowProc(hwnd, msg, wParam, lParam);
        ((RECT *)lParam)->top -= GetSystemMetrics(SM_CYCAPTION);
        return 0;
    case WM_NCPAINT:
    case WM_NCACTIVATE:
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* 0x40d200 */
int RegisterSplashClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hIcon = 0;
    wc.lpszMenuName = 0;
    wc.hInstance = hInstance;
    wc.style = 0;
    wc.cbWndExtra = 0;
    wc.cbClsExtra = 0;
    wc.lpszClassName = g_splashClass;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpfnWndProc = SplashProc;
    return RegisterClass(&wc);
}

/* 0x40d260 */
HWND CreateSplash(HINSTANCE hInstance)
{
    PDIB dib = ReadDib("gfx\\wlemlogo.bmp");
    int w;
    int h;

    if (dib) {
        g_splashBitmap = Dib2HBITMAP(dib, &g_splashPal);
        g_splashW = GetDibWidth(dib);
        g_splashH = GetDibHeight(dib);
        w = g_splashW;
        h = g_splashH;
        FreeDib(dib);
        return CreateWindowEx(WS_EX_TOPMOST, g_splashClass, 0, WS_BORDER, CW_USEDEFAULT, CW_USEDEFAULT,
                              w, h, 0, 0, hInstance, 0);
    }
    return 0;
}

/* 0x40d2f0 */
void ShowSplash(HINSTANCE hInstance)
{
    HDC hdc;
    int height;

    g_splashWnd = 0;
    g_splashBitmap = 0;
    g_splashPal = 0;
    if (RegisterSplashClass(hInstance)) {
        hdc = GetDC(0);
        height = MulDiv(-8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        g_splashFont = CreateFont(height, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0, 0, 0x26, "MS Sans Serif");
        ReleaseDC(0, hdc);
        if (IsTaster())
            strcpy(g_splashTextGame, "");
        else
            wsprintf(g_splashTextGame, "Registered to : %s", g_regUserUsed);
        g_splashWnd = CreateSplash(hInstance);
        CentreWindow(g_splashWnd, GetDesktopWindow());
        ShowWindow(g_splashWnd, SW_SHOW);
        UpdateWindow(g_splashWnd);
    }
}

/* 0x40d3f0 */
void HideSplash(void)
{
    HDC hdc;
    RECT r;
    HBRUSH brush;

    if (g_splashWnd) {
        hdc = GetDC(g_splashWnd);
        GetClientRect(g_splashWnd, &r);
        brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        SelectObject(hdc, brush);
        PatBlt(hdc, r.top, r.left, r.right, r.bottom, PATCOPY);
        DeleteObject(brush);
        ReleaseDC(g_splashWnd, hdc);
        DestroyWindow(g_splashWnd);
        DeleteObject(g_splashBitmap);
        DeleteObject(g_splashPal);
        if (g_splashFont)
            DeleteObject(g_splashFont);
    }
}
