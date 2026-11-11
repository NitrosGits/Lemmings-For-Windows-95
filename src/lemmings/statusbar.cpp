/*
 * The status bar: a small window class ("StatBar") drawing sunken panes
 * from right to left, and the game's four panes (time, home, out and the
 * help text).
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14, pad_top15, pad_top16, pad_top17, pad_top18, pad_top19, pad_top20, pad_top21;

#include <windows.h>
#include <string.h>
#include "winlemm.h"

char g_statusText[256] = "Welcome to Lemmings";
char g_statBarClass[] = "StatBar";
char g_statusFontName[] = "MS Sans Serif";
HFONT g_hStatFont = 0;
int g_unused52c = 0;
int g_statusOn = 0;
HWND g_hStatus = 0;
char g_timeText[16] = "Time: 00:00";
char g_homeText[16] = "Home: 00";
char g_outText[8] = "Out: 00";
PANE g_panes[5] = {
    {12, 4, g_timeText},
    {9, 4, g_homeText},
    {8, 4, g_outText},
    {-1, 4, g_statusText},
    {0},
};

STATBAR g_statBar;

/* 0x403f60 */
int RegisterStatBar(HINSTANCE hInstance, STATBAR *bar, PANE *panes)
{
    WNDCLASS wc;

    InitStatBar(bar, panes);
    DeleteObject(g_hStatFont);
    g_hStatFont = 0;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hIcon = 0;
    wc.lpszMenuName = 0;
    wc.hInstance = hInstance;
    wc.style = 0;
    wc.lpszClassName = g_statBarClass;
    wc.cbClsExtra = 0;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpfnWndProc = StatBarProc;
#ifdef PORT_SDL                 /* 64-bit: room for the pointer kept there (CreateStatBar; PORTING.md) */
    wc.cbWndExtra = sizeof(STATBAR *);
#else
    wc.cbWndExtra = 4;
#endif
    return RegisterClass(&wc);
}

/* 0x403ff0 */
HWND CreateStatBar(HINSTANCE hInstance, HWND parent, int id, RECT *r, STATBAR *bar, PANE *panes)
{
    HWND hwnd;

    InitStatBar(bar, panes);
    hwnd = CreateWindowEx(0, g_statBarClass, 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, r->left, r->top,
                          r->right, r->bottom, parent, (HMENU)id, hInstance, 0);
    if (hwnd)
        PORT_SetWindowLong(hwnd, 0, (PORT_LONG)bar);
    return hwnd;
}

/* 0x404060 */
void ResetStatBar(HWND hwnd, STATBAR *bar, PANE *panes)
{
    SendMessage(hwnd, 0x700, 0, 0);
    if (bar && panes) {
        InitStatBar(bar, panes);
        SendMessage(hwnd, 0x701, 0, 0);
        PORT_SetWindowLong(hwnd, 0, (PORT_LONG)bar);
        InvalidateRect(hwnd, 0, FALSE);
    }
}

