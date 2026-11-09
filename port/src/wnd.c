/*
 * Windows. A top-level window (no WS_CHILD) is a system window (plat.h) with
 * a 32-bit framebuffer the size of its content; its children are areas of
 * that framebuffer. The system window's content is the whole Win32 window:
 * its caption and frame are the system's, outside it, so a top-level window's
 * non-client area is only what the layer draws itself (the menu bar where the
 * system has none, the 1-pixel border of a popup). Child windows have Win32
 * non-client parts: borders and scroll bars. A dialog's standard controls
 * can be system views instead (peer.c).
 */
#include "port_internal.h"

void port_utf8(const char *s, char *out, size_t size);
HMENU menu_load_class(LPCSTR name);
void menu_destroy_bar(Wnd *w);

Wnd *port_top;
Wnd *port_desktop;
Wnd *port_capture;
Wnd *port_focus;
Wnd *port_active;

static WndClass *g_classes[128];
static int g_nclasses;

#define SCROLL_W 16

/* ---- Classes ---- */

WndClass *wnd_find_class(const char *name)
{
    int i;

    if (!name)
        return NULL;
    if (IS_INTRESOURCE(name)) {
        char n[16];

        snprintf(n, sizeof n, "#%u", (unsigned)(uintptr_t)name);
        return wnd_find_class(n);
    }
    for (i = 0; i < g_nclasses; i++)
        if (!strcasecmp(g_classes[i]->name, name))
            return g_classes[i];
    return NULL;
}

ATOM WINAPI RegisterClassA(const WNDCLASSA *wc)
{
    WndClass *c;

    if (!wc->lpszClassName || wnd_find_class(wc->lpszClassName) || g_nclasses >= 128)
        return 0;
    c = calloc(1, sizeof *c);
    port_strlcpy(c->name, wc->lpszClassName, sizeof c->name);
    c->wc = *wc;
    c->wc.lpszClassName = c->name;
    g_classes[g_nclasses++] = c;
    return (ATOM)(0xc000 + g_nclasses);
}

void wnd_register_builtin(const char *name, WNDPROC proc, UINT style, int extra, int cursor)
{
    WNDCLASSA wc;

    memset(&wc, 0, sizeof wc);
    wc.style = style;
    wc.lpfnWndProc = proc;
    wc.cbWndExtra = extra;
    wc.hCursor = cursor ? LoadCursorA(NULL, IDC_ARROW) : NULL;
    wc.lpszClassName = name;
    RegisterClassA(&wc);
    wnd_find_class(name)->builtin = 1;
}

/* ---- Lookups ---- */

Wnd *wnd_get(HWND h)
{
    Wnd *w = (Wnd *)port_handle_get(h, H_WND);

    return w && !w->destroyed ? w : NULL;
}

BOOL WINAPI IsWindow(HWND h)
{
    return wnd_get(h) != NULL;
}

Wnd *wnd_toplevel(Wnd *w)
{
    while (w && w->parent)
        w = w->parent;
    return w;
}

int wnd_visible(Wnd *w)
{
    for (; w; w = w->parent) {
        if (!(w->style & WS_VISIBLE) || w->state == SIZE_MINIMIZED)
            return 0;
    }
    return 1;
}

BOOL WINAPI IsWindowVisible(HWND h)
{
    Wnd *w = wnd_get(h);

    return w && wnd_visible(w);
}

LRESULT wnd_send(Wnd *w, UINT msg, WPARAM wp, LPARAM lp)
{
    return w ? SendMessageA(w->h, msg, wp, lp) : 0;
}

/* ---- Geometry ---- */

static int is_control_class(Wnd *w, const char *name)
{
    return w->cls && !strcasecmp(w->cls->name, name);
}

static int border_of(DWORD style, DWORD exStyle, int child, Wnd *w)
{
    (void)exStyle;
    if (!child) {
        /* the system draws a captioned or resizable window's frame */
        if ((style & WS_CAPTION) == WS_CAPTION || (style & WS_THICKFRAME))
            return 0;
        return (style & (WS_BORDER | WS_DLGFRAME)) ? 1 : 0;
    }
    if (style & WS_DLGFRAME)
        return 2;
    if (style & WS_BORDER) {
        /* edit and list boxes in dialogs: the 3-D look of Windows 95 */
        if (w && w->parent && wnd_is_dialog(w->parent) &&
            (is_control_class(w, "Edit") || is_control_class(w, "ListBox")))
            return 2;
        return 1;
    }
    return 0;
}

void wnd_client_offset(Wnd *w, int *left, int *top, int *right, int *bottom)
{
    int b = border_of(w->style, w->exStyle, w->parent != NULL, w);

    *left = *top = *right = *bottom = b;
    if (!w->parent && w->menu)
        *top += menu_bar_height(w);
    if (w->sb[SB_VERT].shown)
        *right += SCROLL_W;
    if (w->sb[SB_HORZ].shown)
        *bottom += SCROLL_W;
}

BOOL WINAPI AdjustWindowRectEx(LPRECT r, DWORD style, BOOL menu, DWORD exStyle)
{
    int b = border_of(style, exStyle, (style & WS_CHILD) != 0, NULL);

    r->left -= b;
    r->top -= b;
    r->right += b;
    r->bottom += b;
    if (menu && !(style & WS_CHILD))
        r->top -= GetSystemMetrics(SM_CYMENU);
    return TRUE;
}

BOOL WINAPI AdjustWindowRect(LPRECT r, DWORD style, BOOL menu)
{
    return AdjustWindowRectEx(r, style, menu, 0);
}

/* the window rect in its top-level window's framebuffer */
void wnd_window_fb(Wnd *w, RECT *r)
{
    if (!w->parent) {
        *r = (RECT){0, 0, w->rc.right - w->rc.left, w->rc.bottom - w->rc.top};
        return;
    }
    wnd_client_fb(w->parent, r);
    *r = (RECT){r->left + w->rc.left, r->top + w->rc.top, r->left + w->rc.right, r->top + w->rc.bottom};
}

void wnd_client_fb(Wnd *w, RECT *r)
{
    int l, t, rr, b;

    wnd_window_fb(w, r);
    wnd_client_offset(w, &l, &t, &rr, &b);
    r->left += l;
    r->top += t;
    r->right -= rr;
    r->bottom -= b;
    if (r->right < r->left)
        r->right = r->left;
    if (r->bottom < r->top)
        r->bottom = r->top;
}

static void client_size(Wnd *w, int *cw, int *ch)
{
    RECT r;

    wnd_client_fb(w, &r);
    *cw = r.right - r.left;
    *ch = r.bottom - r.top;
}

/* the client area's origin on the screen */
static void client_origin(Wnd *w, int *x, int *y)
{
    Wnd *top = wnd_toplevel(w);
    RECT r;

    wnd_client_fb(w, &r);
    *x = top->rc.left + r.left;
    *y = top->rc.top + r.top;
}

BOOL WINAPI GetClientRect(HWND h, LPRECT r)
{
    Wnd *w = wnd_get(h);
    int cw, ch;

    if (!w) {
        memset(r, 0, sizeof *r);
        return FALSE;
    }
    if (w == port_desktop) {
        *r = (RECT){0, 0, w->rc.right - w->rc.left, w->rc.bottom - w->rc.top};
        return TRUE;
    }
    client_size(w, &cw, &ch);
    *r = (RECT){0, 0, cw, ch};
    return TRUE;
}

