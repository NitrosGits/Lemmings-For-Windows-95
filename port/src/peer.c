/*
 * Native views for a dialog's controls (macOS: plat_control_* in
 * port/macos/controls_cocoa.m). A dialog's push buttons, check boxes, radio
 * buttons, group boxes, edit fields and list boxes get a system view over
 * them; the layer's control (controls.c) stays the truth - its state, its
 * messages, its notifications - and stops drawing itself. peer_sync_all()
 * shows the layer's state in the views before each present; what the user
 * does in a view comes back through input_control*() as the same messages
 * the drawn control would have handled (BM_CLICK, LB_SETCURSEL and
 * LBN_SELCHANGE, WM_SETTEXT and EN_CHANGE, dialog keys). Static text and the
 * game's own control classes stay drawn: they rely on WM_CTLCOLOR* and the
 * game's backgrounds.
 */
#include "port_internal.h"

#define H32(h) ((uint32_t)(uintptr_t)(h))

struct Peer {
    PlatControl *c;
    int kind;
    /* what the view shows now */
    RECT rc;
    int visible, enabled;
    char text[512];
    int sel, top, serial, rowH, rows;
    int redraw;             /* an owner-drawn list was invalidated */
};

static int g_focusFromView;

static int kind_of(Wnd *w)
{
    if (!w->parent || !w->parent->dlg)
        return -1;          /* a dialog's controls only */
    if (!strcasecmp(w->cls->name, "Button")) {
        switch (w->style & 0xf) {
        case BS_PUSHBUTTON:
            return PC_BUTTON;
        case BS_DEFPUSHBUTTON:
            return PC_DEFBUTTON;
        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
        case BS_3STATE:
        case BS_AUTO3STATE:
            return PC_CHECKBOX;
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
            return PC_RADIO;
        case BS_GROUPBOX:
            return PC_GROUPBOX;
        }
        return -1;
    }
    if (!strcasecmp(w->cls->name, "Edit"))
        return (w->style & 0x0004L) ? -1 : PC_EDIT;    /* not ES_MULTILINE */
    if (!strcasecmp(w->cls->name, "ListBox"))
        return (w->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) ? PC_LIST_OWNERDRAW : PC_LIST;
    return -1;
}

static int is_list(const struct Peer *p)
{
    return p->kind == PC_LIST || p->kind == PC_LIST_OWNERDRAW;
}

/* a control's text as its view shows it: UTF-8, and on buttons without the
   '&' of the keyboard mnemonic */
static void view_text(const struct Peer *p, const char *s, char *out, size_t size)
{
    char plain[512];
    size_t n = 0;

    if (p->kind == PC_EDIT) {
        port_utf8(s, out, size);
        return;
    }
    for (; *s && n + 1 < sizeof plain; s++) {
        if (*s == '&' && s[1] != '&')
            continue;
        plain[n++] = *s;
        if (*s == '&')
            s++;
    }
    plain[n] = 0;
    port_utf8(plain, out, size);
}

/* UTF-8 to the game's Windows-1252 */
static void from_utf8(const char *s, char *out, size_t size)
{
    static const unsigned short hi[32] = {
        0x20ac, 0x81, 0x201a, 0x192, 0x201e, 0x2026, 0x2020, 0x2021, 0x2c6, 0x2030, 0x160, 0x2039, 0x152, 0x8d,
        0x17d, 0x8f, 0x90, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x2dc, 0x2122, 0x161,
        0x203a, 0x153, 0x9d, 0x17e, 0x178,
    };
    const unsigned char *u = (const unsigned char *)s;
    size_t n = 0;

    while (*u && n + 1 < size) {
        unsigned cp;
        int i, more = *u >= 0xf0 ? 3 : *u >= 0xe0 ? 2 : *u >= 0xc0 ? 1 : 0;

        cp = more == 3 ? *u & 7 : more == 2 ? *u & 15 : more == 1 ? *u & 31 : *u;
        for (u++, i = 0; i < more && (*u & 0xc0) == 0x80; i++, u++)
            cp = cp << 6 | (*u & 63);
        if (cp >= 0x80 && cp < 0xa0)
            cp = '?';
        else if (cp > 0xff) {
            unsigned c = '?';

            for (i = 0; i < 32; i++)
                if (hi[i] == cp)
                    c = 0x80 + (unsigned)i;
            cp = c;
        }
        out[n++] = (char)cp;
    }
    out[n] = 0;
}

static void notify(Wnd *w, int code)
{
    if (w->parent)
        SendMessageA(w->parent->h, WM_COMMAND, MAKEWPARAM(w->id, code), (LPARAM)w->h);
}

/* ---- The layer's state into the view ---- */