/* 0x4040c0 */
LRESULT CALLBACK StatBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        StatBarCreate(hwnd);
        return 0;
    case WM_DESTROY:
        StatBarDestroy(hwnd);
        return 0;
    case WM_SIZE:
        InvalidateRect(hwnd, 0, FALSE);
        return 0;
    case WM_PAINT:
        StatBarPaint(hwnd);
        return 0;
    case 0x700:
        StatBarDestroy(hwnd);
        return 0;
    case 0x701:
        StatBarCreate(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* 0x404180 */
void StatBarDestroy(HWND hwnd)
{
    STATBAR *bar = (STATBAR *)PORT_GetWindowLong(hwnd, 0);

    if (bar && bar->font) {
        DeleteObject(bar->font);
        bar->font = 0;
    }
}

/* 0x4041b0 */
void StatBarCreate(HWND hwnd)
{
    PORT_SetWindowLong(hwnd, 0, 0);
}

/* 0x4041c0 */
void StatBarPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    STATBAR *bar = (STATBAR *)PORT_GetWindowLong(hwnd, 0);
    HDC hdc = BeginPaint(hwnd, &ps);

    if (bar)
        DrawStatBar(hwnd, hdc, bar);
    EndPaint(hwnd, &ps);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawpanetext0, pad_drawpanetext1, pad_drawpanetext2, pad_drawpanetext3, pad_drawpanetext4, pad_drawpanetext5, pad_drawpanetext6, pad_drawpanetext7, pad_drawpanetext8;

#ifdef PORT_SDL
/* The native build: a system view shows the bar, told its panes' texts
   whenever the bar or one pane is drawn (PORTING.md). 1 if it does. */
static int NativeStatBar(HWND hwnd, STATBAR *bar)
{
    const char *texts[8];
    int n = 0;
    PANE *pane;

    for (pane = bar->panes; pane->text && n < 8; pane++)
        texts[n++] = pane->text;
    return port_native_status(hwnd, texts, n);
}
#endif

/* 0x404210 */
void DrawPaneText(HDC hdc, STATBAR *bar, PANE *pane)
{
#ifdef PORT_SDL
    if (g_hStatus && NativeStatBar(g_hStatus, bar))
        return;
#endif
    HFONT old = (HFONT)SelectObject(hdc, bar->font);
    RECT r;

    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    r.top = pane->rect.top + 4 * bar->border;
    r.bottom = pane->rect.bottom - bar->border3;
    r.left = bar->border + pane->rect.left;
    r.right = pane->textWidth - bar->border + pane->rect.left;
    ExtTextOut(hdc, r.left + bar->border2, r.top, ETO_OPAQUE | ETO_CLIPPED, &r, pane->text, lstrlen(pane->text), 0);
    SelectObject(hdc, old);
}

/* 0x4042c0 */
int PaneWidth(STATBAR *bar, PANE *pane)
{
    return bar->charWidth * pane->width;
}

/* 0x4042d0: the next pane leftwards from r->left */
void PlacePane(STATBAR *bar, PANE *pane, RECT *r)
{
    int w = PaneWidth(bar, pane);

    if (w < 0)
        w = r->left - pane->border * 2;
    r->right = r->left;
    r->left = r->left - pane->border - w;
    {
        RECT *d = &pane->rect;
        *d = *r;
    }
    pane->textWidth = w;
}

/* 0x404320: a sunken box */
void DrawPane(HDC hdc, STATBAR *bar, PANE *pane)
{
    int w = pane->textWidth;
    HBRUSH brush;
    RECT r;

    brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    r = pane->rect;
    r.bottom = bar->border3 + r.top;
    r.top += bar->border2;
    FillRect(hdc, &r, brush);
    r = pane->rect;
    r.top = r.bottom - bar->border2;
    FillRect(hdc, &r, brush);
    r = pane->rect;
    r.left = pane->rect.right - pane->border;
    FillRect(hdc, &r, brush);
    DeleteObject(brush);

    brush = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
    r.top = pane->rect.top + bar->border3;
    r.bottom = bar->border + r.top;
    r.left = pane->rect.left;
    r.right = r.left + w;
    FillRect(hdc, &r, brush);
    r = pane->rect;
    r.top += bar->border3;
    r.bottom -= bar->border2;
    r.right = bar->border + r.left;
    FillRect(hdc, &r, brush);
    DeleteObject(brush);

    if (g_win32)
        brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    else
        brush = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT));
    r = pane->rect;
    r.top = pane->rect.bottom - bar->border3;
    r.bottom = bar->border + r.top;
    r.left = pane->rect.left;
    r.right = r.left + w;
    FillRect(hdc, &r, brush);
    r = pane->rect;
    r.top += bar->border3;
    r.bottom -= bar->border2;
    r.left += w - bar->border;
    r.right = bar->border + r.left;
    FillRect(hdc, &r, brush);
    r = pane->rect;
    r.top += bar->border;
    r.bottom = bar->border + r.top;
    FillRect(hdc, &r, brush);
    DeleteObject(brush);

    brush = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT));
    r = pane->rect;
    r.bottom = bar->border + r.top;
    FillRect(hdc, &r, brush);
    DeleteObject(brush);
    DrawPaneText(hdc, bar, pane);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawbarborder0, pad_drawbarborder1, pad_drawbarborder2, pad_drawbarborder3, pad_drawbarborder4, pad_drawbarborder5, pad_drawbarborder6, pad_drawbarborder7;

/* 0x4045f0: the bar left of the panes */
void DrawBarBorder(HDC hdc, STATBAR *bar, RECT *rect)
{
    HBRUSH brush;
    RECT r;

    brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    r = *rect;
    FillRect(hdc, &r, brush);
    DeleteObject(brush);
    if (g_win32)
        brush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    else
        brush = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT));
    r = *rect;
    r.top += bar->border;
    r.bottom = bar->border + r.top;
    FillRect(hdc, &r, brush);
    DeleteObject(brush);
    brush = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT));
    r = *rect;
    r.bottom = bar->border + r.top;
    FillRect(hdc, &r, brush);
    DeleteObject(brush);
}

/* 0x404700 */
void DrawStatBar(HWND hwnd, HDC hdc, STATBAR *bar)
{
    RECT r;
    PANE *pane;

#ifdef PORT_SDL
    if (NativeStatBar(hwnd, bar))
        return;
#endif
    GetClientRect(hwnd, &r);
    r.top = r.bottom - bar->height;
    r.left = r.right;
    for (pane = bar->panes; pane->text; pane++) {
        PlacePane(bar, pane, &r);
        DrawPane(hdc, bar, pane);
    }
    r.right = r.left;
    r.left = 0;
    DrawBarBorder(hdc, bar, &r);
}

