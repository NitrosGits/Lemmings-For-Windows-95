/*
 * Messages: the queue, PeekMessage/GetMessage/WaitMessage, timers, WM_PAINT,
 * SDL's events turned into Win32 input messages, keyboard translation and
 * accelerators, and putting the windows' framebuffers on the screen.
 *
 * As in Win32, posted and input messages come first, then WM_TIMER, then
 * WM_PAINT, then WM_QUIT; WM_CHAR comes from TranslateMessage only (so a key
 * an accelerator took makes no character).
 */
#include "port_internal.h"
#include "port_res.h"

#define WM_NCMOUSEMOVE   0x00A0
#define WM_NCLBUTTONDOWN 0x00A1
#define WM_MOUSEWHEEL    0x020A

Wnd *wnd_find_paint(void);
Wnd *wnd_from_sdl(SDL_WindowID id);
void wnd_top_moved(Wnd *w, int x, int y);
void wnd_top_state(Wnd *w, int state);
void wnd_init_desktop(void);
int menu_track_bar(Wnd *top, int item, int byKey);
int menu_bar_hit(Wnd *top, int fx, int fy);
void scroll_track(Wnd *w, int bar, int fx, int fy);

int port_modal_depth;
int port_quit;
POINT port_mouse;
static int g_quitCode;
static int g_appActive;
static HCURSOR g_cursor;

/* ---- The queue ---- */

static MSG *g_q;
static int g_qn, g_qcap;

void msg_post(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    MSG *e;

    if (g_qn == g_qcap) {
        g_qcap = g_qcap ? g_qcap * 2 : 256;
        g_q = realloc(g_q, (size_t)g_qcap * sizeof *g_q);
    }
    e = &g_q[g_qn++];
    e->hwnd = h;
    e->message = m;
    e->wParam = wp;
    e->lParam = lp;
    e->time = port_ticks();
    e->pt = port_mouse;
}

BOOL WINAPI PostMessageA(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    if (h && !wnd_get(h))
        return FALSE;
    msg_post(h, m, wp, lp);
    return TRUE;
}

void WINAPI PostQuitMessage(int code)
{
    port_quit = 1;
    g_quitCode = code;
}

LRESULT WINAPI SendMessageA(HWND h, UINT m, WPARAM wp, LPARAM lp)
{
    Wnd *w = wnd_get(h);

    if (!w || !w->proc)
        return 0;
    return w->proc(h, m, wp, lp);
}

/* ---- Timers ---- */

typedef struct Timer {
    HWND h;
    UINT_PTR id;
    UINT ms;
    DWORD due;
    TIMERPROC proc;
    int used;
} Timer;

static Timer g_timers[64];
static UINT_PTR g_timerId = 0x7fff;

UINT_PTR WINAPI SetTimer(HWND h, UINT_PTR id, UINT ms, TIMERPROC proc)
{
    int i, free = -1;

    if (h && !wnd_get(h))
        return 0;
    if (!h)
        id = ++g_timerId;
    for (i = 0; i < 64; i++) {
        if (g_timers[i].used && g_timers[i].h == h && g_timers[i].id == id)
            break;
        if (!g_timers[i].used && free < 0)
            free = i;
    }
    if (i == 64) {
        if (free < 0)
            return 0;
        i = free;
    }
    if (ms < 10)
        ms = 10;
    g_timers[i] = (Timer){h, id, ms, port_ticks() + ms, proc, 1};
    return id ? id : 1;
}

BOOL WINAPI KillTimer(HWND h, UINT_PTR id)
{
    int i;

    for (i = 0; i < 64; i++)
        if (g_timers[i].used && g_timers[i].h == h && g_timers[i].id == id) {
            g_timers[i].used = 0;
            return TRUE;
        }
    return FALSE;
}

