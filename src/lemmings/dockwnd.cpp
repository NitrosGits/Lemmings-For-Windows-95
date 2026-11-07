/*
 * Tool windows that the player can drag between floating on the desktop and
 * docked into a strip of the main window.
 */
/* Three unused locals in the inlined Dock::Layout (window.h, LAYOUT_PAD) stand for symbols the
   original had: inline bodies are expanded at the end of the file, so their registers depend on
   the symbol count (see docs/DECOMP.md and HANDOFF session 33). */
#define LAYOUT_PAD int __lp0; int __lp1; int __lp2;
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14, pad_top15, pad_top16, pad_top17, pad_top18, pad_top19, pad_top20, pad_top21;

#include <windows.h>
#include <string.h>
#include "winlemm.h"
#include "window.h"

#ifdef _MSC_VER
/* The constructor was built for size, not speed (docs/DECOMP.md, "Size-mode functions"). */
#pragma optimize("t", off)
#endif
/* 0x40ddf0 */
DockWnd::DockWnd()
{
    HCURSOR cursor;

    m_dock = 0;
    if (g_win32s)
        cursor = LoadCursor(0, IDC_SIZE);
    else
        cursor = LoadCursor(0, IDC_SIZEALL);
    m_cursor = cursor;
    SetCursor(cursor);
    m_docked = 0;
    m_bkColor = GetSysColor(COLOR_BTNFACE);
    m_hidden = 0;
}

#ifdef _MSC_VER
#pragma optimize("t", on)
/* Supplies the 5 padding bytes (add eax,0) the original has after the size-mode constructor. */
void __declspec(naked) DockWndCtorPad(void)
{
    __asm {
        _emit 0x05
        _emit 0x00
        _emit 0x00
        _emit 0x00
        _emit 0x00
    }
}
#endif

/* 0x40de50 */
DockWnd::~DockWnd()
{
}

/* 0x40de60: the outline dragged round the screen (thin over a dock) */
void DockWnd::DrawFrame(HDC hdc, int docked)
{
    HBRUSH brush = CreateHatchBrush(HS_DIAGCROSS, RGB(255, 255, 255));
    HBRUSH old = (HBRUSH)SelectObject(hdc, brush);
    int t = docked ? 1 : 4;
    int w = m_wndRect.right - m_wndRect.left;
    int h = m_wndRect.bottom - m_wndRect.top;

    PatBlt(hdc, m_wndRect.left, m_wndRect.top, w, t, DSTINVERT);
    PatBlt(hdc, m_wndRect.left, m_wndRect.bottom - t, w, t, DSTINVERT);
    h -= 2 * t;
    PatBlt(hdc, m_wndRect.right - t, m_wndRect.top + t, t, h, DSTINVERT);
    PatBlt(hdc, m_wndRect.left, m_wndRect.top + t, t, h, DSTINVERT);
    SelectObject(hdc, old);
    DeleteObject(brush);
}

extern int pad_s18drag0, pad_s18drag1, pad_s18drag2, pad_s18drag3, pad_s18drag4, pad_s18drag5, pad_s18drag6, pad_s18drag7, pad_s18drag8, pad_s18drag9, pad_s18drag10, pad_s18drag11, pad_s18drag12;