/* 0x404790 */
void InitStatBar(STATBAR *bar, PANE *panes)
{
    HFONT old;
    TEXTMETRIC tm;
    HDC hdc;
    int height;

    bar->border = GetSystemMetrics(SM_CYBORDER);
    bar->border2 = 2 * bar->border;
    bar->border3 = 3 * bar->border;
    bar->border8 = 8 * bar->border;
    bar->border9 = 9 * bar->border;
    hdc = GetDC(0);
    height = MulDiv(-10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    bar->font = g_hStatFont = CreateFont(height, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, FF_SWISS | 6,
                                         g_statusFontName);
    old = (HFONT)SelectObject(hdc, bar->font);
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, old);
    ReleaseDC(0, hdc);
    bar->height = 7 * bar->border + tm.tmExternalLeading + tm.tmHeight;
    bar->charWidth = (tm.tmAveCharWidth + tm.tmMaxCharWidth) / 2;
    bar->panes = panes;
}

/* 0x404870 */
int GetBarHeight(STATBAR *bar)
{
    return bar->height;
}

/* 0x404880: the bar's height before it exists */
int CalcStatusHeight(void)
{
    TEXTMETRIC tm;
    HDC hdc = GetDC(0);
    int height = MulDiv(-10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    HFONT font = CreateFont(height, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, FF_SWISS | 6, g_statusFontName);

    SelectObject(hdc, font);
    GetTextMetrics(hdc, &tm);
    DeleteObject(font);
    ReleaseDC(0, hdc);
    return 7 * GetSystemMetrics(SM_CYBORDER) + tm.tmExternalLeading + tm.tmHeight;
}

/* 0x404920 */
int TotalPaneWidth(STATBAR *bar)
{
    int total = 0;
    PANE *pane = bar->panes;
    PANE *last = 0;

    while (pane->text) {
        if (pane->width < 0)
            total += pane->border + 320;
        else
            total += pane->border + PaneWidth(bar, pane);
        last = pane;
        pane++;
    }
    if (last)
        total += last->border;
    return total;
}

/* 0x404970 */
int RegisterStatusClass(HINSTANCE hInstance)
{
    return RegisterStatBar(hInstance, &g_statBar, g_panes);
}

/* 0x404990 */
void CreateStatusBar(HINSTANCE hInstance, HWND parent, int id, RECT *rect)
{
    ShowStatBar(&g_statBar, g_panes);
    g_hStatus = (HWND)g_global28;
    g_statusOn = 1;
}

/* 0x4049c0 */
int GetStatusHeight(void)
{
    return GetBarHeight(&g_statBar);
}

/* 0x4049d0 */
int GetStatusWidth(void)
{
    return TotalPaneWidth(&g_statBar);
}

/* 0x4049e0 */
void SetStatusTime(int min, int sec)
{
    char text[80];
    HDC hdc;

    wsprintf(text, "Time: %02d:%02d", min, sec);
    if (strcmp(text, g_timeText)) {
        strcpy(g_timeText, text);
        hdc = GetDC(g_hStatus);
        DrawPaneText(hdc, &g_statBar, &g_panes[0]);
        ReleaseDC(g_hStatus, hdc);
    }
}

/* 0x404a90: the "Save n from m" pane was dropped */
void SetStatusSave(int save, int total)
{
    char *unused = "Save %02d from %02d lemmings";
}

/* 0x404aa0 */
void SetStatusMessage(char *text)
{
    HDC hdc;

    if (strcmp(g_statusText, text)) {
        strcpy(g_statusText, text);
        hdc = GetDC(g_hStatus);
        DrawPaneText(hdc, &g_statBar, &g_panes[3]);
        ReleaseDC(g_hStatus, hdc);
    }
}

/* 0x404b30 */
void SetStatusHome(int home)
{
    char text[80];
    HDC hdc;

    wsprintf(text, "Home: %02d", home);
    if (strcmp(text, g_homeText)) {
        strcpy(g_homeText, text);
        hdc = GetDC(g_hStatus);
        DrawPaneText(hdc, &g_statBar, &g_panes[1]);
        ReleaseDC(g_hStatus, hdc);
    }
}

/* 0x404be0 */
void SetStatusOut(int out)
{
    char text[80];
    HDC hdc;

    wsprintf(text, "Out: %02d", out);
    if (strcmp(g_outText, text)) {
        strcpy(g_outText, text);
        hdc = GetDC(g_hStatus);
        DrawPaneText(hdc, &g_statBar, &g_panes[2]);
        ReleaseDC(g_hStatus, hdc);
    }
}

/* 0x404c90 */
void SetStatusText(char *s)
{
    char text[80];
    HDC hdc;

    wsprintf(text, "%s", s);
    if (strcmp(text, g_statusText)) {
        strcpy(g_statusText, text);
        hdc = GetDC(g_hStatus);
        DrawPaneText(hdc, &g_statBar, &g_panes[3]);
        ReleaseDC(g_hStatus, hdc);
    }
}