BOOL WINAPI GetWindowRect(HWND h, LPRECT r)
{
    Wnd *w = wnd_get(h);
    int x, y;

    if (!w) {
        memset(r, 0, sizeof *r);
        return FALSE;
    }
    if (!w->parent) {
        *r = w->rc;
        return TRUE;
    }
    client_origin(w->parent, &x, &y);
    *r = (RECT){x + w->rc.left, y + w->rc.top, x + w->rc.right, y + w->rc.bottom};
    return TRUE;
}

BOOL WINAPI ClientToScreen(HWND h, LPPOINT pt)
{
    Wnd *w = wnd_get(h);
    int x, y;

    if (!w)
        return FALSE;
    if (w == port_desktop)
        return TRUE;
    client_origin(w, &x, &y);
    pt->x += x;
    pt->y += y;
    return TRUE;
}

BOOL WINAPI ScreenToClient(HWND h, LPPOINT pt)
{
    Wnd *w = wnd_get(h);
    int x, y;

    if (!w)
        return FALSE;
    if (w == port_desktop)
        return TRUE;
    client_origin(w, &x, &y);
    pt->x -= x;
    pt->y -= y;
    return TRUE;
}

int WINAPI MapWindowPoints(HWND from, HWND to, LPPOINT pts, UINT n)
{
    int fx = 0, fy = 0, tx = 0, ty = 0;
    UINT i;

    if (from && wnd_get(from) && wnd_get(from) != port_desktop)
        client_origin(wnd_get(from), &fx, &fy);
    if (to && wnd_get(to) && wnd_get(to) != port_desktop)
        client_origin(wnd_get(to), &tx, &ty);
    for (i = 0; i < n; i++) {
        pts[i].x += fx - tx;
        pts[i].y += fy - ty;
    }
    return MAKELONG(fx - tx, fy - ty);
}

/* ---- Visible regions and invalidation ---- */

void wnd_visible_region(Wnd *w, int client, Region *g)
{
    RECT r;
    Wnd *a, *s, *c;

    g->n = 0;
    if (!wnd_visible(w))
        return;
    if (client)
        wnd_client_fb(w, &r);
    else
        wnd_window_fb(w, &r);
    rgn_set(g, &r);
    for (a = w->parent; a; a = a->parent) {
        RECT pc;

        wnd_client_fb(a, &pc);
        rgn_intersect(g, &pc);
    }
    {
        Wnd *top = wnd_toplevel(w);
        RECT fbr = {0, 0, top->fb.w, top->fb.h};

        rgn_intersect(g, &fbr);
    }
    if (client && (w->style & WS_CLIPCHILDREN)) {
        for (c = w->child; c; c = c->next) {
            if (c->style & WS_VISIBLE) {
                RECT cr;

                wnd_window_fb(c, &cr);
                rgn_subtract(g, &cr);
            }
        }
    }
    for (a = w; a && a->parent; a = a->parent) {
        if (!(a->style & WS_CLIPSIBLINGS))
            continue;
        for (s = a->prev; s; s = s->prev) {
            if (s->style & WS_VISIBLE) {
                RECT sr;

                wnd_window_fb(s, &sr);
                rgn_subtract(g, &sr);
            }
        }
    }
}

static int g_paintPending;

void wnd_invalidate(Wnd *w, const RECT *rect, int erase)
{
    int cw, ch;
    RECT r;
    Wnd *c;

    if (!w || w->destroyed || w == port_desktop)
        return;
    client_size(w, &cw, &ch);
    r = rect ? *rect : (RECT){0, 0, cw, ch};
    if (!rect_intersect(&r, &r, &(RECT){0, 0, cw, ch}))
        return;
    if (w->invalid) {
        if (r.left < w->inv.left)
            w->inv.left = r.left;
        if (r.top < w->inv.top)
            w->inv.top = r.top;
        if (r.right > w->inv.right)
            w->inv.right = r.right;
        if (r.bottom > w->inv.bottom)
            w->inv.bottom = r.bottom;
    } else {
        w->inv = r;
        w->invalid = 1;
    }
    w->erase |= erase;
    g_paintPending = 1;
    /* without WS_CLIPCHILDREN, the parent's painting covers its children */
    if (!(w->style & WS_CLIPCHILDREN)) {
        for (c = w->child; c; c = c->next) {
            RECT cr = c->rc, x;
            int l, t, rr, b;

            if (!(c->style & WS_VISIBLE) || !rect_intersect(&x, &cr, &r))
                continue;
            wnd_client_offset(c, &l, &t, &rr, &b);
            OffsetRect(&x, -c->rc.left - l, -c->rc.top - t);
            wnd_invalidate(c, &x, erase);
        }
    }
}

/* everything again: after a resize, a show, a move of a child */
void wnd_redraw_all(Wnd *w)
{
    Wnd *c;

    if (!w)
        return;
    wnd_invalidate(w, NULL, 1);
    for (c = w->child; c; c = c->next)
        wnd_redraw_all(c);
}

/* the parent area a child covered (or covers) */
static void invalidate_under(Wnd *w)
{
    if (w->parent) {
        RECT r = w->rc;
        int l, t, rr, b;

        wnd_client_offset(w->parent, &l, &t, &rr, &b);
        (void)rr;
        (void)b;
        wnd_invalidate(w->parent, &r, 1);
        {
            Wnd *s;

            /* siblings under it show through */
            for (s = w->parent->child; s; s = s->next) {
                RECT x;

                if (s != w && (s->style & WS_VISIBLE) && rect_intersect(&x, &s->rc, &r))
                    wnd_redraw_all(s);
            }
        }
    }
}

static Wnd *find_paint(Wnd *w)
{
    Wnd *c, *f;

    for (; w; w = w->next) {
        if (!wnd_visible(w))
            continue;
        if (w->invalid)
            return w;
        for (c = w->child; c; c = c->next)
            if ((f = find_paint(c)) != NULL)
                return f;
    }
    return NULL;
}

Wnd *wnd_find_paint(void)
{
    Wnd *w;

    if (!g_paintPending)
        return NULL;
    w = find_paint(port_top);
    if (!w)
        g_paintPending = 0;
    return w;
}

BOOL WINAPI InvalidateRect(HWND h, const RECT *r, BOOL erase)
{
    Wnd *w;

    if (!h) {
        for (w = port_top; w; w = w->next)
            wnd_redraw_all(w);
        return TRUE;
    }
    w = wnd_get(h);
    if (!w)
        return FALSE;
    wnd_invalidate(w, r, erase);
    return TRUE;
}

BOOL WINAPI ValidateRect(HWND h, const RECT *r)
{
    Wnd *w = wnd_get(h);

    (void)r;
    if (w)
        w->invalid = w->erase = 0;
    return w != NULL;
}

/* ---- Non-client painting: borders, scroll bars, the menu bar ---- */

int wnd_scrollbar_rect(Wnd *w, int bar, RECT *r)
{
    RECT wr = {0, 0, w->rc.right - w->rc.left, w->rc.bottom - w->rc.top};
    int b = border_of(w->style, w->exStyle, w->parent != NULL, w);

    if (!w->sb[bar].shown)
        return 0;
    if (bar == SB_VERT)
        *r = (RECT){wr.right - b - SCROLL_W, wr.top + b, wr.right - b,
                    wr.bottom - b - (w->sb[SB_HORZ].shown ? SCROLL_W : 0)};
    else
        *r = (RECT){wr.left + b, wr.bottom - b - SCROLL_W, wr.right - b - (w->sb[SB_VERT].shown ? SCROLL_W : 0),
                    wr.bottom - b};
    if (!w->parent && w->menu) {
        int m = menu_bar_height(w);

        if (bar == SB_VERT)
            r->top += m;
    }
    return 1;
}