static void sync_one(Wnd *w)
{
    struct Peer *p = w->peer;
    int visible = wnd_visible(w), enabled = !(w->style & WS_DISABLED);
    RECT r;

    wnd_window_fb(w, &r);
    if (!EqualRect(&r, &p->rc) || visible != p->visible || enabled != p->enabled) {
        plat_control_frame(p->c, r.left, r.top, r.right - r.left, r.bottom - r.top, visible, enabled);
        p->rc = r;
        p->visible = visible;
        p->enabled = enabled;
    }
    if (!is_list(p)) {
        char t[512], u[512];

        GetWindowTextA(w->h, t, sizeof t);
        view_text(p, t, u, sizeof u);
        if (strcmp(u, p->text)) {
            plat_control_text(p->c, u);
            port_strlcpy(p->text, u, sizeof p->text);
        }
        if (p->kind == PC_CHECKBOX || p->kind == PC_RADIO)
            plat_control_check(p->c, (int)SendMessageA(w->h, BM_GETCHECK, 0, 0));
    } else {
        int serial = lb_serial(w), rowH = (int)SendMessageA(w->h, LB_GETITEMHEIGHT, 0, 0);
        int rows = (int)SendMessageA(w->h, LB_GETCOUNT, 0, 0), sel, top;

        if (serial != p->serial || rowH != p->rowH || rows != p->rows || p->redraw) {
            p->serial = serial;
            p->rowH = rowH;
            p->rows = rows;
            p->redraw = 0;
            plat_list_reload(p->c, rows, rowH);
            p->sel = -2;
            p->top = -1;
        }
        sel = (int)SendMessageA(w->h, LB_GETCURSEL, 0, 0);
        if (sel != p->sel) {
            plat_list_select(p->c, sel);
            p->sel = sel;
        }
        top = (int)SendMessageA(w->h, LB_GETTOPINDEX, 0, 0);
        if (top != p->top) {
            plat_list_top(p->c, top);
            p->top = top;
        }
    }
}

static void sync_tree(Wnd *w)
{
    Wnd *c;

    for (c = w->child; c; c = c->next) {
        if (c->destroyed)
            continue;
        if (c->peer)
            sync_one(c);
        sync_tree(c);
    }
}

void peer_sync_all(void)
{
    Wnd *t;

    for (t = port_top; t; t = t->next)
        if (!t->destroyed && t->plat && t->dlg)
            sync_tree(t);
}

void peer_create(Wnd *w)
{
    Wnd *top = wnd_toplevel(w);
    int kind = kind_of(w);
    PlatControl *c;
    struct Peer *p;

    if (kind < 0 || !top->plat || w->peer)
        return;
    c = plat_control_create(top->plat, H32(w->h), kind);
    if (!c)
        return;
    p = calloc(1, sizeof *p);
    p->c = c;
    p->kind = kind;
    p->visible = p->enabled = -1;
    p->sel = -2;
    p->top = p->serial = p->rowH = p->rows = -1;
    p->text[0] = 1;         /* not shown yet */
    p->text[1] = 0;
    w->peer = p;
    sync_one(w);
}

void peer_destroy(Wnd *w)
{
    if (!w->peer)
        return;
    plat_control_destroy(w->peer->c);
    free(w->peer);
    w->peer = NULL;
}

int peer_paint(Wnd *w)
{
    PAINTSTRUCT ps;

    if (!w->peer)
        return 0;
    BeginPaint(w->h, &ps);
    EndPaint(w->h, &ps);
    if (w->peer->kind == PC_LIST_OWNERDRAW)
        w->peer->redraw = 1;    /* the game draws its rows: draw them again */
    return 1;
}

/* the layer's focus moved: the keyboard goes to the view, or to the window */
void peer_focus(Wnd *w)
{
    Wnd *top;

    if (g_focusFromView || !w || w->destroyed)
        return;
    top = wnd_toplevel(w);
    if (top->plat && top->dlg)
        plat_control_focus(top->plat, w->peer ? w->peer->c : NULL);
}

/* ---- What the user does in a view ---- */

static Wnd *peer_wnd(uint32_t hwnd)
{
    Wnd *w = wnd_get((HWND)(uintptr_t)hwnd);

    return w && !w->destroyed && w->peer ? w : NULL;
}

