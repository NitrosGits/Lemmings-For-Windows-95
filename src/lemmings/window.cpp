/*
 * The base window class: one registered window class whose procedure hands
 * each message to the C++ object stored in the window's user data.
 */
#include <windows.h>
#include "winlemm.h"
#include "window.h"

int g_numWindows = 0;
char *g_windowClass = "WINLEMM_Base_Window_Class";
int g_numWindowObjects = 0;

/* 0x40d7e0 */
LRESULT CALLBACK BaseWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Window *w = (Window *)PORT_GetWindowLong(hwnd, GWL_USERDATA);

    if (!w && msg != WM_CREATE)
        return DefWindowProc(hwnd, msg, wParam, lParam);
    switch (msg) {
    case WM_CREATE:
        {
            POINT pt;

            g_numWindows++;
            w = (Window *)((CREATESTRUCT *)lParam)->lpCreateParams;
            PORT_SetWindowLong(hwnd, GWL_USERDATA, (PORT_LONG)w);
            pt.x = 0;
            pt.y = 0;
            ClientToScreen(hwnd, &pt);
            w->m_rect.top = pt.x;
            w->m_rect.left = pt.y;
            w->m_hwnd = hwnd;
            w->OnCreate();
        }
        return 0;
    case WM_DESTROY:
        g_numWindows--;
        w->OnDestroy();
        w->m_hwnd = 0;
        return 0;
    case WM_MOVE:
        {
            POINT pt;

            pt.x = 0;
            pt.y = 0;
            ClientToScreen(hwnd, &pt);
            w->m_rect.top = pt.x;
            w->m_rect.left = pt.y;
            w->OnMove();
        }
        return 0;
    case WM_SIZE:
        w->m_rect.right = LOWORD(lParam);
        w->m_rect.bottom = HIWORD(lParam);
        w->OnSize();
        switch (wParam) {
        case SIZE_RESTORED:
            w->m_state = 2;
            w->OnRestore();
            return 0;
        case SIZE_MINIMIZED:
            w->m_state = 0;
            w->OnMinimize();
            return 0;
        case SIZE_MAXIMIZED:
            w->m_state = 1;
            w->OnMaximize();
            return 0;
        }
        return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            w->Paint(hdc, &ps.rcPaint);
            EndPaint(hwnd, &ps);
        }
        return 0;
    case WM_ERASEBKGND:
        {
            RECT rect;
            HDC hdc = (HDC)wParam;

            GetClientRect(hwnd, &rect);
            SetBkColor(hdc, w->m_bkColor);
            ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, "", 0, 0);
        }
        return 0;
    case WM_SETCURSOR:
        {
            POINT pt;
            RECT rect;
            HWND child;

            GetCursorPos(&pt);
            ScreenToClient(hwnd, &pt);
            GetClientRect(hwnd, &rect);
            if (PtInRect(&rect, pt)) {
                child = ChildWindowFromPoint(hwnd, pt);
                if (!child || hwnd == child) {
                    SetCursor(w->m_cursor);
                    return 1;
                }
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return w->WndProc(msg, wParam, lParam);
}

/* 0x40dab0 */
Window::Window()
{
    if (!g_numWindowObjects)
        RegisterBaseClass();
    g_numWindowObjects++;
    m_hwnd = 0;
}

/* 0x40dae0 */
int RegisterBaseClass(void)
{
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = BaseWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 4;
    wc.hInstance = g_hInst;
    wc.hIcon = LoadIcon(0, IDI_APPLICATION);
    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszMenuName = 0;
    wc.lpszClassName = g_windowClass;
    if (!RegisterClass(&wc))
        FatalError(0x7f0c);
    return 1;
}

/* 0x40db60 */
void Window::Create(RECT *rect, HWND parent, char *name)
{
    RECT r;
    DWORD style;

    m_parent = parent;
    style = GetStyle();
    r = *rect;
    m_rect = *rect;
    AdjustWindowRect(&r, style, FALSE);
    if (r.top < 0) {
        r.bottom -= r.top;
        r.top = 0;
    }
    if (r.left < 0) {
        r.right -= r.left;
        r.left = 0;
    }
    m_hwnd = CreateWindowEx(0, g_windowClass, name, style, r.top, r.left, r.right, r.bottom,
                            parent, 0, g_hInst, this);
    if (!m_hwnd) {
        GetLastError();
        FatalError(0x7f0d);
    }
}

/* 0x40dc40 */
void Window::Destroy()
{
    if (m_hwnd)
        DestroyWindow(m_hwnd);
}