int wnd_scroll_page(Wnd *w, int bar)
{
    int cw, ch;

    client_size(w, &cw, &ch);
    return bar == SB_VERT ? ch : cw;
}

void wnd_nc_paint(Wnd *w)
{
    HDC hdc;
    PDC *dc;
    int b, wv, wh, bar;

    if (!wnd_visible(w) || w == port_desktop)
        return;
    hdc = dc_for_window(w, 0, NULL);
    dc = dc_get(hdc);
    wv = w->rc.right - w->rc.left;
    wh = w->rc.bottom - w->rc.top;
    b = border_of(w->style, w->exStyle, w->parent != NULL, w);
    if (b == 1)
        gdi_frame3d(dc, &(RECT){0, 0, wv, wh}, 0, 0);
    else if (b == 2) {
        gdi_frame3d(dc, &(RECT){0, 0, wv, wh}, sys_xrgb(COLOR_BTNSHADOW), sys_xrgb(COLOR_BTNHIGHLIGHT));
        gdi_frame3d(dc, &(RECT){1, 1, wv - 1, wh - 1}, 0, sys_xrgb(COLOR_3DLIGHT));
    }
    for (bar = 0; bar < 2; bar++) {
        RECT r;

        if (!wnd_scrollbar_rect(w, bar, &r))
            continue;
        if (w->sb[bar].view)        /* the system's scroller shows it: only its background here */
            gdi_fill(dc, &r, sys_xrgb(COLOR_BTNFACE));
        else
            draw_scrollbar(dc, &r, bar == SB_VERT, w->sb[bar].min, w->sb[bar].max, w->sb[bar].pos,
                           wnd_scroll_page(w, bar), !(w->style & WS_DISABLED));
    }
    if (w->sb[0].shown && w->sb[1].shown)
        gdi_fill(dc, &(RECT){wv - b - SCROLL_W, wh - b - SCROLL_W, wv - b, wh - b}, sys_xrgb(COLOR_BTNFACE));
    if (!w->parent && w->menu)
        menu_bar_draw(w);
    ReleaseDC(w->h, hdc);
}

static void nc_paint_tree(Wnd *w)
{
    Wnd *c;

    wnd_nc_paint(w);
    for (c = w->child; c; c = c->next)
        if (c->style & WS_VISIBLE)
            nc_paint_tree(c);
}

/* ---- Top-level windows' system side ---- */

void wnd_surface_resize(Wnd *w, int width, int height)
{
    Surface *s = &w->fb;
    BYTE *old = s->bits;
    int ow = s->w, oh = s->h, y;

    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;
    if (old && ow == width && oh == height)
        return;
    s->w = width;
    s->h = height;
    s->bpp = 32;
    s->pitch = width * 4;
    s->bottomUp = 0;
    s->bits = calloc((size_t)width * (size_t)height, 4);
    s->ownBits = 1;
    {
        uint32_t face = sys_xrgb(COLOR_BTNFACE);
        uint32_t *p = (uint32_t *)s->bits;
        size_t i, n = (size_t)width * (size_t)height;

        for (i = 0; i < n; i++)
            p[i] = face;
    }
    if (old) {
        for (y = 0; y < oh && y < height; y++)
            memcpy(s->bits + (size_t)y * s->pitch, old + (size_t)y * ow * 4, (size_t)(ow < width ? ow : width) * 4);
        free(old);
    }
    w->dirty = 1;
}

static void top_create_plat(Wnd *w)
{
    int width = w->rc.right - w->rc.left, height = w->rc.bottom - w->rc.top, flags = 0;
    char title[512];
    PlatWindow *owner = w->owner && w->owner != port_desktop ? w->owner->plat : NULL;

    if (w == port_desktop || w->plat)
        return;
    if ((w->style & WS_CAPTION) == WS_CAPTION || (w->style & WS_THICKFRAME))
        flags |= PW_CAPTION;
    if (w->style & WS_THICKFRAME)
        flags |= PW_RESIZABLE;
    if (w->exStyle & WS_EX_TOPMOST)
        flags |= PW_TOPMOST;
    if (owner && (w->style & WS_POPUP))
        flags |= (w->style & WS_CAPTION) == WS_CAPTION ? PW_DIALOG : PW_UTILITY;
    if (width < 1)
        width = 1;
    if (height < 1)
        height = 1;
    port_utf8(w->text ? w->text : "", title, sizeof title);
    w->plat = plat_window_create(w, w->rc.left, w->rc.top, width, height, flags, owner, title);
    if (!w->plat)
        port_fatal("Unable to create a window");
    wnd_surface_resize(w, width, height);
}

static void top_destroy_plat(Wnd *w)
{
    if (w->plat)
        plat_window_destroy(w->plat);
    w->plat = NULL;
}

static void update_minmax(Wnd *w)
{
    MINMAXINFO mm;

    if (!w->plat || !(w->style & WS_THICKFRAME))
        return;
    memset(&mm, 0, sizeof mm);
    mm.ptMaxSize.x = GetSystemMetrics(SM_CXSCREEN);
    mm.ptMaxSize.y = GetSystemMetrics(SM_CYSCREEN);
    mm.ptMinTrackSize.x = 100;
    mm.ptMinTrackSize.y = 50;
    mm.ptMaxTrackSize = mm.ptMaxSize;
    SendMessageA(w->h, WM_GETMINMAXINFO, 0, (LPARAM)&mm);
    if (mm.ptMinTrackSize.x != w->minW || mm.ptMinTrackSize.y != w->minH) {
        w->minW = mm.ptMinTrackSize.x;
        w->minH = mm.ptMinTrackSize.y;
        plat_window_set_minmax(w->plat, w->minW, w->minH, w->maxW, w->maxH);
    }
    if (mm.ptMaxTrackSize.x != w->maxW || mm.ptMaxTrackSize.y != w->maxH) {
        w->maxW = mm.ptMaxTrackSize.x;
        w->maxH = mm.ptMaxTrackSize.y;
        plat_window_set_minmax(w->plat, w->minW, w->minH, w->maxW, w->maxH);
    }
}

void wnd_send_size(Wnd *w)
{
    int cw, ch, x, y;
    RECT r;

    client_size(w, &cw, &ch);
    SendMessageA(w->h, WM_SIZE, (WPARAM)w->state, MAKELPARAM(cw, ch));
    if (!wnd_get(w->h))
        return;
    if (w->parent) {
        wnd_client_fb(w, &r);
        {
            RECT pr;

            wnd_client_fb(w->parent, &pr);
            x = r.left - pr.left;
            y = r.top - pr.top;
        }
    } else
        client_origin(w, &x, &y);
    SendMessageA(w->h, WM_MOVE, 0, MAKELPARAM(x, y));
}

/* the system resized a top-level window */
void wnd_top_resized(Wnd *w, int width, int height)
{
    if (w->rc.right - w->rc.left == width && w->rc.bottom - w->rc.top == height)
        return;
    w->rc.right = w->rc.left + width;
    w->rc.bottom = w->rc.top + height;
    wnd_surface_resize(w, width, height);
    wnd_send_size(w);
    update_minmax(w);
    wnd_redraw_all(w);
    nc_paint_tree(w);
}

void wnd_top_moved(Wnd *w, int x, int y)
{
    int cx, cy;

    if (w->rc.left == x && w->rc.top == y)
        return;
    OffsetRect(&w->rc, x - w->rc.left, y - w->rc.top);
    client_origin(w, &cx, &cy);
    SendMessageA(w->h, WM_MOVE, 0, MAKELPARAM(cx, cy));
}

