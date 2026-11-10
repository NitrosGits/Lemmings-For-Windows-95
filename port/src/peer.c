/*
 * Native views for a dialog's controls (macOS: plat_control_* in
 * port/macos/controls_cocoa.m). A dialog's push buttons, check boxes, radio
 * buttons, group boxes, edit fields and list boxes get a system view over
 * them; the layer's control (controls.c) stays the truth - its state, its
 * messages, its notifications - and stops drawing itself. peer_sync_all()
 * shows the layer's state in the views before each present; what the user
 * does in a view comes back through input_control*() as the same messages
 * the drawn control would have handled (BM_CLICK, LB_SETCURSEL and
 * LBN_SELCHANGE, WM_SETTEXT and EN_CHANGE, dialog keys). Static text (and the
 * level dialog's own text class) is a system label, shown only. Static icons
 * and the game's picture controls stay drawn.
 *
 * Two of the game's own windows are system views too, because the game says
 * so when it paints them (port_native_icon, port_native_status: #ifdef
 * PORT_SDL in src/lemmings): the skill panel's buttons and the status bar.
 * They are shown only - the mouse still goes to the game's windows under
 * them, so pressing, holding and the double click on the nuke stay the game's.
 *
 * Any window's scroll bars (WS_HSCROLL, WS_VSCROLL: the game view's) are the
 * system's scrollers the same way: the layer keeps range and position, the
 * scroller shows them, and using it sends WM_HSCROLL/WM_VSCROLL as the drawn
 * bar did (SB_PAGE*, SB_THUMBTRACK while the knob moves, SB_THUMBPOSITION and
 * SB_ENDSCROLL when it is let go).
 */
#include "port_internal.h"

#define H32(h) ((uint32_t)(uintptr_t)(h))

struct Peer {
    PlatControl *c;
    int kind;
    Wnd *top;               /* the top-level window its view is in */
    int ownPaint;           /* a class of the game's: its WM_PAINT is not called */
    int shown;              /* an icon button, the status bar: what it shows was set */
    /* what the view shows now */
    RECT rc;
    int visible, enabled;
    char text[512];
    int sel, top_, serial, rowH, rows;  /* a list: selection, first row shown, contents */
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
    if (!strcasecmp(w->cls->name, "Static")) {
        switch (w->style & 0x1f) {
        case SS_LEFT:
            return PC_LABEL;
        case SS_CENTER:
            return PC_LABEL_CENTER;
        case SS_RIGHT:
            return PC_LABEL_RIGHT;
        case SS_SIMPLE:
        case SS_LEFTNOWORDWRAP:
            return PC_LABEL_LINE;
        case SS_ICON:
            return PC_ICON; /* the program's icon, as the system shows it */
        }
        return -1;          /* a frame, a filled box: drawn */
    }
    /* the level dialog's centred text (src/lemmings/levelsel.cpp) */
    if (!strcasecmp(w->cls->name, "IconTextClass"))
        return PC_LABEL_CENTER;
    return -1;
}