static int timer_due(MSG *m, int remove)
{
    DWORD now = port_ticks();
    int i;

    for (i = 0; i < 64; i++) {
        Timer *t = &g_timers[i];

        if (t->used && (int)(now - t->due) >= 0) {
            if (t->h && !wnd_get(t->h)) {
                t->used = 0;
                continue;
            }
            m->hwnd = t->h;
            m->message = WM_TIMER;
            m->wParam = t->id;
            m->lParam = (LPARAM)t->proc;
            m->time = now;
            m->pt = port_mouse;
            if (remove)
                t->due = now + t->ms;
            return 1;
        }
    }
    return 0;
}

static int next_timer_ms(void)
{
    DWORD now = port_ticks();
    int i, best = 1000;

    for (i = 0; i < 64; i++)
        if (g_timers[i].used) {
            int d = (int)(g_timers[i].due - now);

            if (d < best)
                best = d < 0 ? 0 : d;
        }
    return best;
}

void msg_window_gone(Wnd *w)
{
    int i, n = 0;

    for (i = 0; i < 64; i++)
        if (g_timers[i].used && g_timers[i].h == w->h)
            g_timers[i].used = 0;
    for (i = 0; i < g_qn; i++)
        if (g_q[i].hwnd != w->h)
            g_q[n++] = g_q[i];
    g_qn = n;
}

/* ---- Keys ---- */

static int vk_of(SDL_Keycode k, SDL_Scancode sc)
{
    if (k >= 'a' && k <= 'z')
        return (int)(k - 'a' + 'A');
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return 'A' + (sc - SDL_SCANCODE_A);
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return '1' + (sc - SDL_SCANCODE_1);
    if (sc == SDL_SCANCODE_0)
        return '0';
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12)
        return VK_F1 + (sc - SDL_SCANCODE_F1);
    if (sc >= SDL_SCANCODE_KP_1 && sc <= SDL_SCANCODE_KP_9)
        return VK_NUMPAD0 + 1 + (sc - SDL_SCANCODE_KP_1);
    switch (sc) {
    case SDL_SCANCODE_KP_0: return VK_NUMPAD0;
    case SDL_SCANCODE_KP_PLUS: return VK_ADD;
    case SDL_SCANCODE_KP_MINUS: return VK_SUBTRACT;
    case SDL_SCANCODE_KP_MULTIPLY: return VK_MULTIPLY;
    case SDL_SCANCODE_KP_DIVIDE: return VK_DIVIDE;
    case SDL_SCANCODE_KP_PERIOD: return VK_DECIMAL;
    case SDL_SCANCODE_KP_ENTER: return VK_RETURN;
    case SDL_SCANCODE_RETURN: return VK_RETURN;
    case SDL_SCANCODE_ESCAPE: return VK_ESCAPE;
    case SDL_SCANCODE_BACKSPACE: return VK_BACK;
    case SDL_SCANCODE_TAB: return VK_TAB;
    case SDL_SCANCODE_SPACE: return VK_SPACE;
    case SDL_SCANCODE_LEFT: return VK_LEFT;
    case SDL_SCANCODE_RIGHT: return VK_RIGHT;
    case SDL_SCANCODE_UP: return VK_UP;
    case SDL_SCANCODE_DOWN: return VK_DOWN;
    case SDL_SCANCODE_HOME: return VK_HOME;
    case SDL_SCANCODE_END: return VK_END;
    case SDL_SCANCODE_PAGEUP: return VK_PRIOR;
    case SDL_SCANCODE_PAGEDOWN: return VK_NEXT;
    case SDL_SCANCODE_INSERT: return VK_INSERT;
    case SDL_SCANCODE_DELETE: return VK_DELETE;
    case SDL_SCANCODE_PAUSE: return VK_PAUSE;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return VK_SHIFT;
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return VK_CONTROL;
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return VK_MENU;
    case SDL_SCANCODE_MINUS: return 0xBD;
    case SDL_SCANCODE_EQUALS: return 0xBB;
    case SDL_SCANCODE_COMMA: return 0xBC;
    case SDL_SCANCODE_PERIOD: return 0xBE;
    case SDL_SCANCODE_SLASH: return 0xBF;
    case SDL_SCANCODE_SEMICOLON: return 0xBA;
    case SDL_SCANCODE_GRAVE: return 0xC0;
    case SDL_SCANCODE_LEFTBRACKET: return 0xDB;
    case SDL_SCANCODE_BACKSLASH: return 0xDC;
    case SDL_SCANCODE_RIGHTBRACKET: return 0xDD;
    case SDL_SCANCODE_APOSTROPHE: return 0xDE;
    default: return 0;
    }
}