void wnd_top_state(Wnd *w, int state)
{
    if (w->state == state)
        return;
    w->state = state;
    if (state == SIZE_MAXIMIZED)
        w->style |= WS_MAXIMIZE;
    else
        w->style &= ~WS_MAXIMIZE;
    wnd_send_size(w);
    if (state != SIZE_MINIMIZED) {
        wnd_redraw_all(w);
        nc_paint_tree(w);
    }
}

/* ---- Create and destroy ---- */

static void link_child(Wnd *w, Wnd *parent)
{
    Wnd **p;

    w->parent = parent;
    w->prev = NULL;
    w->next = NULL;
    if (!parent->child) {
        parent->child = w;
        return;
    }
    for (p = &parent->child; (*p)->next; p = &(*p)->next)
        ;
    (*p)->next = w;
    w->prev = *p;
}

static void link_top(Wnd *w)
{
    w->parent = NULL;
    w->prev = NULL;
    w->next = port_top;
    if (port_top)
        port_top->prev = w;
    port_top = w;
}

static void unlink(Wnd *w)
{
    if (w->prev)
        w->prev->next = w->next;
    else if (w->parent)
        w->parent->child = w->next;
    else if (port_top == w)
        port_top = w->next;
    if (w->next)
        w->next->prev = w->prev;
    w->next = w->prev = NULL;
}

static void default_position(int *x, int *y, int w, int h)
{
    int bx, by, bw, bh;
    static int cascade;

    plat_usable_bounds(&bx, &by, &bw, &bh);
    *x = bx + (bw - w) / 2 + cascade;
    *y = by + (bh - h) / 3 + cascade;
    if (*x < bx)
        *x = bx;
    if (*y < by)
        *y = by;
    cascade = (cascade + 24) % 96;
}

HWND WINAPI CreateWindowExA(DWORD exStyle, LPCSTR clsName, LPCSTR name, DWORD style, int x, int y, int width,
                            int height, HWND parent, HMENU menu, HINSTANCE inst, LPVOID param)
{
    WndClass *cls = wnd_find_class(clsName);
    Wnd *w, *pw = parent ? wnd_get(parent) : NULL;
    CREATESTRUCTA cs;
    LRESULT r;

    if (!cls) {
        port_trace("CreateWindowEx: no class %s", IS_INTRESOURCE(clsName) ? "#atom" : clsName);
        port_last_error = 1407;
        return NULL;
    }
    if ((style & WS_CHILD) && !pw) {
        port_last_error = 1406;
        return NULL;
    }
    w = calloc(1, sizeof *w);
    w->h = (HWND)port_handle_new(H_WND, w);
    w->cls = cls;
    w->proc = cls->wc.lpfnWndProc;
    w->style = style;
    w->exStyle = exStyle;
    w->state = SIZE_RESTORED;
    w->text = strdup(name ? name : "");
    w->nextra = cls->wc.cbWndExtra + 8;
    w->extra = calloc(1, (size_t)w->nextra);
    if (style & WS_CHILD) {
        w->id = (int)(intptr_t)menu;
        if (x == CW_USEDEFAULT)
            x = y = 0;
        if (width == CW_USEDEFAULT)
            width = height = 0;
        w->rc = (RECT){x, y, x + width, y + height};
        link_child(w, pw);
    } else {
        w->owner = pw ? wnd_toplevel(pw) : NULL;
        if (width == CW_USEDEFAULT) {
            width = 640;
            height = 480;
        }
        if (x == CW_USEDEFAULT)
            default_position(&x, &y, width, height);
        w->rc = (RECT){x, y, x + width, y + height};
        w->menu = menu;
        if (!w->menu && cls->wc.lpszMenuName)
            w->menu = LoadMenuA(inst, cls->wc.lpszMenuName);
        link_top(w);
        top_create_plat(w);
    }
    port_trace("CreateWindow %s '%s' %08x -> %p", cls->name, w->text, style, (void *)w->h);
    memset(&cs, 0, sizeof cs);
    cs.lpCreateParams = param;
    cs.hInstance = inst;
    cs.hMenu = menu;
    cs.hwndParent = parent;
    cs.cx = width;
    cs.cy = height;
    cs.x = x;
    cs.y = y;
    cs.style = (LONG)style;
    cs.lpszName = name;
    cs.lpszClass = cls->name;
    cs.dwExStyle = exStyle;
    if (!SendMessageA(w->h, WM_NCCREATE, 0, (LPARAM)&cs)) {
        DestroyWindow(w->h);
        return NULL;
    }
    r = SendMessageA(w->h, WM_CREATE, 0, (LPARAM)&cs);
    if (r == -1 || !wnd_get(w->h)) {
        if (wnd_get(w->h))
            DestroyWindow(w->h);
        return NULL;
    }
    wnd_send_size(w);
    if (!wnd_get(w->h))
        return NULL;
    update_minmax(w);
    if (style & WS_CHILD)
        peer_create(w);     /* a dialog's control: the system's own view, where there is one */
    if (style & WS_VISIBLE) {
        w->style &= ~WS_VISIBLE;
        ShowWindow(w->h, (style & WS_MAXIMIZE) && !(style & WS_CHILD) ? SW_SHOWMAXIMIZED : SW_SHOW);
    }
    return w->h;
}

static void destroy_tree(Wnd *w)
{
    Wnd *c;

    SendMessageA(w->h, WM_DESTROY, 0, 0);
    for (c = w->child; c; c = c->next)
        destroy_tree(c);
}

static void free_tree(Wnd *w)
{
    Wnd *c, *n;

    for (c = w->child; c; c = n) {
        n = c->next;
        free_tree(c);
    }
    SendMessageA(w->h, WM_NCDESTROY, 0, 0);
    w->destroyed = 1;
    if (port_capture == w) {
        port_capture = NULL;
        plat_capture_mouse(0);
    }
    if (port_focus == w)
        port_focus = NULL;
    if (port_active == w)
        port_active = NULL;
    msg_window_gone(w);
    peer_destroy(w);
    peer_scrollbars_gone(w);
    if (w->ctlFree)
        w->ctlFree(w);
    if (!w->parent) {
        menu_destroy_bar(w);
        top_destroy_plat(w);
        free(w->fb.bits);
        w->fb.bits = NULL;
    }
    free(w->text);
    free(w->extra);
    port_handle_free(w->h);
    w->child = NULL;
    /* the Wnd itself is kept (stale pointers in the game stay harmless) */
}

BOOL WINAPI DestroyWindow(HWND h)
{
    Wnd *w = wnd_get(h), *o, *next;

    if (!w || w == port_desktop)
        return FALSE;
    port_trace("DestroyWindow %p (%s)", (void *)h, w->cls->name);
    /* owned top-level windows go first */
    if (!w->parent) {
        for (o = port_top; o; o = next) {
            next = o->next;
            if (o->owner == w && !o->destroyed)
                DestroyWindow(o->h);
        }
    }
    if (w->style & WS_VISIBLE) {
        if (!w->parent && w->plat)
            plat_window_show(w->plat, 0, 0);
        else
            invalidate_under(w);
    }
    if (!w->parent && port_active == w && w->owner && !w->owner->destroyed) {
        port_active = w->owner;
        if (w->owner->plat)
            plat_window_raise(w->owner->plat);
    }
    destroy_tree(w);
    if (w->destroyed)
        return TRUE;
    unlink(w);
    free_tree(w);
    return TRUE;
}

/* ---- Showing, moving ---- */