/* 0x40df40: follow the mouse until the button is let go */
void DockWnd::Drag(POINT *start)
{
    extern int g_numDocks;
    int docked = m_docked;
    int inDock;
    POINT origin;
    RECT desktop;
    HDC hdc;
    Dock *oldDock = m_dock;
    int i;

    {
        RECT size;
        POINT offset;
        POINT cur;
        POINT last;
        RECT tmp;

        GetWindowRect(m_hwnd, &m_wndRect);
        size = m_wndRect;
        size.right -= size.left;
        size.bottom -= size.top;
        offset.x = start->x - m_wndRect.left;
        offset.y = start->y - m_wndRect.top;
        origin.x = 0;
        origin.y = 0;
        ClientToScreen(m_parent, &origin);
        for (i = 0; i < g_numDocks; i++) {
            Dock *d = &g_dockList[i];

            d->screen.top = d->y + origin.y;
            d->screen.left = d->x + origin.x;
            d->screen.bottom = d->h + d->screen.top;
            d->screen.right = d->w + d->screen.left;
        }
        hdc = GetDC(0);
        inDock = 0;
        DrawFrame(hdc, 0);
        while (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
            last = cur;
            GetCursorPos(&cur);
            if (memcmp(&last, &cur, sizeof(POINT))) {
                DrawFrame(hdc, inDock);
                inDock = 0;
                m_dock = 0;
                for (i = 0; i < g_numDocks; i++) {
                    if (IntersectRect(&tmp, &m_wndRect, &g_dockList[i].screen)) {
                        inDock = 1;
                        m_dock = &g_dockList[i];
                        break;
                    }
                }
                OnDock(&m_wndRect, inDock);
                if (m_wndRect.right < offset.x)
                    offset.x = m_wndRect.right;
                if (m_wndRect.bottom < offset.y)
                    offset.y = m_wndRect.bottom;
                m_wndRect.top = cur.y - offset.y;
                m_wndRect.bottom = m_wndRect.top + size.bottom;
                m_wndRect.left = cur.x - offset.x;
                m_wndRect.right = m_wndRect.left + size.right;
                DrawFrame(hdc, inDock);
            }
        }
        DrawFrame(hdc, inDock);
        ReleaseDC(0, hdc);
    }
    GetWindowRect(GetDesktopWindow(), &desktop);
    if (!inDock) {
        int w = m_wndRect.right - m_wndRect.left;
        int h = m_wndRect.bottom - m_wndRect.top;

        if (desktop.top > m_wndRect.top) {
            m_wndRect.top = desktop.top;
            m_wndRect.bottom = m_wndRect.top + h;
        }
        if (desktop.left > m_wndRect.left) {
            m_wndRect.left = desktop.left;
            m_wndRect.right = m_wndRect.left + w;
        }
        if (m_wndRect.right > desktop.right) {
            m_wndRect.right = desktop.right;
            m_wndRect.left = m_wndRect.right - w;
        }
        if (m_wndRect.bottom > desktop.bottom) {
            m_wndRect.bottom = desktop.bottom;
            m_wndRect.top = m_wndRect.bottom - h;
        }
    }
    if (docked && m_dock != oldDock) {
        for (i = 0; i < oldDock->count; i++) {
            if (oldDock->wnds[i] == this) {
                oldDock->wnds[i] = 0;
                for (; i < oldDock->count - 1; i++)
                    oldDock->wnds[i] = oldDock->wnds[i + 1];
                oldDock->wnds[oldDock->count - 1] = 0;
                oldDock->count--;
                break;
            }
        }
        oldDock->Layout();
        SendMessage(g_hwnd, 0x601, 0, 0);
    }
    if (inDock) {
        DockIn();
        if (m_dock != oldDock) {
            m_dock->Add(this);
            m_dock->Layout();
            SendMessage(g_hwnd, 0x601, 0, 0);
        }
    } else {
        Undock();
        Move(&m_wndRect, SWP_NOSIZE);
    }
}

/* 0x40e430 */
void DockWnd::OnDock(RECT *rect, int docked)
{
}

/* 0x40e440 */
void DockWnd::ClampPosition(POINT *pt)
{
    if (m_docked) {
        if (pt->y < 0)
            pt->y = 0;
        if (pt->x < 0)
            pt->x = 0;
    }
}

/* 0x40e470 */
LRESULT DockWnd::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    POINT pt;

    if (!m_state)
        return DefWindowProc(m_hwnd, msg, wParam, lParam);
    switch (msg) {
    case WM_SETFOCUS:
        SendMessage(m_parent, WM_NCACTIVATE, 1, 0);
        return 0;
    case WM_CLOSE:
        SendMessage(m_parent, WM_CLOSE, 0, 0);
        return 0;
    case WM_NCHITTEST:
        DefWindowProc(m_hwnd, msg, wParam, lParam);
        return HTCLIENT;
    case WM_LBUTTONDOWN:
        SetCursor(m_cursor);
        GetCursorPos(&pt);
        Drag(&pt);
        return 0;
    }
    return Window::WndProc(msg, wParam, lParam);
}

/* 0x40e570 */
void DockWnd::DockIn()
{
    POINT pt;
    RECT r;
    LONG style;

    if (!m_docked) {
        pt.x = 0;
        pt.y = 0;
        ClientToScreen(m_parent, &pt);
        GetWindowRect(m_hwnd, &r);
        r.top -= pt.y;
        r.left -= pt.x;
        style = GetWindowLong(m_hwnd, GWL_STYLE);
        SetWindowLong(m_hwnd, GWL_STYLE, style | WS_CHILD);
        SetParent(m_hwnd, g_hwnd);
        Move(&r, SWP_NOSIZE);
        m_docked = 1;
    }
}

/* 0x40e600 */
void DockWnd::Undock()
{
    RECT r;
    LONG style;

    if (m_docked) {
        GetWindowRect(m_hwnd, &r);
        style = GetWindowLong(m_hwnd, GWL_STYLE);
        SetWindowLong(m_hwnd, GWL_STYLE, style & ~WS_CHILD);
        SetParent(m_hwnd, 0);
        Move(&r, SWP_NOSIZE);
        m_docked = 0;
    }
}