static SDL_Scancode scancode_of(int vk)
{
    if (vk >= 'A' && vk <= 'Z')
        return (SDL_Scancode)(SDL_SCANCODE_A + vk - 'A');
    if (vk >= '1' && vk <= '9')
        return (SDL_Scancode)(SDL_SCANCODE_1 + vk - '1');
    if (vk >= VK_F1 && vk <= VK_F12)
        return (SDL_Scancode)(SDL_SCANCODE_F1 + vk - VK_F1);
    if (vk > VK_NUMPAD0 && vk <= VK_NUMPAD9)
        return (SDL_Scancode)(SDL_SCANCODE_KP_1 + vk - VK_NUMPAD0 - 1);
    switch (vk) {
    case '0': return SDL_SCANCODE_0;
    case VK_NUMPAD0: return SDL_SCANCODE_KP_0;
    case VK_LEFT: return SDL_SCANCODE_LEFT;
    case VK_RIGHT: return SDL_SCANCODE_RIGHT;
    case VK_UP: return SDL_SCANCODE_UP;
    case VK_DOWN: return SDL_SCANCODE_DOWN;
    case VK_RETURN: return SDL_SCANCODE_RETURN;
    case VK_ESCAPE: return SDL_SCANCODE_ESCAPE;
    case VK_SPACE: return SDL_SCANCODE_SPACE;
    case VK_TAB: return SDL_SCANCODE_TAB;
    case VK_BACK: return SDL_SCANCODE_BACKSPACE;
    }
    return SDL_SCANCODE_UNKNOWN;
}

int port_key_down(int vk)
{
    const bool *ks = SDL_GetKeyboardState(NULL);
    SDL_Keymod mod = SDL_GetModState();

    switch (vk) {
    case VK_SHIFT:
        return (mod & SDL_KMOD_SHIFT) != 0;
    case VK_CONTROL:
        return (mod & SDL_KMOD_CTRL) != 0;
    case VK_MENU:
        return (mod & SDL_KMOD_ALT) != 0;
    }
    {
        SDL_Scancode sc = scancode_of(vk);

        return sc != SDL_SCANCODE_UNKNOWN && ks[sc];
    }
}

SHORT WINAPI GetAsyncKeyState(int vk)
{
    SDL_MouseButtonFlags b;
    float x, y;

    SDL_PumpEvents();
    if (port_harness)
        return harness_key(vk) ? (SHORT)0x8001 : 0;
    if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) {
        b = SDL_GetGlobalMouseState(&x, &y);
        if (vk == VK_LBUTTON && (b & SDL_BUTTON_LMASK)) {
            SDL_Delay(1);       /* the dock drag polls this in a tight loop */
            return (SHORT)0x8000;
        }
        if (vk == VK_RBUTTON && (b & SDL_BUTTON_RMASK))
            return (SHORT)0x8000;
        if (vk == VK_MBUTTON && (b & SDL_BUTTON_MMASK))
            return (SHORT)0x8000;
        return 0;
    }
    return port_key_down(vk) ? (SHORT)0x8000 : 0;
}

SHORT WINAPI GetKeyState(int vk)
{
    return GetAsyncKeyState(vk) ? (SHORT)0x8000 : 0;
}

BOOL WINAPI GetCursorPos(LPPOINT pt)
{
    float x, y;

    if (port_harness && harness_cursor(pt))
        return TRUE;
    SDL_PumpEvents();
    SDL_GetGlobalMouseState(&x, &y);
    pt->x = (LONG)x;
    pt->y = (LONG)y;
    return TRUE;
}