BOOL WINAPI ShowWindow(HWND h, int cmd)
{
    Wnd *w = wnd_get(h);
    int was;

    if (!w || w == port_desktop)
        return FALSE;
    was = (w->style & WS_VISIBLE) != 0;
    if (cmd == SW_HIDE) {
        if (!was)
            return FALSE;
        SendMessageA(h, WM_SHOWWINDOW, FALSE, 0);
        w->style &= ~WS_VISIBLE;
        if (!w->parent && w->plat)
            plat_window_show(w->plat, 0, 0);
        else
            invalidate_under(w);
        if (port_focus && wnd_toplevel(port_focus) == wnd_toplevel(w) && !wnd_visible(port_focus))
            port_focus = w->parent ? wnd_toplevel(w) : NULL;
        return TRUE;
    }
    if (!was) {
        SendMessageA(h, WM_SHOWWINDOW, TRUE, 0);
        if (!wnd_get(h))
            return FALSE;
        w->style |= WS_VISIBLE;
    }
    if (!w->parent && w->plat) {
        switch (cmd) {
        case SW_MINIMIZE:
        case SW_SHOWMINIMIZED:
        case SW_SHOWMINNOACTIVE:
            plat_window_show(w->plat, 1, 0);
            plat_window_minimize(w->plat);
            wnd_top_state(w, SIZE_MINIMIZED);
            break;
        case SW_SHOWMAXIMIZED:
            plat_window_show(w->plat, 1, 0);
            plat_window_maximize(w->plat);
            break;
        case SW_RESTORE:
            plat_window_show(w->plat, 1, 0);
            plat_window_restore(w->plat);
            wnd_top_state(w, SIZE_RESTORED);
            break;
        default:
            plat_window_show(w->plat, 1, 0);
            if (cmd != SW_SHOWNA && cmd != SW_SHOWNOACTIVATE)
                plat_window_raise(w->plat);
            break;
        }
        if (!was) {
            wnd_redraw_all(w);
            nc_paint_tree(w);
        }
    } else if (!was) {
        wnd_redraw_all(w);
        nc_paint_tree(w);
        invalidate_under(w);
    }
    return was;
}

BOOL WINAPI SetWindowPos(HWND h, HWND after, int x, int y, int cx, int cy, UINT flags)
{
    Wnd *w = wnd_get(h);
    RECT old;
    int moved, sized;

    if (!w || w == port_desktop)
        return FALSE;
    old = w->rc;
    if (flags & SWP_NOMOVE) {
        x = w->rc.left;
        y = w->rc.top;
    }
    if (flags & SWP_NOSIZE) {
        cx = w->rc.right - w->rc.left;
        cy = w->rc.bottom - w->rc.top;
    }
    if (cx < 0)
        cx = 0;
    if (cy < 0)
        cy = 0;
    if (!w->parent && (w->style & WS_THICKFRAME) && !(flags & SWP_NOSIZE)) {
        /* as DefWindowProc's WM_WINDOWPOSCHANGING: the window's limits now */
        update_minmax(w);
        if (cx < w->minW)
            cx = w->minW;
        if (cy < w->minH)
            cy = w->minH;
        if (w->maxW && cx > w->maxW)
            cx = w->maxW;
        if (w->maxH && cy > w->maxH)
            cy = w->maxH;
    }
    moved = x != old.left || y != old.top;
    sized = cx != old.right - old.left || cy != old.bottom - old.top;
    if (moved || sized) {
        if (w->parent && (w->style & WS_VISIBLE))
            invalidate_under(w);
        w->rc = (RECT){x, y, x + cx, y + cy};
        if (!w->parent && w->plat) {
            if (sized)
                wnd_surface_resize(w, cx, cy);
            plat_window_set_rect(w->plat, x, y, cx, cy);
        }
    }
    if (!(flags & SWP_NOZORDER) && w->parent && (after == HWND_TOP || after == NULL)) {
        Wnd *p = w->parent;

        unlink(w);
        w->parent = p;
        w->next = p->child;
        if (p->child)
            p->child->prev = w;
        p->child = w;
    }
    if (flags & SWP_HIDEWINDOW)
        ShowWindow(h, SW_HIDE);
    if (moved || sized) {
        if (sized)
            wnd_send_size(w);
        else if (wnd_get(h)) {
            int px, py;
            RECT r, pr;

            if (w->parent) {
                wnd_client_fb(w, &r);
                wnd_client_fb(w->parent, &pr);
                px = r.left - pr.left;
                py = r.top - pr.top;
            } else
                client_origin(w, &px, &py);
            SendMessageA(h, WM_MOVE, 0, MAKELPARAM(px, py));
        }
        if (!wnd_get(h))
            return TRUE;
        if (w->style & WS_VISIBLE) {
            wnd_redraw_all(w);
            nc_paint_tree(w);
        }
    }
    if (flags & SWP_SHOWWINDOW)
        ShowWindow(h, SW_SHOW);
    return TRUE;
}