void input_control(uint32_t hwnd, int event, int arg)
{
    Wnd *w = peer_wnd(hwnd), *top;

    if (!w)
        return;
    top = wnd_toplevel(w);
    if (port_harness && event != PCE_TOP)
        port_harness_log("  view %s %d: event %d %d", w->cls->name, w->id, event, arg);
    switch (event) {
    case PCE_CLICK:
        if (!(w->style & WS_DISABLED) && !(top->style & WS_DISABLED))
            SendMessageA(w->h, BM_CLICK, 0, 0);
        break;
    case PCE_SELECT:
    case PCE_DBLCLK:
        if (arg >= 0 && arg != (int)SendMessageA(w->h, LB_GETCURSEL, 0, 0)) {
            SendMessageA(w->h, LB_SETCURSEL, (WPARAM)arg, 0);
            if (wnd_get(w->h) && (w->style & LBS_NOTIFY))
                notify(w, LBN_SELCHANGE);
        }
        if (event == PCE_DBLCLK && arg >= 0 && peer_wnd(hwnd) && (w->style & LBS_NOTIFY))
            notify(w, LBN_DBLCLK);
        break;
    case PCE_TOP:
        SendMessageA(w->h, LB_SETTOPINDEX, (WPARAM)arg, 0);
        if (peer_wnd(hwnd)) {
            w->peer->top = (int)SendMessageA(w->h, LB_GETTOPINDEX, 0, 0);
            ValidateRect(w->h, NULL);   /* the view scrolled itself */
        }
        return;
    case PCE_FOCUS:
        g_focusFromView = 1;
        SetFocus(w->h);
        g_focusFromView = 0;
        break;
    case PCE_KEY:
        /* as if typed in the drawn control: the dialog manager takes it */
        PostMessageA(w->h, WM_KEYDOWN, (WPARAM)arg, 1);
        if (arg == VK_RETURN || arg == VK_ESCAPE || arg == VK_TAB)
            PostMessageA(w->h, WM_CHAR, arg == VK_RETURN ? 13 : arg == VK_ESCAPE ? 27 : 9, 1);
        PostMessageA(w->h, WM_KEYUP, (WPARAM)arg, (LPARAM)(1 | 3u << 30));
        break;
    }
    /* the views show the result at the next present, not now: AppKit is still
       inside the view's event (a table reloaded in its own mouse tracking
       loses the next click) */
}

void input_control_text(uint32_t hwnd, const char *utf8)
{
    Wnd *w = peer_wnd(hwnd);
    char t[512];

    if (!w)
        return;
    port_strlcpy(w->peer->text, utf8, sizeof w->peer->text);    /* the view has it already */
    from_utf8(utf8, t, sizeof t);
    SendMessageA(w->h, WM_SETTEXT, 0, (LPARAM)t);
}

int list_native_text(uint32_t hwnd, int row, char *utf8, int size)
{
    Wnd *w = peer_wnd(hwnd);
    char t[512];

    if (!w || row < 0 || row >= (int)SendMessageA(w->h, LB_GETCOUNT, 0, 0) ||
        SendMessageA(w->h, LB_GETTEXTLEN, (WPARAM)row, 0) >= (LRESULT)sizeof t)
        return 0;
    SendMessageA(w->h, LB_GETTEXT, (WPARAM)row, (LPARAM)t);
    port_utf8(t, utf8, (size_t)size);
    return 1;
}

int list_native_draw(uint32_t hwnd, int row, int width, int height, uint32_t *xrgb)
{
    Wnd *w = peer_wnd(hwnd);
    BITMAPINFO bi;
    DRAWITEMSTRUCT dis;
    HDC dc;
    HBITMAP bm, old;
    void *bits = NULL;
    int sel, y;

    if (!w || !w->parent || width <= 0 || height <= 0)
        return 0;
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    dc = CreateCompatibleDC(NULL);
    bm = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!bm || !bits) {
        DeleteDC(dc);
        return 0;
    }
    old = (HBITMAP)SelectObject(dc, bm);
    {
        /* the list's background first, as its erase would have left it */
        HBRUSH br = (HBRUSH)SendMessageA(w->parent->h, WM_CTLCOLORLISTBOX, (WPARAM)dc, (LPARAM)w->h);
        RECT all = {0, 0, width, height};

        FillRect(dc, &all, br ? br : GetSysColorBrush(COLOR_WINDOW));
    }
    sel = (int)SendMessageA(w->h, LB_GETCURSEL, 0, 0);
    memset(&dis, 0, sizeof dis);
    dis.CtlType = ODT_LISTBOX;
    dis.CtlID = (UINT)w->id;
    dis.itemID = (UINT)row;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.itemState = (row == sel ? ODS_SELECTED : 0) | (row == sel && port_focus == w ? ODS_FOCUS : 0) |
                    ((w->style & WS_DISABLED) ? ODS_DISABLED : 0);
    dis.hwndItem = w->h;
    dis.hDC = dc;
    dis.rcItem = (RECT){0, 0, width, height};
    dis.itemData = (ULONG_PTR)SendMessageA(w->h, LB_GETITEMDATA, (WPARAM)row, 0);
    SendMessageA(w->parent->h, WM_DRAWITEM, (WPARAM)w->id, (LPARAM)&dis);
    for (y = 0; y < height; y++)
        memcpy(xrgb + (size_t)y * (size_t)width, (BYTE *)bits + (size_t)y * (size_t)width * 4, (size_t)width * 4);
    SelectObject(dc, old);
    DeleteObject(bm);
    DeleteDC(dc);
    return 1;
}
