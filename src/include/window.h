/*
 * LEMMINGS.EXE's window classes: a C++ wrapper round one registered window
 * class, the tool windows that dock into the main window (icon panel, map)
 * and the buttons on the icon panel.
 */
#ifndef WINDOW_H
#define WINDOW_H

#include <windows.h>
#include <string.h>
#include "winlemm.h"

class Window {
public:
    HWND m_hwnd;
    HWND m_parent;
    RECT m_rect;                /* left/top hold the position, right/bottom the size */
    int m_state;                /* 0 minimised, 1 maximised, 2 restored */
    HCURSOR m_cursor;
    COLORREF m_bkColor;

    Window();
    ~Window() {}
    virtual void Create(RECT *rect, HWND parent, char *name);
    virtual DWORD GetStyle() { return WS_OVERLAPPEDWINDOW; }
    virtual void Move(RECT *rect, UINT flags)
    {
        if (m_hwnd)
            SetWindowPos(m_hwnd, 0, rect->left, rect->top, rect->right, rect->bottom, flags | SWP_NOZORDER);
    }
    virtual void OnCreate() {}
    virtual void OnDestroy() {}
    virtual void Paint(HDC hdc, RECT *rect) {}
    virtual void Redraw(RECT *rect)
    {
        HDC hdc = GetDC(m_hwnd);

        Paint(hdc, rect);
        ReleaseDC(m_hwnd, hdc);
    }
    virtual void OnSize() {}
    virtual void OnMinimize() {}
    virtual void OnMaximize() {}
    virtual void OnRestore() {}
    virtual void OnMove() {}
    virtual void Show()
    {
        if (m_hwnd)
            ShowWindow(m_hwnd, SW_SHOW);
    }
    virtual void Hide()
    {
        if (m_hwnd)
            ShowWindow(m_hwnd, SW_HIDE);
    }
    virtual void Minimize()
    {
        if (m_hwnd)
            ShowWindow(m_hwnd, SW_MINIMIZE);
    }
    virtual void Restore()
    {
        if (m_hwnd)
            ShowWindow(m_hwnd, SW_RESTORE);
    }
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (m_hwnd)
            return DefWindowProc(m_hwnd, msg, wParam, lParam);
        return 0;
    }
    void Destroy();
};

class DockWnd;
class IconWnd;
class MapWnd;

/* A strip of the main window that tool windows dock into, side by side */
struct Dock {
    int x, y, w, h;
    DockWnd *wnds[2];
    int count;
    RECT screen;                /* x, y, w, h on the screen while dragging */

    Dock() { count = 0; }
    void SetPos(int ax, int ay)
    {
#ifdef MAINWND_PAD
        int v0;
        int v1;
        int v2;
        int v3;
        int v4;
        int v5;
#endif
        x = ax;
        y = ay;
    }
    void SetSize(int aw, int ah)
    {
        w = aw;
        h = ah;
    }
    void Add(DockWnd *wnd)
    {
        wnds[count] = wnd;
        count++;
    }
    void Layout();
};

/* A tool window that can float or dock into one of the main window's docks */
class DockWnd : public Window {
public:
    Dock *m_dock;
    RECT m_wndRect;             /* its window rectangle while dragging */
    int m_unused3c;
    int m_docked;
    int m_hidden;               /* hidden while the main window is minimised */

    DockWnd();
    ~DockWnd();
    virtual DWORD GetStyle() { return WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER; }
    virtual void OnMinimize()
    {
        if (m_docked) {
            ShowWindow(m_hwnd, SW_HIDE);
            m_hidden = 1;
        }
    }
    virtual void OnMaximize() { OnRestore(); }
    virtual void OnRestore()
    {
        if (m_hidden) {
            ShowWindow(m_hwnd, SW_SHOW);
            m_hidden = 0;
        }
    }
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
    virtual void ClampPosition(POINT *pt);
    virtual void OnDock(RECT *rect, int docked);
    void DrawFrame(HDC hdc, int docked);
    void Drag(POINT *start);
    void DockIn();
    void Undock();
};

extern Dock g_docks[2];
extern Dock *g_dockList;
extern int g_numDocks;
extern IconWnd *g_iconWnd;
extern MapWnd *g_mapWnd;

inline void Dock::Layout()
{
    RECT client;
    RECT r;
    RECT pos;
    int x;
    int y;
    int i;
#ifdef MAINWND_PAD
    int u0;
    int u1;
#endif
#ifdef LAYOUT_PAD
    LAYOUT_PAD
#endif

    GetClientRect(g_hwnd, &client);
    client.right -= client.left;
    client.bottom -= client.top;
    x = 0;
    y = 0;
    for (i = 0; i < count; i++) {
        r = wnds[i]->m_rect;
        if (r.right + x > client.right) {
            x = 0;
            y += r.bottom + 2;
        }
        pos.left = this->x + x;
        pos.top = this->y + y;
        wnds[i]->Move(&pos, SWP_NOSIZE | SWP_NOZORDER);
        x += r.right;
    }
    if (i)
        y += r.bottom;
    if (!y)
        y = 1;
    if (h != y)
        h = y;
}