BOOL WINAPI MoveWindow(HWND h, int x, int y, int width, int height, BOOL repaint)
{
    (void)repaint;
    return SetWindowPos(h, NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL WINAPI IsIconic(HWND h)
{
    Wnd *w = wnd_get(h);

    return w && w->state == SIZE_MINIMIZED;
}

BOOL WINAPI IsZoomed(HWND h)
{
    Wnd *w = wnd_get(h);

    return w && w->state == SIZE_MAXIMIZED;
}

BOOL WINAPI UpdateWindow(HWND h)
{
    Wnd *w = wnd_get(h);

    if (w && w->invalid && wnd_visible(w))
        SendMessageA(h, WM_PAINT, 0, 0);
    return w != NULL;
}

/* ---- Relations ---- */

HWND WINAPI GetParent(HWND h)
{
    Wnd *w = wnd_get(h);

    if (!w)
        return NULL;
    if (w->parent)
        return w->parent->h;
    return w->owner && w->owner != port_desktop ? w->owner->h : NULL;
}

HWND WINAPI SetParent(HWND h, HWND parent)
{
    Wnd *w = wnd_get(h), *p = parent ? wnd_get(parent) : NULL, *old;
    int vis;

    if (!w)
        return NULL;
    old = w->parent ? w->parent : NULL;
    if (p == port_desktop)
        p = NULL;
    if (p == old)
        return old ? old->h : NULL;
    vis = (w->style & WS_VISIBLE) != 0;
    if (vis && old)
        invalidate_under(w);
    if (!old && w->plat) {
        if (vis)
            plat_window_show(w->plat, 0, 0);
    }
    unlink(w);
    if (p) {
        /* a top-level window becomes a child: it keeps its numbers, now in the parent's client */
        if (!old) {
            top_destroy_plat(w);
            free(w->fb.bits);
            memset(&w->fb, 0, sizeof w->fb);
        }
        link_child(w, p);
        w->owner = NULL;
        if (vis) {
            wnd_redraw_all(w);
            nc_paint_tree(w);
        }
    } else {
        link_top(w);
        w->owner = old ? wnd_toplevel(old) : NULL;
        top_create_plat(w);
        if (vis) {
            plat_window_show(w->plat, 1, 0);
            wnd_redraw_all(w);
            nc_paint_tree(w);
        }
    }
    return old ? old->h : port_desktop->h;
}

HWND WINAPI GetWindow(HWND h, UINT cmd)
{
    Wnd *w = wnd_get(h), *x = NULL;

    if (!w)
        return NULL;
    switch (cmd) {
    case GW_OWNER:
        x = w->parent ? NULL : (w->owner == port_desktop ? NULL : w->owner);
        break;
    case GW_CHILD:
        x = w->child;
        break;
    case GW_HWNDNEXT:
        x = w->next;
        break;
    case GW_HWNDPREV:
        x = w->prev;
        break;
    case GW_HWNDFIRST:
        x = w->parent ? w->parent->child : port_top;
        break;
    case GW_HWNDLAST:
        for (x = w; x->next; x = x->next)
            ;
        break;
    }
    return x ? x->h : NULL;
}

HWND WINAPI GetDesktopWindow(void)
{
    return port_desktop->h;
}

HWND WINAPI FindWindowA(LPCSTR cls, LPCSTR name)
{
    Wnd *w;

    for (w = port_top; w; w = w->next) {
        if (w->destroyed || w == port_desktop)
            continue;
        if (cls && strcasecmp(w->cls->name, cls))
            continue;
        if (name && strcmp(w->text, name))
            continue;
        return w->h;
    }
    return NULL;
}

HWND WINAPI ChildWindowFromPoint(HWND h, POINT pt)
{
    Wnd *w = wnd_get(h), *c;
    int cw, ch;

    if (!w)
        return NULL;
    client_size(w, &cw, &ch);
    if (pt.x < 0 || pt.y < 0 || pt.x >= cw || pt.y >= ch)
        return NULL;
    for (c = w->child; c; c = c->next)
        if (pt.x >= c->rc.left && pt.x < c->rc.right && pt.y >= c->rc.top && pt.y < c->rc.bottom)
            return c->h;
    return h;
}

/* the innermost visible, enabled window at a framebuffer point, with its hit test */
Wnd *wnd_from_point(Wnd *top, int fx, int fy, int *hit)
{
    Wnd *w = top, *c;
    RECT r;
    int found;

    if (!top || !wnd_visible(top))
        return NULL;
    do {
        found = 0;
        wnd_client_fb(w, &r);
        if (fx < r.left || fy < r.top || fx >= r.right || fy >= r.bottom)
            break;
        for (c = w->child; c; c = c->next) {
            RECT cr;

            if (!(c->style & WS_VISIBLE))
                continue;
            wnd_window_fb(c, &cr);
            if (fx >= cr.left && fx < cr.right && fy >= cr.top && fy < cr.bottom) {
                /* as Windows: a window that says HTTRANSPARENT (group boxes, static
                   text) lets the mouse through to the windows under it */
                if (SendMessageA(c->h, WM_NCHITTEST, 0, MAKELPARAM(top->rc.left + fx, top->rc.top + fy)) ==
                    HTTRANSPARENT)
                    continue;
                w = c;
                found = 1;
                break;
            }
        }
    } while (found);
    if (hit) {
        RECT wr, sbr;
        int bar;

        wnd_window_fb(w, &wr);
        wnd_client_fb(w, &r);
        *hit = HTCLIENT;
        if (fx < r.left || fy < r.top || fx >= r.right || fy >= r.bottom) {
            *hit = HTBORDER;
            for (bar = 0; bar < 2; bar++)
                if (wnd_scrollbar_rect(w, bar, &sbr) && fx >= wr.left + sbr.left && fx < wr.left + sbr.right &&
                    fy >= wr.top + sbr.top && fy < wr.top + sbr.bottom)
                    *hit = bar == SB_VERT ? HTVSCROLL : HTHSCROLL;
            if (!w->parent && w->menu && fy < r.top && fy >= wr.top)
                *hit = HTMENU;
        }
    }
    return w;
}

HWND WINAPI WindowFromPoint(POINT pt)
{
    Wnd *t, *w;

    for (t = port_top; t; t = t->next) {
        if (t == port_desktop || !wnd_visible(t))
            continue;
        if (pt.x >= t->rc.left && pt.x < t->rc.right && pt.y >= t->rc.top && pt.y < t->rc.bottom) {
            w = wnd_from_point(t, pt.x - t->rc.left, pt.y - t->rc.top, NULL);
            return w ? w->h : t->h;
        }
    }
    return port_desktop->h;
}

/* ---- Longs, words, text ---- */

static LONG_PTR get_long(Wnd *w, int index, int size)
{
    switch (index) {
    case GWL_STYLE:
        return (LONG)w->style;
    case GWL_EXSTYLE:
        return (LONG)w->exStyle;
    case GWL_ID:
        return w->id;
    case GWL_USERDATA:
        return w->user;
    case GWL_WNDPROC:
        return (LONG_PTR)w->proc;
    case GWL_HWNDPARENT:
        return (LONG_PTR)(w->parent ? w->parent->h : w->owner ? w->owner->h : NULL);
    case GWL_HINSTANCE:
        return (LONG_PTR)GetModuleHandleA(NULL);
    }
    if (index >= 0 && index + size <= w->nextra) {
        if (size == 8) {
            LONG_PTR v;

            memcpy(&v, w->extra + index, 8);
            return v;
        }
        {
            LONG v;

            memcpy(&v, w->extra + index, 4);
            return v;
        }
    }
    port_last_error = 1413;
    return 0;
}

static LONG_PTR set_long(Wnd *w, int index, LONG_PTR value, int size)
{
    LONG_PTR old = get_long(w, index, size);

    switch (index) {
    case GWL_STYLE:
        w->style = (DWORD)value;
        return old;
    case GWL_EXSTYLE:
        w->exStyle = (DWORD)value;
        return old;
    case GWL_ID:
        w->id = (int)value;
        return old;
    case GWL_USERDATA:
        w->user = value;
        return old;
    case GWL_WNDPROC:
        w->proc = (WNDPROC)value;
        return old;
    }
    if (index >= 0 && index + size <= w->nextra) {
        if (size == 8)
            memcpy(w->extra + index, &value, 8);
        else {
            LONG v = (LONG)value;

            memcpy(w->extra + index, &v, 4);
        }
    }
    return old;
}

LONG WINAPI GetWindowLongA(HWND h, int index)
{
    Wnd *w = wnd_get(h);

    return w ? (LONG)get_long(w, index, 4) : 0;
}

LONG WINAPI SetWindowLongA(HWND h, int index, LONG value)
{
    Wnd *w = wnd_get(h);

    return w ? (LONG)set_long(w, index, value, 4) : 0;
}

LONG_PTR WINAPI GetWindowLongPtrA(HWND h, int index)
{
    Wnd *w = wnd_get(h);

    return w ? get_long(w, index, 8) : 0;
}

LONG_PTR WINAPI SetWindowLongPtrA(HWND h, int index, LONG_PTR value)
{
    Wnd *w = wnd_get(h);

    return w ? set_long(w, index, value, 8) : 0;
}

WORD WINAPI GetWindowWord(HWND h, int index)
{
    Wnd *w = wnd_get(h);
    WORD v = 0;

    if (w && index >= 0 && index + 2 <= w->nextra)
        memcpy(&v, w->extra + index, 2);
    return v;
}

WORD WINAPI SetWindowWord(HWND h, int index, WORD value)
{
    Wnd *w = wnd_get(h);
    WORD old = GetWindowWord(h, index);

    if (w && index >= 0 && index + 2 <= w->nextra)
        memcpy(w->extra + index, &value, 2);
    return old;
}

int WINAPI GetWindowTextA(HWND h, LPSTR buf, int size)
{
    if (size > 0)
        buf[0] = 0;
    return (int)SendMessageA(h, WM_GETTEXT, (WPARAM)size, (LPARAM)buf);
}

int WINAPI GetWindowTextLengthA(HWND h)
{
    return (int)SendMessageA(h, WM_GETTEXTLENGTH, 0, 0);
}

BOOL WINAPI SetWindowTextA(HWND h, LPCSTR text)
{
    return (BOOL)SendMessageA(h, WM_SETTEXT, 0, (LPARAM)text);
}

int WINAPI GetClassNameA(HWND h, LPSTR buf, int size)
{
    Wnd *w = wnd_get(h);

    if (!w || size <= 0)
        return 0;
    port_strlcpy(buf, w->cls->name, (size_t)size);
    return (int)strlen(buf);
}

/* ---- Enabling, focus, capture ---- */

BOOL WINAPI EnableWindow(HWND h, BOOL enable)
{
    Wnd *w = wnd_get(h);
    int was;

    if (!w)
        return FALSE;
    was = (w->style & WS_DISABLED) != 0;
    if (enable)
        w->style &= ~WS_DISABLED;
    else
        w->style |= WS_DISABLED;
    if (was != !enable) {
        SendMessageA(h, WM_ENABLE, enable, 0);
        if (!enable && port_focus == w)
            SetFocus(NULL);
        if (!w->parent && w->plat)
            ;   /* disabled top-level windows just ignore input (msg.c) */
        else
            wnd_invalidate(w, NULL, 1);
    }
    return was;
}

BOOL WINAPI IsWindowEnabled(HWND h)
{
    Wnd *w = wnd_get(h);

    return w && !(w->style & WS_DISABLED);
}

HWND WINAPI SetFocus(HWND h)
{
    Wnd *w = h ? wnd_get(h) : NULL, *old = port_focus;

    if (h && !w)
        return NULL;
    if (w == old)
        return old ? old->h : NULL;
    port_focus = w;
    if (old && !old->destroyed)
        SendMessageA(old->h, WM_KILLFOCUS, (WPARAM)(w ? w->h : NULL), 0);
    if (w && port_focus == w) {
        Wnd *top = wnd_toplevel(w);

        if (top != port_active && top->plat && wnd_visible(top))
            plat_window_raise(top->plat);
        SendMessageA(w->h, WM_SETFOCUS, (WPARAM)(old && !old->destroyed ? old->h : NULL), 0);
        if (port_focus == w)
            peer_focus(w);
    }
    return old && !old->destroyed ? old->h : NULL;
}

HWND WINAPI GetFocus(void)
{
    return port_focus ? port_focus->h : NULL;
}

HWND WINAPI GetActiveWindow(void)
{
    return port_active ? port_active->h : NULL;
}

HWND WINAPI SetActiveWindow(HWND h)
{
    Wnd *w = wnd_get(h), *old = port_active;

    if (w && !w->parent && w->plat)
        plat_window_raise(w->plat);
    if (w)
        port_active = wnd_toplevel(w);
    return old ? old->h : NULL;
}

BOOL WINAPI SetForegroundWindow(HWND h)
{
    SetActiveWindow(h);
    return TRUE;
}

HWND WINAPI SetCapture(HWND h)
{
    Wnd *w = wnd_get(h), *old = port_capture;

    port_capture = w;
    if (w)
        plat_capture_mouse(1);
    if (old && old != w && !old->destroyed)
        SendMessageA(old->h, WM_CAPTURECHANGED, 0, (LPARAM)h);
    return old ? old->h : NULL;
}

BOOL WINAPI ReleaseCapture(void)
{
    Wnd *old = port_capture;

    port_capture = NULL;
    plat_capture_mouse(0);
    if (old && !old->destroyed)
        SendMessageA(old->h, WM_CAPTURECHANGED, 0, 0);
    return TRUE;
}

HWND WINAPI GetCapture(void)
{
    return port_capture ? port_capture->h : NULL;
}

/* ---- Scroll bars ---- */

static void scroll_changed(Wnd *w, int bar, BOOL redraw)
{
    ScrollBar *s = &w->sb[bar];
    DWORD flag = bar == SB_VERT ? WS_VSCROLL : WS_HSCROLL;
    int shown = (w->style & flag) && s->max > s->min;

    if (s->pos < s->min)
        s->pos = s->min;
    if (s->pos > s->max)
        s->pos = s->max;
    if (shown != s->shown) {
        s->shown = shown;
        /* the client area changes: Windows sends WM_SIZE */
        wnd_send_size(w);
        if (!wnd_get(w->h))
            return;
        wnd_redraw_all(w);
        redraw = TRUE;
    }
    if (redraw)
        wnd_nc_paint(w);
}

int WINAPI SetScrollPos(HWND h, int bar, int pos, BOOL redraw)
{
    Wnd *w = wnd_get(h);
    int old;

    if (!w || bar < 0 || bar > 1)
        return 0;
    old = w->sb[bar].pos;
    w->sb[bar].pos = pos;
    scroll_changed(w, bar, redraw);
    return old;
}

int WINAPI GetScrollPos(HWND h, int bar)
{
    Wnd *w = wnd_get(h);

    return w && bar >= 0 && bar <= 1 ? w->sb[bar].pos : 0;
}

BOOL WINAPI SetScrollRange(HWND h, int bar, int min, int max, BOOL redraw)
{
    Wnd *w = wnd_get(h);

    if (!w || bar < 0 || bar > 1)
        return FALSE;
    w->sb[bar].min = min;
    w->sb[bar].max = max;
    scroll_changed(w, bar, redraw);
    return TRUE;
}

BOOL WINAPI GetScrollRange(HWND h, int bar, LPINT min, LPINT max)
{
    Wnd *w = wnd_get(h);

    if (!w || bar < 0 || bar > 1)
        return FALSE;
    *min = w->sb[bar].min;
    *max = w->sb[bar].max;
    return TRUE;
}

BOOL WINAPI ScrollWindow(HWND h, int dx, int dy, const RECT *rect, const RECT *clip)
{
    Wnd *w = wnd_get(h);
    Wnd *top;
    RECT c;
    Region g;
    int y, cw, ch;

    (void)rect;
    (void)clip;
    if (!w)
        return FALSE;
    top = wnd_toplevel(w);
    wnd_client_fb(w, &c);
    client_size(w, &cw, &ch);
    wnd_visible_region(w, 1, &g);
    /* moves the pixels of the fully visible client area; the rest is repainted */
    if (g.n == 1 && g.r[0].left == c.left && g.r[0].top == c.top && g.r[0].right == c.right &&
        g.r[0].bottom == c.bottom && abs(dx) < cw && abs(dy) < ch) {
        Surface *s = &top->fb;
        int w0 = cw - abs(dx), x0 = c.left + (dx < 0 ? -dx : 0), x1 = c.left + (dx > 0 ? dx : 0);

        if (dy <= 0) {
            for (y = c.top; y < c.bottom + dy; y++)
                memmove(s->bits + (size_t)y * s->pitch + (size_t)x1 * 4,
                        s->bits + (size_t)(y - dy) * s->pitch + (size_t)x0 * 4, (size_t)w0 * 4);
        } else {
            for (y = c.bottom - 1; y >= c.top + dy; y--)
                memmove(s->bits + (size_t)y * s->pitch + (size_t)x1 * 4,
                        s->bits + (size_t)(y - dy) * s->pitch + (size_t)x0 * 4, (size_t)w0 * 4);
        }
        top->dirty = 1;
        if (dx > 0)
            wnd_invalidate(w, &(RECT){0, 0, dx, ch}, 1);
        else if (dx < 0)
            wnd_invalidate(w, &(RECT){cw + dx, 0, cw, ch}, 1);
        if (dy > 0)
            wnd_invalidate(w, &(RECT){0, 0, cw, dy}, 1);
        else if (dy < 0)
            wnd_invalidate(w, &(RECT){0, ch + dy, cw, ch}, 1);
    } else
        wnd_invalidate(w, NULL, 1);
    {
        Wnd *k;

        for (k = w->child; k; k = k->next)
            OffsetRect(&k->rc, dx, dy);
    }
    return TRUE;
}

/* ---- System metrics ---- */

int WINAPI GetSystemMetrics(int index)
{

    switch (index) {
    case SM_CXSCREEN:
    case SM_CXFULLSCREEN:
        return port_desktop ? port_desktop->rc.right - port_desktop->rc.left : 1280;
    case SM_CYSCREEN:
    case SM_CYFULLSCREEN:
        return port_desktop ? port_desktop->rc.bottom - port_desktop->rc.top : 800;
    case SM_CXVSCROLL:
    case SM_CYHSCROLL:
    case SM_CYVSCROLL:
    case SM_CXHSCROLL:
    case SM_CYVTHUMB:
    case SM_CXHTHUMB:
        return SCROLL_W;
    case SM_CYCAPTION:
        return 0;               /* the system's caption is outside the window */
    case SM_CXBORDER:
    case SM_CYBORDER:
        return 1;
    case SM_CXDLGFRAME:
    case SM_CYDLGFRAME:
    case SM_CXFRAME:
    case SM_CYFRAME:
        return 0;
    case SM_CXICON:
    case SM_CYICON:
        return 32;
    case SM_CXCURSOR:
    case SM_CYCURSOR:
        return 32;
    case SM_CYMENU:
        return plat_menubar_native() ? 0 : 19;
    case SM_CXSIZE:
    case SM_CYSIZE:
        return 18;
    }
    return 0;
}

/* ---- The default window procedure ---- */

int menu_syskey(Wnd *w, WPARAM vk);

LRESULT WINAPI DefWindowProcA(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    Wnd *w = wnd_get(h);

    if (!w)
        return 0;
    switch (msg) {
    case WM_NCCREATE:
        return TRUE;
    case WM_CLOSE:
        DestroyWindow(h);
        return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;

            BeginPaint(h, &ps);
            EndPaint(h, &ps);
        }
        return 0;
    case WM_ERASEBKGND:
        {
            HBRUSH br = w->cls->wc.hbrBackground;
            RECT r;

            if (!br)
                return 0;
            GetClientRect(h, &r);
            FillRect((HDC)wp, &r, br);
        }
        return 1;
    case WM_SETTEXT:
        {
            const char *t = lp ? (const char *)lp : "";

            free(w->text);
            w->text = strdup(t);
            if (!w->parent && w->plat) {
                char u[512];

                port_utf8(t, u, sizeof u);
                plat_window_set_title(w->plat, u);
            }
        }
        return TRUE;
    case WM_GETTEXT:
        if (wp > 0) {
            port_strlcpy((char *)lp, w->text, (size_t)wp);
            return (LRESULT)strlen((char *)lp);
        }
        return 0;
    case WM_GETTEXTLENGTH:
        return (LRESULT)strlen(w->text);
    case WM_SETFONT:
        w->font = (HFONT)wp;
        if (LOWORD(lp))
            wnd_invalidate(w, NULL, 1);
        return 0;
    case WM_GETFONT:
        return (LRESULT)w->font;
    case WM_SETCURSOR:
        if (w->parent && SendMessageA(w->parent->h, WM_SETCURSOR, wp, lp))
            return TRUE;
        if ((short)LOWORD(lp) == HTCLIENT) {
            if (w->cls->wc.hCursor) {
                SetCursor(w->cls->wc.hCursor);
                return TRUE;
            }
            return FALSE;
        }
        SetCursor(LoadCursorA(NULL, IDC_ARROW));
        return FALSE;
    case WM_NCHITTEST:
        return HTCLIENT;
    case WM_MOUSEACTIVATE:
        return 1;
    case WM_NCACTIVATE:
        return TRUE;
    case WM_SYSCOMMAND:
        switch (wp & 0xfff0) {
        case SC_CLOSE:
            SendMessageA(h, WM_CLOSE, 0, 0);
            break;
        case SC_MINIMIZE:
            ShowWindow(h, SW_MINIMIZE);
            break;
        case SC_MAXIMIZE:
            ShowWindow(h, SW_MAXIMIZE);
            break;
        case SC_RESTORE:
            ShowWindow(h, SW_RESTORE);
            break;
        case SC_KEYMENU:
            menu_syskey(wnd_toplevel(w), lp);
            break;
        }
        return 0;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        if (wp == VK_F10 || (msg == WM_SYSKEYDOWN && wp == VK_MENU)) {
            Wnd *top = wnd_toplevel(w);

            if (top->menu)
                menu_syskey(top, 0);
            return 0;
        }
        if (msg == WM_SYSKEYDOWN && wp == VK_F1 + 3 && (lp & (1 << 29))) {   /* Alt+F4 */
            PostMessageA(wnd_toplevel(w)->h, WM_SYSCOMMAND, SC_CLOSE, 0);
            return 0;
        }
        return 0;
    case WM_SYSCHAR:
        {
            Wnd *top = wnd_toplevel(w);

            if (top->menu)
                menu_syskey(top, wp);
        }
        return 0;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        {
            int edit = msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX;

            SetTextColor((HDC)wp, GetSysColor(edit ? COLOR_WINDOWTEXT : COLOR_BTNTEXT));
            SetBkColor((HDC)wp, GetSysColor(edit ? COLOR_WINDOW : COLOR_BTNFACE));
            return (LRESULT)(uintptr_t)(edit ? COLOR_WINDOW + 1 : COLOR_BTNFACE + 1);
        }
    case WM_QUERYENDSESSION:
    case WM_QUERYNEWPALETTE:
        return msg == WM_QUERYENDSESSION;
    case WM_GETMINMAXINFO:
        return 0;
    }
    return 0;
}

LRESULT WINAPI CallWindowProcA(WNDPROC proc, HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    return proc ? proc(h, msg, wp, lp) : DefWindowProcA(h, msg, wp, lp);
}

/* ---- Painting ---- */

HDC WINAPI BeginPaint(HWND h, LPPAINTSTRUCT ps)
{
    Wnd *w = wnd_get(h);
    RECT r;
    HDC hdc;

    memset(ps, 0, sizeof *ps);
    if (!w)
        return NULL;
    wnd_nc_paint(w);
    if (w->invalid)
        r = w->inv;
    else
        r = (RECT){0, 0, 0, 0};
    ps->fErase = w->erase;
    w->invalid = 0;
    hdc = dc_for_window(w, 1, &r);
    ps->hdc = hdc;
    ps->rcPaint = r;
    if (w->erase) {
        w->erase = 0;
        ps->fErase = !SendMessageA(h, WM_ERASEBKGND, (WPARAM)hdc, 0);
    }
    return hdc;
}

BOOL WINAPI EndPaint(HWND h, const PAINTSTRUCT *ps)
{
    ReleaseDC(h, ps->hdc);
    return TRUE;
}

/* ---- Set-up ---- */

void wnd_init_desktop(void)
{
    int bx, by, bw, bh;
    Wnd *w;

    wnd_register_builtin("#32769", DefWindowProcA, 0, 0, 1);
    w = calloc(1, sizeof *w);
    w->h = (HWND)port_handle_new(H_WND, w);
    w->cls = wnd_find_class("#32769");
    w->proc = DefWindowProcA;
    w->style = WS_VISIBLE | WS_POPUP;
    w->text = strdup("");
    w->nextra = 8;
    w->extra = calloc(1, 8);
    plat_usable_bounds(&bx, &by, &bw, &bh);
    /* a system caption above each window: count it out of the height */
    w->rc = (RECT){bx, by, bx + bw, by + bh - 28};
    port_desktop = w;
}