/* static text or a static icon: shown by its view, on the dialog's background */
static int is_label(const struct Peer *p)
{
    return p->kind >= PC_LABEL && p->kind <= PC_ICON;
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

static PlatControl *view_new(Wnd *w, struct Peer *p)
{
    p->top = wnd_toplevel(w);
    p->visible = p->enabled = -1;
    p->sel = -2;
    p->top_ = p->serial = p->rowH = p->rows = -1;
    p->text[0] = 1;         /* not shown yet */
    p->text[1] = 0;
    p->shown = 0;
    SetRectEmpty(&p->rc);
    return p->top->plat ? plat_control_create(p->top->plat, H32(w->h), p->kind) : NULL;
}

static void sync_one(Wnd *w)
{
    struct Peer *p = w->peer;
    int visible = wnd_visible(w), enabled = !(w->style & WS_DISABLED);
    RECT r;

    if (p->top != wnd_toplevel(w)) {
        /* it moved to another top-level window (a tool window docked or
           undocked): its view goes there */
        if (p->c)
            plat_control_destroy(p->c);
        p->c = view_new(w, p);
        wnd_invalidate(w, NULL, 1);
    }
    if (!p->c)
        return;

    wnd_window_fb(w, &r);
    if (!EqualRect(&r, &p->rc) || visible != p->visible || enabled != p->enabled) {
        plat_control_frame(p->c, r.left, r.top, r.right - r.left, r.bottom - r.top, visible, enabled);
        p->rc = r;
        p->visible = visible;
        p->enabled = enabled;
    }
    if (p->kind == PC_ICONBUTTON || p->kind == PC_STATUSBAR || p->kind == PC_ICON)
        return;             /* the game sets what they show when it paints them; the icon is the program's */
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
            p->top_ = -1;
        }
        sel = (int)SendMessageA(w->h, LB_GETCURSEL, 0, 0);
        if (sel != p->sel) {
            plat_list_select(p->c, sel);
            p->sel = sel;
        }
        top = (int)SendMessageA(w->h, LB_GETTOPINDEX, 0, 0);
        if (top != p->top_) {
            plat_list_top(p->c, top);
            p->top_ = top;
        }
    }
}