BOOL WINAPI SetCursorPos(int x, int y)
{
    if (!port_harness)
        SDL_WarpMouseGlobal((float)x, (float)y);
    return TRUE;
}

HCURSOR WINAPI SetCursor(HCURSOR c)
{
    HCURSOR old = g_cursor;

    if (c != g_cursor) {
        g_cursor = c;
        cursor_apply(c);
    }
    return old;
}

HCURSOR WINAPI GetCursor(void)
{
    return g_cursor;
}

int WINAPI ShowCursor(BOOL show)
{
    static int count;

    count += show ? 1 : -1;
    if (count >= 0)
        SDL_ShowCursor();
    else
        SDL_HideCursor();
    return count;
}

/* ---- SDL events to messages ---- */

static Wnd *main_window(void)
{
    Wnd *w, *best = NULL;

    for (w = port_top; w; w = w->next)
        if (!w->destroyed && !w->owner && (w->style & WS_CAPTION) == WS_CAPTION)
            best = w;
    return best;
}

static int top_enabled(Wnd *top)
{
    return top && !(top->style & WS_DISABLED);
}

static WPARAM mouse_keys(SDL_MouseButtonFlags b)
{
    WPARAM k = 0;
    SDL_Keymod mod = SDL_GetModState();

    if (b & SDL_BUTTON_LMASK)
        k |= MK_LBUTTON;
    if (b & SDL_BUTTON_RMASK)
        k |= MK_RBUTTON;
    if (b & SDL_BUTTON_MMASK)
        k |= MK_MBUTTON;
    if (mod & SDL_KMOD_SHIFT)
        k |= MK_SHIFT;
    if (mod & SDL_KMOD_CTRL)
        k |= MK_CONTROL;
    return k;
}

void msg_set_cursor_for(Wnd *top, int fx, int fy)
{
    int hit;
    Wnd *w = port_capture ? port_capture : wnd_from_point(top, fx, fy, &hit);

    if (port_capture)
        hit = HTCLIENT;
    if (!w)
        return;
    if (!top_enabled(wnd_toplevel(w))) {
        SetCursor(LoadCursorA(NULL, IDC_ARROW));
        return;
    }
    SendMessageA(w->h, WM_SETCURSOR, (WPARAM)w->h, MAKELPARAM(hit, WM_MOUSEMOVE));
}

/* client coordinates of a framebuffer point */
static LPARAM client_lparam(Wnd *w, int fx, int fy)
{
    RECT r;

    wnd_client_fb(w, &r);
    return MAKELPARAM((WORD)(short)(fx - r.left), (WORD)(short)(fy - r.top));
}

static void mouse_event(Wnd *top, int fx, int fy, UINT down, UINT up, int clicks, SDL_MouseButtonFlags b)
{
    Wnd *w;
    int hit = HTCLIENT;
    Wnd *t;

    port_mouse.x = top->rc.left + fx;
    port_mouse.y = top->rc.top + fy;
    if (port_capture && !port_capture->destroyed) {
        w = port_capture;
        t = wnd_toplevel(w);
        fx = port_mouse.x - t->rc.left;
        fy = port_mouse.y - t->rc.top;
    } else {
        w = wnd_from_point(top, fx, fy, &hit);
        if (!w)
            return;
        /* a modal dialog's owner and disabled controls take no clicks */
        if (!top_enabled(top)) {
            if (down) {
                Wnd *d;

                for (d = port_top; d; d = d->next)
                    if (d->owner == top && wnd_visible(d) && d->sdl)
                        SDL_RaiseWindow(d->sdl);
            }
            return;
        }
        while (w->parent && (w->style & WS_DISABLED))
            w = w->parent;
    }
    if (!down && !up) {
        msg_set_cursor_for(top, fx, fy);
        if (hit == HTCLIENT)
            msg_post(w->h, WM_MOUSEMOVE, mouse_keys(b), client_lparam(w, fx, fy));
        else
            msg_post(w->h, WM_NCMOUSEMOVE, (WPARAM)hit, MAKELPARAM(port_mouse.x, port_mouse.y));
        return;
    }
    if (down) {
        if (port_active != top && top->sdl) {
            port_active = top;
            SendMessageA(top->h, WM_NCACTIVATE, TRUE, 0);
        }
        if (hit != HTCLIENT) {
            msg_post(w->h, WM_NCLBUTTONDOWN, (WPARAM)hit, MAKELPARAM(fx, fy));
            return;
        }
        if (clicks == 2 && (w->cls->wc.style & CS_DBLCLKS))
            down = down == WM_LBUTTONDOWN ? WM_LBUTTONDBLCLK : down == WM_RBUTTONDOWN ? WM_RBUTTONDBLCLK
                                                                                         : WM_MBUTTONDBLCLK;
        msg_post(w->h, down, mouse_keys(b), client_lparam(w, fx, fy));
    } else if (hit == HTCLIENT || port_capture)
        msg_post(w->h, up, mouse_keys(b), client_lparam(w, fx, fy));
}