/* An 8-bit DIB section with its own DC and a row table */
class Bitmap {
public:
    int m_width;
    int m_height;
    BYTE m_colour;              /* background: nearest palette index */
    HDC m_dc;
    HPALETTE m_palette;
    HBITMAP m_dib;
    HBITMAP m_oldBitmap;
    BITMAPINFOHEADER m_header;
    RGBQUAD m_colours[256];
    BYTE *m_bits;
    BYTE **m_rows;
    int m_pitch;
    int m_size;

    Bitmap(int width, int height, COLORREF colour);
    ~Bitmap();
    void Free();
    void Create(int width, int height);
    void BuildRows();
    HPALETTE MakePalette();
    void Resize(int width, int height);
    void Draw(HDC hdc, int x, int y, int w, int h, int sx, int sy);
    void Clear() { memset(m_bits, m_colour, m_size); }
    void SetPixel(int x, int y, BYTE c)
    {
#ifdef SETPIXEL_PAD
        SETPIXEL_PAD
#endif
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
            m_rows[y][x] = c;
    }
};

/* An icon-panel button: a sprite drawn into its own bitmap, a number above */
class Button : public Window {
public:
    int m_pressed;
    Bitmap *m_bitmap;
    int m_id;
    int m_count;                /* the number shown over it */
    int m_frame;                /* animation frame while pressed */
    BYTE *m_sprite;             /* SRLE sprite file */
    HFONT m_font;

    Button();
    ~Button();
    virtual DWORD GetStyle() { return WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS; }
    virtual void OnCreate();
    virtual void Paint(HDC hdc, RECT *rect);
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
    virtual void Create(RECT *rect, HWND parent, char *name, int id)
    {
        m_id = id;
        Window::Create(rect, parent, name);
    }
    virtual void DrawFace(HDC hdc, RECT *rect, HBRUSH light, HBRUSH dark);
    virtual void Press()
    {
        PNT at;

        at.x = 0;
        at.y = 0;
        if (!g_resetting && InGame())
            SoundPlayFX(at, 9);
    }
    virtual void Release() { m_frame = 0; }
    virtual void Tick();
    void SetPressed(int pressed)
    {
        if (pressed != m_pressed) {
            m_pressed = pressed;
            if (!pressed)
                Release();
            else
                Press();
            InvalidateRect(m_hwnd, 0, FALSE);
        }
    }
    void DrawBorder(HDC hdc, RECT *rect, HBRUSH light, HBRUSH dark);
    void LoadSprite(char *file);
    void DrawSprite(int x, int y, int frame);
};

/* One of the eight skills */
class SkillButton : public Button {
public:
    virtual void Press()
    {
        PNT at;

        SendMessage(m_parent, 0x800, m_id, 0);
        at.x = 0;
        at.y = 0;
        if (!g_resetting && InGame())
            SoundPlayFX(at, 9);
    }
};

/* The release rate's + and - buttons: they repeat while held */
class RateButton : public Button {
public:
    int m_tip;
    int m_delta;
    int m_captured;

    virtual void OnDestroy()
    {
        if (m_captured) {
            ReleaseCapture();
            m_captured = 0;
        }
    }
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_MOUSEMOVE:
            SetStatusText(GetString(m_tip));
            break;
        case WM_LBUTTONDOWN:
            SetPressed(1);
            if (!m_captured) {
                SetCapture(m_hwnd);
                m_captured = 1;
            }
            if (g_replaying) {
                StopReplay();
                g_replaying = 0;
            }
            break;
        case WM_LBUTTONUP:
            SetPressed(0);
            if (m_captured) {
                ReleaseCapture();
                m_captured = 0;
            }
            break;
        }
        return Window::WndProc(msg, wParam, lParam);
    }
    virtual void Tick()
    {
        int old = g_releaseRate;
        RECT r;

        g_releaseRate += m_delta;
        if (g_releaseRate > 99)
            g_releaseRate = 99;
        if (g_releaseRate < g_minReleaseRate)
            g_releaseRate = g_minReleaseRate;
        if (old != g_releaseRate) {
            CountRect(&r);
            InvalidateRect(g_hPlusButton, &r, FALSE);
            SendMessage(g_hwnd, 0x403, g_releaseRate, 0);
            if (!g_replaying)
                RecordRate(g_recordTime, g_releaseRate);
        }
    }
};

class PlusButton : public RateButton {
public:
    PlusButton()
    {
        m_delta = 1;
        m_tip = 0x136;
    }
    virtual void OnCreate()
    {
        g_hPlusButton = m_hwnd;
        Button::OnCreate();
    }
    virtual void Paint(HDC hdc, RECT *rect)
    {
        m_count = g_releaseRate;
        Button::Paint(hdc, rect);
    }
};

class MinusButton : public RateButton {
public:
    MinusButton()
    {
        m_delta = -1;
        m_tip = 0x137;
    }
    virtual void Paint(HDC hdc, RECT *rect)
    {
        m_count = g_minReleaseRate;
        Button::Paint(hdc, rect);
    }
};