static void redraw_lists(Wnd *w)
{
    Wnd *c;

    for (c = w->child; c; c = c->next) {
        if (c->peer && c->peer->kind == PC_LIST_OWNERDRAW)
            c->peer->redraw = 1;
        redraw_lists(c);
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

/* ---- Scroll bars ---- */

static void sync_scrollbar(Wnd *top, Wnd *w, int bar)
{
    ScrollBar *sb = &w->sb[bar];
    RECT r, wr;
    int visible, enabled, range;
    double value, proportion;

    if (!wnd_scrollbar_rect(w, bar, &r)) {
        if (sb->view && sb->visible) {
            plat_control_frame(sb->view, sb->rc.left, sb->rc.top, 0, 0, 0, 0);
            sb->visible = 0;
        }
        return;
    }
    if (!sb->view) {
        sb->view = plat_scroller_create(top->plat, H32(w->h), bar == SB_VERT);
        if (!sb->view)
            return;
        sb->visible = sb->enabled = -1;
        sb->value = sb->proportion = -1;
        wnd_nc_paint(w);    /* the drawn bar is gone */
    }
    wnd_window_fb(w, &wr);
    OffsetRect(&r, wr.left, wr.top);
    visible = wnd_visible(w);
    range = sb->max - sb->min;
    enabled = range > 0 && !(w->style & WS_DISABLED);
    if (!EqualRect(&r, &sb->rc) || visible != sb->visible || enabled != sb->enabled) {
        plat_control_frame(sb->view, r.left, r.top, r.right - r.left, r.bottom - r.top, visible, enabled);
        sb->rc = r;
        sb->visible = visible;
        sb->enabled = enabled;
    }
    /* the knob as large as the page is part of the whole */
    {
        int page = wnd_scroll_page(w, bar);

        value = range > 0 ? (double)(sb->pos - sb->min) / range : 0;
        proportion = range > 0 && page > 0 ? (double)page / (range + page) : 1;
    }
    if (value != sb->value || proportion != sb->proportion) {
        plat_scroller_set(sb->view, value, proportion);
        sb->value = value;
        sb->proportion = proportion;
    }
}

static void sync_scrollbars(Wnd *top, Wnd *w)
{
    Wnd *c;

    for (c = w->child; c; c = c->next) {
        if (c->destroyed)
            continue;
        /* a native list scrolls itself */
        if (!c->peer && (c->sb[SB_HORZ].shown || c->sb[SB_HORZ].view))
            sync_scrollbar(top, c, SB_HORZ);
        if (!c->peer && (c->sb[SB_VERT].shown || c->sb[SB_VERT].view))
            sync_scrollbar(top, c, SB_VERT);
        sync_scrollbars(top, c);
    }
}

void peer_scrollbars_gone(Wnd *w)
{
    int bar;

    for (bar = 0; bar < 2; bar++)
        if (w->sb[bar].view) {
            plat_control_destroy(w->sb[bar].view);
            w->sb[bar].view = NULL;
        }
}

void input_scroll(uint32_t hwnd, int vertical, int code, double value)
{
    Wnd *w = wnd_get((HWND)(uintptr_t)hwnd);
    int bar = vertical ? SB_VERT : SB_HORZ, pos = 0;

    if (!w || w->destroyed || (w->style & WS_DISABLED) || (wnd_toplevel(w)->style & WS_DISABLED))
        return;
    if (code == SB_THUMBTRACK || code == SB_THUMBPOSITION) {
        ScrollBar *sb = &w->sb[bar];

        value = value < 0 ? 0 : value > 1 ? 1 : value;
        pos = sb->min + (int)(value * (sb->max - sb->min) + 0.5);
    }
    if (port_harness)
        port_harness_log("  view %s %d: scroll %s code %d pos %d", w->cls->name, w->id, vertical ? "V" : "H", code,
                         pos);
    SendMessageA(w->h, vertical ? WM_VSCROLL : WM_HSCROLL, MAKEWPARAM(code, pos), 0);
}

void peer_sync_all(void)
{
    Wnd *t;

    for (t = port_top; t; t = t->next) {
        if (t->destroyed || !t->plat)
            continue;
        sync_tree(t);
        sync_scrollbars(t, t);
    }
}

/* the system's colours changed: what the game drew for a view is drawn again */
void peer_colors_changed(void)
{
    Wnd *t;

    for (t = port_top; t; t = t->next)
        if (!t->destroyed)
            redraw_lists(t);
}

static struct Peer *peer_new(Wnd *w, int kind)
{
    struct Peer *p = calloc(1, sizeof *p);

    p->kind = kind;
    p->c = view_new(w, p);
    if (!p->c) {
        free(p);
        return NULL;
    }
    w->peer = p;
    return p;
}

void peer_create(Wnd *w)
{
    int kind = kind_of(w);
    struct Peer *p;

    if (kind < 0 || w->peer)
        return;
    p = peer_new(w, kind);
    if (!p)
        return;
    p->ownPaint = is_label(p) && strcasecmp(w->cls->name, "Static") != 0;
    sync_one(w);
}

void peer_destroy(Wnd *w)
{
    if (!w->peer)
        return;
    if (w->peer->c)
        plat_control_destroy(w->peer->c);
    free(w->peer);
    w->peer = NULL;
}

/* the window's own background under a view that shows only a part of it */
static void fill_under(Wnd *w, HDC hdc)
{
    RECT r;
    int br = 0;

    GetClientRect(w->h, &r);
    if (w->parent && w->parent->dlg)
        ctl_color(w, WM_CTLCOLORSTATIC, hdc, &br);
    FillRect(hdc, &r, br ? (HBRUSH)(uintptr_t)br : GetSysColorBrush(COLOR_BTNFACE));
}

int peer_paint(Wnd *w)
{
    PAINTSTRUCT ps;
    HDC hdc;

    if (!w->peer || !w->peer->c)
        return 0;
    hdc = BeginPaint(w->h, &ps);
    if (is_label(w->peer))
        fill_under(w, hdc);
    EndPaint(w->h, &ps);
    if (w->peer->kind == PC_LIST_OWNERDRAW)
        w->peer->redraw = 1;    /* the game draws its rows: draw them again */
    return 1;
}

/* WM_PAINT for a window of the game's whose view shows all of it */
int peer_paints(Wnd *w)
{
    return w->peer && w->peer->ownPaint && peer_paint(w);
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

    return w && !w->destroyed && w->peer && w->peer->c ? w : NULL;
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
            w->peer->top_ = (int)SendMessageA(w->h, LB_GETTOPINDEX, 0, 0);
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

/* ---- What the game tells (windows.h): its skill panel and status bar ---- */

/* the view of a window of the game's that it is painting now; made the first time */
static struct Peer *game_peer(HWND hwnd, int kind)
{
    Wnd *w = wnd_get(hwnd);
    HDC hdc;

    if (!w || w->destroyed)
        return NULL;
    if (!w->peer && !peer_new(w, kind))
        return NULL;
    if (w->peer->kind != kind || !w->peer->c)
        return NULL;
    /* the view shows the face; under it the bar's own background */
    hdc = GetDC(hwnd);
    fill_under(w, hdc);
    ReleaseDC(hwnd, hdc);
    return w->peer;
}

/* one frame of an SRLE sprite (src/lemmings/icons.cpp, Button::DrawSprite) as
   0xAARRGGBB pixels, clear where the sprite has none */
static void sprite_frame(const BYTE *spr, int frame, const RGBQUAD *col, uint32_t *px, int w, int h)
{
    const BYTE *f, *src;
    int x0, y0, rows, j;

    memset(px, 0, (size_t)w * (size_t)h * 4);
    if (!spr || memcmp(spr, "SRLE", 4) || frame < 0 || frame >= *(const WORD *)(spr + 4))
        return;
    f = spr + *(const DWORD *)(spr + 6 + frame * 4);
    x0 = ((const WORD *)f)[0];
    y0 = ((const WORD *)f)[1];
    rows = ((const WORD *)f)[3];
    src = f + 12;
    for (j = 0; j < rows; j++) {
        int x = x0, y = y0 + j;
        BYTE c;

        do {
            c = *src++;
            if (c < 0x80)
                x += c;
            else if (c > 0x80) {
                for (c &= 0x7f; c; c--, x++, src++)
                    if (x >= 0 && x < w && y >= 0 && y < h)
                        px[y * w + x] = 0xff000000u | (uint32_t)col[*src].rgbRed << 16 |
                                        (uint32_t)col[*src].rgbGreen << 8 | col[*src].rgbBlue;
                c = 0;
            }
        } while (c != 0x80);
    }
}

int port_native_icon(HWND hwnd, const BYTE *sprite, int frame, const RGBQUAD *colours, int count, int pressed)
{
    enum { W = 34, H = 42 };
    struct Peer *p = game_peer(hwnd, PC_ICONBUTTON);
    uint32_t px[W * H];
    char num[16] = "";

    if (!p)
        return 0;
    if (count > 0)              /* none left, or a button without a number: blank, as the game has it */
        snprintf(num, sizeof num, "%02d", count % 100);
    sprite_frame(sprite, frame, colours, px, W, H);
    plat_iconbutton_set(p->c, px, W, H, num, pressed != 0);
    p->shown = 1;
    return 1;
}

int port_native_status(HWND hwnd, const char *const *texts, int n)
{
    struct Peer *p = game_peer(hwnd, PC_STATUSBAR);
    char u[8][256];
    const char *parts[8];
    int i;

    if (!p)
        return 0;
    if (n > 8)
        n = 8;
    for (i = 0; i < n; i++) {   /* the game's panes run from the right */
        port_utf8(texts[n - 1 - i], u[i], sizeof u[i]);
        parts[i] = u[i];
    }
    plat_statusbar_set(p->c, n, parts);
    p->shown = 1;
    return 1;
}

/* the harness: what the game's views show ("views") */
void peer_log_views(Wnd *w)
{
    Wnd *c;

    for (c = w->child; c; c = c->next) {
        if (c->peer && c->peer->c && (c->peer->kind == PC_ICONBUTTON || c->peer->kind == PC_STATUSBAR || is_label(c->peer))) {
            char t[600];
            RECT r;

            plat_control_describe(c->peer->c, t, sizeof t);
            wnd_window_fb(c, &r);
            if (c->peer->kind == PC_ICON)
                snprintf(t, sizeof t, "%ld x %ld", (long)(r.right - r.left), (long)(r.bottom - r.top));
            port_harness_log("  view %s%s: %s", c->peer->kind == PC_ICONBUTTON ? "icon" : c->peer->kind == PC_STATUSBAR
                             ? "status" : c->peer->kind == PC_ICON ? "picture" : "label",
                             wnd_visible(c) ? "" : " (hidden)", t);
        }
        peer_log_views(c);
    }
}