static void key_event(const SDL_KeyboardEvent *k)
{
    int vk = vk_of(k->key, k->scancode);
    Wnd *w = port_focus && !port_focus->destroyed ? port_focus : port_active;
    SDL_Keymod mod = k->mod;
    int alt = (mod & SDL_KMOD_ALT) != 0;
    LPARAM lp;
    UINT m;

    if (!vk || !w || w->destroyed)
        return;
    if (!top_enabled(wnd_toplevel(w)))
        return;
    if (mod & SDL_KMOD_GUI)
        return;                 /* Cmd+key belongs to the system */
    lp = 1 | ((LPARAM)(k->scancode & 0xff) << 16) | ((mod & SDL_KMOD_SHIFT) ? 1L << 24 : 0) |
         ((mod & SDL_KMOD_CAPS) ? 1L << 25 : 0) | ((mod & SDL_KMOD_CTRL) ? 1L << 26 : 0) |
         (alt ? 1L << 29 : 0);
    if (k->down) {
        if (k->repeat)
            lp |= 1L << 30;
        m = alt || vk == VK_F10 ? WM_SYSKEYDOWN : WM_KEYDOWN;
    } else {
        lp |= 3L << 30;
        m = alt || vk == VK_F10 ? WM_SYSKEYUP : WM_KEYUP;
    }
    msg_post(w->h, m, (WPARAM)vk, lp);
}

static void window_event(const SDL_WindowEvent *e)
{
    Wnd *w = wnd_from_sdl(e->windowID);

    if (!w || w->destroyed)
        return;
    switch (e->type) {
    case SDL_EVENT_WINDOW_MOVED:
        wnd_top_moved(w, e->data1, e->data2);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        wnd_top_resized(w, e->data1, e->data2);
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
        wnd_top_state(w, SIZE_MINIMIZED);
        break;
    case SDL_EVENT_WINDOW_MAXIMIZED:
        wnd_top_state(w, SIZE_MAXIMIZED);
        break;
    case SDL_EVENT_WINDOW_RESTORED:
        wnd_top_state(w, SIZE_RESTORED);
        break;
    case SDL_EVENT_WINDOW_EXPOSED:
        w->dirty = 1;
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        if (!g_appActive) {
            Wnd *t;

            g_appActive = 1;
            for (t = port_top; t; t = t->next)
                if (!t->destroyed)
                    SendMessageA(t->h, WM_ACTIVATEAPP, TRUE, 0);
        }
        if (port_active != w) {
            Wnd *old = port_active;

            port_active = w;
            if (old && !old->destroyed)
                SendMessageA(old->h, WM_NCACTIVATE, FALSE, 0);
            SendMessageA(w->h, WM_NCACTIVATE, TRUE, 0);
            SendMessageA(w->h, WM_ACTIVATE, 1, (LPARAM)(old ? old->h : NULL));
        }
        if (!port_focus || wnd_toplevel(port_focus) != w)
            if (top_enabled(w) && !w->dlg)
                SetFocus(w->h);
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        break;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        if (top_enabled(w))
            msg_post(w->h, WM_SYSCOMMAND, SC_CLOSE, 0);
        break;
    default:
        break;
    }
}