/* Buttons without a number */
class PlainButton : public Button {
public:
    virtual void DrawFace(HDC hdc, RECT *rect, HBRUSH light, HBRUSH dark) {}
};

class PauseButton : public PlainButton {
public:
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_MOUSEMOVE:
            SetStatusText(GetString(0x12f));
            break;
        case WM_LBUTTONDOWN:
            SetPressed(m_pressed ^ 1);
            break;
        }
        return Window::WndProc(msg, wParam, lParam);
    }
    virtual void Press()
    {
        PNT at;

        if (!g_resetting)
            PostMessage(g_hwnd, WM_COMMAND, 0x168, 0);
        at.x = 0;
        at.y = 0;
        if (!g_resetting && InGame())
            SoundPlayFX(at, 9);
    }
    virtual void Release()
    {
        PNT at;

        if (!g_resetting)
            PostMessage(g_hwnd, WM_COMMAND, 0x168, 0);
        m_frame = 0;
        at.x = 0;
        at.y = 0;
        if (!g_resetting && InGame())
            SoundPlayFX(at, 9);
    }
};

class NukeButton : public PlainButton {
public:
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_MOUSEMOVE:
            SetStatusText(GetString(0x130));
            break;
        case WM_LBUTTONDBLCLK:
            SetPressed(1);
            break;
        }
        return Window::WndProc(msg, wParam, lParam);
    }
    virtual void Press()
    {
        PNT at;

        if (!g_resetting)
            PostMessage(g_hwnd, WM_COMMAND, 0x104, 0);
        at.x = 0;
        at.y = 0;
        if (!g_resetting && InGame())
            SoundPlayFX(at, 9);
    }
};

class FastButton : public PlainButton {
public:
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
        case WM_MOUSEMOVE:
            SetStatusText(GetString(m_pressed ? 0x132 : 0x131));
            break;
        case WM_LBUTTONDOWN:
            SetPressed(m_pressed ^ 1);
            SetStatusText(GetString(m_pressed ? 0x132 : 0x131));
            break;
        }
        return Window::WndProc(msg, wParam, lParam);
    }
    virtual void Press()
    {
        PNT at;

        if (!g_resetting)
            PostMessage(g_hwnd, WM_COMMAND, 0x16a, 0);
        at.x = 0;
        at.y = 0;
        if (!g_resetting && InGame())
            SoundPlayFX(at, 9);
    }
    virtual void Release()
    {
        PNT at;

        if (!g_resetting)
            PostMessage(g_hwnd, WM_COMMAND, 0x16a, 0);
        m_frame = 0;
        at.x = 0;
        at.y = 0;
        if (!g_resetting && InGame())
            SoundPlayFX(at, 9);
    }
};

/* The skill panel */
class IconWnd : public DockWnd {
public:
    SkillButton m_skills[10];
    PlusButton m_plus;
    MinusButton m_minus;
    PauseButton m_pause;
    NukeButton m_nuke;
    FastButton m_fast;
    UINT m_timer;
    int m_selected;

    IconWnd();
    ~IconWnd();
    virtual DWORD GetStyle() { return WS_POPUP | WS_CLIPSIBLINGS | WS_BORDER; }
    virtual void OnCreate();
    virtual void OnDestroy();
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
    virtual void ClampPosition(POINT *pt);
    virtual void OnDock(RECT *rect, int docked);
};

/* The map's picture: the whole level shrunk, scrolling, with dots for lemmings */
class MapView : public Window {
public:
    BYTE *m_surface;            /* the level's pixels in the renderer */
    Bitmap *m_view;             /* what is drawn */
    Bitmap *m_level;            /* the shrunk level */
    int m_modulo;
    int m_xstep;                /* level pixels per map pixel, 8.8 fixed point */
    int m_ystep;
    int m_width;
    int m_height;
    int m_scrollX;
    int m_scrollPos;

    MapView();
    ~MapView();
    virtual DWORD GetStyle() { return WS_CHILD | WS_VISIBLE; }
    virtual void OnCreate();
    virtual void OnDestroy();
    virtual void Paint(HDC hdc, RECT *rect);
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
    void Init();
    void Free();
    void Scale();
    void Scroll();
    void Update();
    void DrawLemmings();
    void DrawViewRect();
};

/* The map tool window */
class MapWnd : public DockWnd {
public:
    UINT m_timer;
    MapView m_view;

    MapWnd();
    ~MapWnd();
    virtual DWORD GetStyle() { return WS_POPUP | WS_CLIPSIBLINGS | WS_BORDER; }
    virtual void OnCreate();
    virtual void OnDestroy();
    virtual LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
};

extern BYTE g_defaultColours[20][3];
extern UINT g_mapTimer;
extern short g_lemmingXY[160];

extern int g_numWindows;
extern char *g_windowClass;
extern int g_numWindowObjects;

LRESULT CALLBACK BaseWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int RegisterBaseClass(void);

#endif