static void check_app_active(void)
{
    if (g_appActive && !SDL_GetKeyboardFocus()) {
        Wnd *t;

        g_appActive = 0;
        for (t = port_top; t; t = t->next)
            if (!t->destroyed)
                SendMessageA(t->h, WM_ACTIVATEAPP, FALSE, 0);
    }
}

void msg_handle_event(const SDL_Event *ev)
{
    Wnd *top;

    switch (ev->type) {
    case SDL_EVENT_QUIT:
        {
            Wnd *m = main_window();

            if (m && top_enabled(m))
                msg_post(m->h, WM_CLOSE, 0, 0);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        top = wnd_from_sdl(ev->motion.windowID);
        if (top && !port_harness)
            mouse_event(top, (int)ev->motion.x, (int)ev->motion.y, 0, 0, 0, ev->motion.state);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        top = wnd_from_sdl(ev->button.windowID);
        if (top && !port_harness) {
            int d = ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            UINT dn = 0, up = 0;
            SDL_MouseButtonFlags b = SDL_GetMouseState(NULL, NULL);

            switch (ev->button.button) {
            case SDL_BUTTON_LEFT:
                dn = WM_LBUTTONDOWN;
                up = WM_LBUTTONUP;
                break;
            case SDL_BUTTON_RIGHT:
                dn = WM_RBUTTONDOWN;
                up = WM_RBUTTONUP;
                break;
            case SDL_BUTTON_MIDDLE:
                dn = WM_MBUTTONDOWN;
                up = WM_MBUTTONUP;
                break;
            default:
                return;
            }
            mouse_event(top, (int)ev->button.x, (int)ev->button.y, d ? dn : 0, d ? 0 : up, ev->button.clicks, b);
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        if (port_focus && !port_harness)
            msg_post(port_focus->h, WM_MOUSEWHEEL, MAKEWPARAM(0, (short)(ev->wheel.y * 120)), 0);
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        if (!port_harness)
            key_event(&ev->key);
        break;
    default:
        if (ev->type >= SDL_EVENT_WINDOW_FIRST && ev->type <= SDL_EVENT_WINDOW_LAST)
            window_event(&ev->window);
        break;
    }
}

int msg_pump(int wait)
{
    SDL_Event ev;
    int n = 0;

    if (wait > 0 ? SDL_WaitEventTimeout(&ev, wait) : SDL_PollEvent(&ev)) {
        do {
            msg_handle_event(&ev);
            n++;
        } while (SDL_PollEvent(&ev));
    }
    check_app_active();
    return n;
}

/* ---- Presenting ---- */

static void present(Wnd *w)
{
    int ow, oh;

    if (!w->sdl || !w->ren || !wnd_visible(w))
        return;
    if (!w->tex) {
        w->tex = SDL_CreateTexture(w->ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, w->fb.w, w->fb.h);
        if (!w->tex)
            return;
        SDL_SetTextureScaleMode(w->tex, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderLogicalPresentation(w->ren, w->fb.w, w->fb.h, SDL_LOGICAL_PRESENTATION_STRETCH);
    }
    SDL_UpdateTexture(w->tex, NULL, w->fb.bits, w->fb.pitch);
    SDL_RenderClear(w->ren);
    SDL_RenderTexture(w->ren, w->tex, NULL, NULL);
    if (w->overlay) {
        SDL_Texture *t = SDL_CreateTexture(w->ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STATIC,
                                           w->overlay->w, w->overlay->h);

        if (t) {
            SDL_FRect d = {(float)w->overlayX, (float)w->overlayY, (float)w->overlay->w, (float)w->overlay->h};

            SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
            SDL_UpdateTexture(t, NULL, w->overlay->bits, w->overlay->pitch);
            SDL_RenderTexture(w->ren, t, NULL, &d);
            SDL_DestroyTexture(t);
        }
    }
    (void)ow;
    (void)oh;
    SDL_RenderPresent(w->ren);
    w->dirty = 0;
}

void msg_present(void)
{
    Wnd *w;

    for (w = port_top; w; w = w->next)
        if (w->dirty && !w->destroyed)
            present(w);
}

/* ---- Getting messages ---- */

static int matches(const MSG *m, HWND h, UINT min, UINT max)
{
    if (h && m->hwnd != h)
        return 0;
    if (min || max)
        return m->message >= min && m->message <= max;
    return 1;
}

BOOL WINAPI PeekMessageA(LPMSG msg, HWND h, UINT min, UINT max, UINT remove)
{
    Wnd *w;
    int i;

    msg_pump(0);
    if (port_harness)
        harness_poll();
    for (i = 0; i < g_qn; i++) {
        if (matches(&g_q[i], h, min, max)) {
            *msg = g_q[i];
            if (remove & PM_REMOVE) {
                memmove(g_q + i, g_q + i + 1, (size_t)(g_qn - i - 1) * sizeof *g_q);
                g_qn--;
            }
            return TRUE;
        }
    }
    mci_poll();
    if ((!min && !max) || (min <= WM_TIMER && max >= WM_TIMER)) {
        if (timer_due(msg, remove & PM_REMOVE) && matches(msg, h, 0, 0))
            return TRUE;
    }
    if ((!min && !max) || (min <= WM_PAINT && max >= WM_PAINT)) {
        w = wnd_find_paint();
        if (w && (!h || w->h == h)) {
            msg->hwnd = w->h;
            msg->message = WM_PAINT;
            msg->wParam = 0;
            msg->lParam = 0;
            msg->time = port_ticks();
            msg->pt = port_mouse;
            return TRUE;
        }
    }
    if (port_quit) {
        msg->hwnd = NULL;
        msg->message = WM_QUIT;
        msg->wParam = (WPARAM)g_quitCode;
        msg->lParam = 0;
        if (remove & PM_REMOVE)
            port_quit = 0;
        return TRUE;
    }
    msg_present();
    return FALSE;
}

BOOL WINAPI WaitMessage(void)
{
    int t;

    msg_present();
    if (g_qn || port_quit || wnd_find_paint())
        return TRUE;
    t = next_timer_ms();
    if (t <= 0)
        return TRUE;
    if (t > 20)
        t = 20;                 /* MCI notifications and the harness are polled */
    msg_pump(t);
    return TRUE;
}

BOOL WINAPI GetMessageA(LPMSG msg, HWND h, UINT min, UINT max)
{
    while (!PeekMessageA(msg, h, min, max, PM_REMOVE))
        WaitMessage();
    return msg->message != WM_QUIT;
}

LRESULT WINAPI DispatchMessageA(const MSG *msg)
{
    Wnd *w;
    LRESULT r;

    if (msg->message == WM_TIMER && msg->lParam) {
        ((TIMERPROC)msg->lParam)(msg->hwnd, WM_TIMER, msg->wParam, port_ticks());
        return 0;
    }
    w = wnd_get(msg->hwnd);
    if (!w)
        return 0;
    if (msg->message == WM_NCLBUTTONDOWN) {
        int hit = (int)msg->wParam, fx = (short)LOWORD(msg->lParam), fy = (short)HIWORD(msg->lParam);
        Wnd *top = wnd_toplevel(w);

        if (hit == HTMENU)
            menu_track_bar(top, menu_bar_hit(top, fx, fy), 0);
        else if (hit == HTVSCROLL || hit == HTHSCROLL)
            scroll_track(w, hit == HTVSCROLL ? SB_VERT : SB_HORZ, fx, fy);
        return 0;
    }
    r = w->proc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
    if (msg->message == WM_PAINT && wnd_get(msg->hwnd) && w->invalid) {
        /* painted without BeginPaint: validate, as DefWindowProc would */
        w->invalid = 0;
        w->erase = 0;
    }
    return r;
}

BOOL WINAPI TranslateMessage(const MSG *msg)
{
    int vk = (int)msg->wParam, sc = (int)((msg->lParam >> 16) & 0xff);
    int shift = (msg->lParam >> 24) & 1, caps = (msg->lParam >> 25) & 1, ctrl = (msg->lParam >> 26) & 1;
    int ch = 0;

    if (msg->message != WM_KEYDOWN && msg->message != WM_SYSKEYDOWN)
        return FALSE;
    switch (vk) {
    case VK_RETURN:
        ch = 13;
        break;
    case VK_ESCAPE:
        ch = 27;
        break;
    case VK_BACK:
        ch = 8;
        break;
    case VK_TAB:
        ch = 9;
        break;
    default:
        if (ctrl && vk >= 'A' && vk <= 'Z')
            ch = vk - 'A' + 1;
        else if (sc) {
            SDL_Keymod mod = (shift ? SDL_KMOD_SHIFT : 0) | (caps ? SDL_KMOD_CAPS : 0);
            SDL_Keycode k = SDL_GetKeyFromScancode((SDL_Scancode)sc, mod, false);

            if (k >= 32 && k < 127)
                ch = (int)k;
            else if (k >= 0xa0 && k < 0x100)
                ch = (int)k;
        }
        break;
    }
    if (!ch)
        return FALSE;
    if (msg->message == WM_SYSKEYDOWN && ch >= 'A' && ch <= 'Z' && !shift)
        ch += 'a' - 'A';
    msg_post(msg->hwnd, msg->message == WM_SYSKEYDOWN ? WM_SYSCHAR : WM_CHAR, (WPARAM)ch, msg->lParam);
    return TRUE;
}

/* ---- Accelerators ---- */

HACCEL WINAPI LoadAcceleratorsA(HINSTANCE inst, LPCSTR name)
{
    const PortResAccelTable *t;

    (void)inst;
    for (t = port_res_accels; t->accels; t++) {
        if (IS_INTRESOURCE(name) ? t->id == (int)(uintptr_t)name : (t->name && !SDL_strcasecmp(t->name, name)))
            return (HACCEL)port_handle_new(H_ACCEL, (void *)t);
    }
    return NULL;
}

int WINAPI TranslateAcceleratorA(HWND h, HACCEL accel, LPMSG msg)
{
    const PortResAccelTable *t = (const PortResAccelTable *)port_handle_get(accel, H_ACCEL);
    int i, key, virt, alt, shift, ctrl;
    Wnd *w = wnd_get(h);

    if (!t || !w || !top_enabled(wnd_toplevel(w)))
        return 0;
    switch (msg->message) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        virt = 1;
        break;
    case WM_CHAR:
    case WM_SYSCHAR:
        virt = 0;
        break;
    default:
        return 0;
    }
    key = (int)msg->wParam;
    alt = (msg->lParam >> 29) & 1;
    shift = (msg->lParam >> 24) & 1;
    ctrl = (msg->lParam >> 26) & 1;
    for (i = 0; i < t->count; i++) {
        const PortResAccel *a = &t->accels[i];

        if (!!(a->flags & FVIRTKEY) != virt || a->key != key)
            continue;
        if (!!(a->flags & FALT) != alt)
            continue;
        if (virt && (!!(a->flags & FSHIFT) != shift || !!(a->flags & FCONTROL) != ctrl))
            continue;
        if (w->menu && !menu_item_enabled(w->menu, (UINT)a->cmd))
            return 1;
        port_trace("accelerator %d -> command %d", key, a->cmd);
        SendMessageA(h, WM_COMMAND, MAKEWPARAM(a->cmd, 1), 0);
        return 1;
    }
    return 0;
}

void msg_init(void)
{
    wnd_init_desktop();
    g_appActive = 0;
}
