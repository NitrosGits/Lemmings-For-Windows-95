/*
 * Menus: menu objects from the resource tables, a Windows 95 style menu bar
 * drawn at the top of its window, drop-down lists drawn over the window, and
 * the modal loop that runs while a menu is open (as in Win32, the game's own
 * loop waits meanwhile). WM_INITMENU, WM_INITMENUPOPUP, WM_MENUSELECT (the
 * game shows help for the item in its status bar) and WM_COMMAND go to the
 * window.
 */
#include "port_internal.h"
#include "port_res.h"

#define WM_ENTERMENULOOP 0x0211
#define WM_EXITMENULOOP  0x0212
#define BAR_H 19
#define ITEM_H 17
#define SEP_H 8

void msg_handle_event(const SDL_Event *ev);
Wnd *wnd_from_sdl(SDL_WindowID id);

typedef struct PMenuItem {
    char *text;
    UINT id;
    UINT flags;
    HMENU sub;
} PMenuItem;

typedef struct PMenu {
    int n;
    PMenuItem *items;
} PMenu;

static PMenu *menu_get(HMENU h)
{
    return (PMenu *)port_handle_get(h, H_MENU);
}

static HMENU build(const PortResMenuItem *items, int n)
{
    PMenu *m = calloc(1, sizeof *m);
    int i;

    m->n = n;
    m->items = calloc((size_t)(n ? n : 1), sizeof(PMenuItem));
    for (i = 0; i < n; i++) {
        m->items[i].text = SDL_strdup(items[i].text ? items[i].text : "");
        m->items[i].id = (UINT)items[i].id;
        m->items[i].flags = (UINT)items[i].flags;
        if (items[i].flags & MF_POPUP)
            m->items[i].sub = build(items[i].items, items[i].count);
    }
    return (HMENU)port_handle_new(H_MENU, m);
}

HMENU WINAPI LoadMenuA(HINSTANCE inst, LPCSTR name)
{
    const PortResMenu *r;

    (void)inst;
    for (r = port_res_menus; r->items; r++) {
        if (IS_INTRESOURCE(name) ? r->id == (int)(uintptr_t)name : (r->name && !SDL_strcasecmp(r->name, name)))
            return build(r->items, r->count);
    }
    return NULL;
}

BOOL WINAPI DestroyMenu(HMENU h)
{
    PMenu *m = menu_get(h);
    int i;

    if (!m)
        return FALSE;
    for (i = 0; i < m->n; i++) {
        free(m->items[i].text);
        if (m->items[i].sub)
            DestroyMenu(m->items[i].sub);
    }
    free(m->items);
    free(m);
    port_handle_free(h);
    return TRUE;
}

void menu_destroy_bar(Wnd *w)
{
    if (w->menu) {
        DestroyMenu(w->menu);
        w->menu = NULL;
    }
}

int menu_bar_height(Wnd *w)
{
    return w->menu && menu_get(w->menu) ? BAR_H : 0;
}

HMENU WINAPI GetMenu(HWND h)
{
    Wnd *w = wnd_get(h);

    return w && !w->parent ? w->menu : NULL;
}

BOOL WINAPI SetMenu(HWND h, HMENU menu)
{
    Wnd *w = wnd_get(h);
    int before;

    if (!w || w->parent)
        return FALSE;
    before = menu_bar_height(w);
    w->menu = menu;
    if (menu_bar_height(w) != before) {
        /* the client area grows or shrinks: Windows sends WM_SIZE */
        wnd_send_size(w);
        if (!wnd_get(h))
            return TRUE;
        wnd_redraw_all(w);
    }
    wnd_nc_paint(w);
    return TRUE;
}

BOOL WINAPI DrawMenuBar(HWND h)
{
    Wnd *w = wnd_get(h);

    if (w)
        wnd_nc_paint(w);
    return w != NULL;
}

static PMenuItem *find_item(HMENU h, UINT id, UINT flags, HMENU *owner)
{
    PMenu *m = menu_get(h);
    int i;

    if (!m)
        return NULL;
    if (flags & MF_BYPOSITION) {
        if ((int)id >= m->n)
            return NULL;
        if (owner)
            *owner = h;
        return &m->items[id];
    }
    for (i = 0; i < m->n; i++) {
        if (!(m->items[i].flags & MF_POPUP) && m->items[i].id == id) {
            if (owner)
                *owner = h;
            return &m->items[i];
        }
        if (m->items[i].sub) {
            PMenuItem *it = find_item(m->items[i].sub, id, flags, owner);

            if (it)
                return it;
        }
    }
    return NULL;
}

static Wnd *bar_owner(HMENU h)
{
    Wnd *w;

    for (w = port_top; w; w = w->next)
        if (!w->destroyed && w->menu == h)
            return w;
    return NULL;
}

BOOL WINAPI EnableMenuItem(HMENU h, UINT id, UINT flags)
{
    PMenuItem *it = find_item(h, id, flags, NULL);
    UINT old;
    Wnd *w;

    if (!it)
        return (BOOL)-1;
    old = it->flags & (MF_GRAYED | MF_DISABLED);
    it->flags = (it->flags & ~(MF_GRAYED | MF_DISABLED)) | (flags & (MF_GRAYED | MF_DISABLED));
    if ((w = bar_owner(h)) != NULL && (flags & MF_BYPOSITION))
        wnd_nc_paint(w);
    return (BOOL)old;
}

DWORD WINAPI CheckMenuItem(HMENU h, UINT id, UINT flags)
{
    PMenuItem *it = find_item(h, id, flags, NULL);
    DWORD old;

    if (!it)
        return (DWORD)-1;
    old = it->flags & MF_CHECKED;
    it->flags = (it->flags & ~MF_CHECKED) | (flags & MF_CHECKED);
    return old;
}

int menu_item_enabled(HMENU h, UINT id)
{
    PMenuItem *it = find_item(h, id, MF_BYCOMMAND, NULL);

    return !it || !(it->flags & (MF_GRAYED | MF_DISABLED));
}

HMENU WINAPI GetSubMenu(HMENU h, int pos)
{
    PMenu *m = menu_get(h);

    return m && pos >= 0 && pos < m->n ? m->items[pos].sub : NULL;
}

int WINAPI GetMenuItemCount(HMENU h)
{
    PMenu *m = menu_get(h);

    return m ? m->n : -1;
}

UINT WINAPI GetMenuItemID(HMENU h, int pos)
{
    PMenu *m = menu_get(h);

    if (!m || pos < 0 || pos >= m->n)
        return (UINT)-1;
    return m->items[pos].sub ? (UINT)-1 : m->items[pos].id;
}

UINT WINAPI GetMenuState(HMENU h, UINT id, UINT flags)
{
    PMenuItem *it = find_item(h, id, flags, NULL);

    return it ? it->flags : (UINT)-1;
}

int WINAPI GetMenuStringA(HMENU h, UINT id, LPSTR buf, int size, UINT flags)
{
    PMenuItem *it = find_item(h, id, flags, NULL);

    if (!it || size <= 0)
        return 0;
    SDL_strlcpy(buf, it->text, (size_t)size);
    return (int)strlen(buf);
}

/* ---- Drawing ---- */

static PFont *menu_font(void)
{
    return font_get(font_default());
}

static int text_width(const char *s)
{
    PFont *f = menu_font();
    int w = 0;

    for (; *s; s++) {
        if (*s == '&' && s[1] && s[1] != '&')
            continue;
        if (*s == '\t')
            break;
        w += font_char_width(f, (unsigned char)*s);
    }
    return w;
}

/* the bar item rectangles, in framebuffer coordinates */
static int bar_items(Wnd *w, RECT *out, int max)
{
    PMenu *m = menu_get(w->menu);
    int i, x, top, l, t, r, b;

    if (!m)
        return 0;
    wnd_client_offset(w, &l, &t, &r, &b);
    top = t - BAR_H;
    x = l + 1;
    for (i = 0; i < m->n && i < max; i++) {
        int iw = text_width(m->items[i].text) + 12;

        out[i] = (RECT){x, top + 1, x + iw, top + BAR_H - 1};
        x += iw;
    }
    return i;
}

static int g_openItem = -1;     /* the bar item shown open */
static Wnd *g_openWnd;

void menu_bar_draw(Wnd *w)
{
    PMenu *m = menu_get(w->menu);
    RECT items[32];
    HDC hdc;
    PDC *dc;
    int n, i, l, t, r, b, width;
    PFont *f = menu_font();

    if (!m)
        return;
    hdc = dc_for_window(w, 0, NULL);
    dc = dc_get(hdc);
    wnd_client_offset(w, &l, &t, &r, &b);
    width = w->rc.right - w->rc.left;
    gdi_fill(dc, &(RECT){l, t - BAR_H, width - r, t}, sys_xrgb(COLOR_MENU));
    n = bar_items(w, items, 32);
    for (i = 0; i < n; i++) {
        PMenuItem *it = &m->items[i];
        int open = g_openWnd == w && g_openItem == i;
        int gray = (it->flags & (MF_GRAYED | MF_DISABLED)) != 0;
        int tx = items[i].left + 6 + open, ty = items[i].top + (items[i].bottom - items[i].top - f->height) / 2 + open;

        if (open)
            gdi_frame3d(dc, &items[i], sys_xrgb(COLOR_BTNSHADOW), sys_xrgb(COLOR_BTNHIGHLIGHT));
        if (gray) {
            font_draw_prefixed(dc, f, tx + 1, ty + 1, it->text, sys_xrgb(COLOR_BTNHIGHLIGHT), 1);
            font_draw_prefixed(dc, f, tx, ty, it->text, sys_xrgb(COLOR_GRAYTEXT), 1);
        } else
            font_draw_prefixed(dc, f, tx, ty, it->text, sys_xrgb(COLOR_MENUTEXT), 1);
    }
    ReleaseDC(w->h, hdc);
}

int menu_bar_hit(Wnd *top, int fx, int fy)
{
    RECT items[32];
    int n = bar_items(top, items, 32), i;

    for (i = 0; i < n; i++)
        if (fx >= items[i].left && fx < items[i].right && fy >= items[i].top - 1 && fy <= items[i].bottom)
            return i;
    return -1;
}

int menu_bar_click(Wnd *w, int fx, int fy)
{
    int i = menu_bar_hit(w, fx, fy);

    return i >= 0;
}

/* ---- The drop-down list ---- */

typedef struct Popup {
    HMENU h;
    PMenu *m;
    int x, y, w, hgt;           /* framebuffer coordinates */
    int sel;
    Surface s;
} Popup;

static int popup_item_at(Popup *p, int fx, int fy)
{
    int i, y = p->y + 3;

    if (fx < p->x || fx >= p->x + p->w)
        return -1;
    for (i = 0; i < p->m->n; i++) {
        int h = (p->m->items[i].flags & MF_SEPARATOR) ? SEP_H : ITEM_H;

        if (fy >= y && fy < y + h)
            return (p->m->items[i].flags & MF_SEPARATOR) ? -1 : i;
        y += h;
    }
    return -1;
}

static void popup_layout(Popup *p, int x, int y)
{
    int i, w = 0, h = 6;

    for (i = 0; i < p->m->n; i++) {
        int tw = text_width(p->m->items[i].text);

        if (tw > w)
            w = tw;
        h += (p->m->items[i].flags & MF_SEPARATOR) ? SEP_H : ITEM_H;
    }
    p->x = x;
    p->y = y;
    p->w = w + 40;
    p->hgt = h;
    free(p->s.bits);
    memset(&p->s, 0, sizeof p->s);
    p->s.w = p->w;
    p->s.h = p->hgt;
    p->s.bpp = 32;
    p->s.pitch = p->w * 4;
    p->s.bits = calloc((size_t)p->w * p->hgt, 4);
}

static void popup_draw(Popup *p)
{
    PDC dc;
    PFont *f = menu_font();
    int i, y = 3;

    memset(&dc, 0, sizeof dc);
    dc.kind = DCK_MEMORY;
    dc.s = &p->s;
    rgn_set(&dc.clip, &(RECT){0, 0, p->w, p->hgt});
    gdi_fill(&dc, &(RECT){0, 0, p->w, p->hgt}, sys_xrgb(COLOR_MENU));
    gdi_frame3d(&dc, &(RECT){0, 0, p->w, p->hgt}, sys_xrgb(COLOR_3DLIGHT), sys_xrgb(COLOR_3DDKSHADOW));
    gdi_frame3d(&dc, &(RECT){1, 1, p->w - 1, p->hgt - 1}, sys_xrgb(COLOR_BTNHIGHLIGHT), sys_xrgb(COLOR_BTNSHADOW));
    for (i = 0; i < p->m->n; i++) {
        PMenuItem *it = &p->m->items[i];

        if (it->flags & MF_SEPARATOR) {
            gdi_hline(&dc, 4, p->w - 4, y + 3, sys_xrgb(COLOR_BTNSHADOW));
            gdi_hline(&dc, 4, p->w - 4, y + 4, sys_xrgb(COLOR_BTNHIGHLIGHT));
            y += SEP_H;
            continue;
        }
        {
            int gray = (it->flags & (MF_GRAYED | MF_DISABLED)) != 0;
            int ty = y + (ITEM_H - f->height) / 2;

            if (i == p->sel) {
                gdi_fill(&dc, &(RECT){3, y, p->w - 3, y + ITEM_H}, sys_xrgb(COLOR_HIGHLIGHT));
                font_draw_prefixed(&dc, f, 20, ty, it->text,
                                   gray ? sys_xrgb(COLOR_GRAYTEXT) : sys_xrgb(COLOR_HIGHLIGHTTEXT), 1);
            } else if (gray) {
                font_draw_prefixed(&dc, f, 21, ty + 1, it->text, sys_xrgb(COLOR_BTNHIGHLIGHT), 1);
                font_draw_prefixed(&dc, f, 20, ty, it->text, sys_xrgb(COLOR_GRAYTEXT), 1);
            } else
                font_draw_prefixed(&dc, f, 20, ty, it->text, sys_xrgb(COLOR_MENUTEXT), 1);
            if (it->flags & MF_CHECKED) {
                int cx = 8, cy = y + ITEM_H / 2;
                uint32_t c = i == p->sel ? sys_xrgb(COLOR_HIGHLIGHTTEXT) : 0;

                gdi_vline(&dc, cx, cy - 1, cy + 2, c);
                gdi_vline(&dc, cx + 1, cy, cy + 3, c);
                gdi_vline(&dc, cx + 2, cy - 1, cy + 2, c);
                gdi_vline(&dc, cx + 3, cy - 2, cy + 1, c);
                gdi_vline(&dc, cx + 4, cy - 3, cy, c);
            }
        }
        y += ITEM_H;
    }
}

static void menu_select(Wnd *top, HMENU menu, UINT idOrPos, UINT flags)
{
    SendMessageA(top->h, WM_MENUSELECT, MAKEWPARAM(idOrPos, flags), (LPARAM)menu);
}

static void popup_set_sel(Wnd *top, Popup *p, int sel)
{
    if (sel == p->sel)
        return;
    p->sel = sel;
    if (sel >= 0) {
        PMenuItem *it = &p->m->items[sel];

        menu_select(top, p->h, it->id, MF_HILITE | MF_MOUSESELECT | (it->flags & (MF_GRAYED | MF_DISABLED)));
    }
    popup_draw(p);
    top->dirty = 1;
}

static void open_item(Wnd *top, Popup *p, int item)
{
    PMenu *bar = menu_get(top->menu);
    RECT items[32];

    g_openWnd = top;
    g_openItem = item;
    menu_bar_draw(top);
    top->dirty = 1;
    top->overlay = NULL;
    p->m = NULL;
    p->sel = -1;
    if (item < 0 || !bar || item >= bar->n)
        return;
    menu_select(top, top->menu, (UINT)item, MF_POPUP | MF_HILITE | MF_MOUSESELECT);
    if (!bar->items[item].sub)
        return;
    SendMessageA(top->h, WM_INITMENUPOPUP, (WPARAM)bar->items[item].sub, item);
    bar_items(top, items, 32);
    p->h = bar->items[item].sub;
    p->m = menu_get(p->h);
    if (!p->m)
        return;
    popup_layout(p, items[item].left, items[item].bottom + 1);
    if (p->x + p->w > top->fb.w)
        p->x = top->fb.w - p->w;
    if (p->x < 0)
        p->x = 0;
    popup_draw(p);
    top->overlay = &p->s;
    top->overlayX = p->x;
    top->overlayY = p->y;
}

static int mnemonic(const char *s)
{
    for (; *s; s++)
        if (*s == '&' && s[1] && s[1] != '&')
            return SDL_tolower((unsigned char)s[1]);
    return 0;
}

static int next_enabled(Popup *p, int from, int dir)
{
    int i, k;

    if (!p->m || !p->m->n)
        return -1;
    for (k = 1; k <= p->m->n; k++) {
        i = ((from < 0 && dir < 0 ? p->m->n : from) + dir * k + p->m->n * 4) % p->m->n;
        if (!(p->m->items[i].flags & MF_SEPARATOR))
            return i;
    }
    return -1;
}

/* Runs the menu of top's bar, item open; returns the command chosen or 0 */
int menu_track_bar(Wnd *top, int item, int byKey)
{
    PMenu *bar = menu_get(top->menu);
    Popup p;
    SDL_Event ev;
    UINT cmd = 0;
    int done = 0, downOnBar = !byKey;

    if (!bar || item < 0 || (top->style & WS_DISABLED))
        return 0;
    memset(&p, 0, sizeof p);
    p.sel = -1;
    port_modal_depth++;
    SendMessageA(top->h, WM_ENTERMENULOOP, 0, 0);
    SendMessageA(top->h, WM_INITMENU, (WPARAM)top->menu, 0);
    open_item(top, &p, item);
    if (byKey && p.m)
        popup_set_sel(top, &p, next_enabled(&p, -1, 1));
    while (!done) {
        msg_present();
        if (port_harness)
            harness_poll();
        if (!SDL_WaitEventTimeout(&ev, 50))
            continue;
        switch (ev.type) {
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                Wnd *w = wnd_from_sdl(ev.type == SDL_EVENT_MOUSE_MOTION ? ev.motion.windowID : ev.button.windowID);
                int fx = (int)(ev.type == SDL_EVENT_MOUSE_MOTION ? ev.motion.x : ev.button.x);
                int fy = (int)(ev.type == SDL_EVENT_MOUSE_MOTION ? ev.motion.y : ev.button.y);
                int bi = w == top ? menu_bar_hit(top, fx, fy) : -1;
                int pi = w == top && p.m ? popup_item_at(&p, fx, fy) : -1;
                int inPopup = w == top && p.m && fx >= p.x && fx < p.x + p.w && fy >= p.y && fy < p.y + p.hgt;

                if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                    if (bi >= 0 && bi != g_openItem)
                        open_item(top, &p, bi);
                    else if (inPopup)
                        popup_set_sel(top, &p, pi);
                    break;
                }
                if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    if (bi >= 0) {
                        if (bi == g_openItem && !downOnBar)
                            done = 1;
                        else
                            open_item(top, &p, bi);
                        downOnBar = 1;
                    } else if (!inPopup)
                        done = 1;
                    break;
                }
                /* button up: on an item chooses it */
                if (inPopup && pi >= 0) {
                    PMenuItem *it = &p.m->items[pi];

                    if (!(it->flags & (MF_GRAYED | MF_DISABLED)) && !it->sub) {
                        cmd = it->id;
                        done = 1;
                    }
                } else if (bi < 0 && !inPopup)
                    done = 1;
                downOnBar = 0;
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            switch (ev.key.key) {
            case SDLK_ESCAPE:
                done = 1;
                break;
            case SDLK_LEFT:
                open_item(top, &p, (g_openItem + bar->n - 1) % bar->n);
                if (p.m)
                    popup_set_sel(top, &p, next_enabled(&p, -1, 1));
                break;
            case SDLK_RIGHT:
                open_item(top, &p, (g_openItem + 1) % bar->n);
                if (p.m)
                    popup_set_sel(top, &p, next_enabled(&p, -1, 1));
                break;
            case SDLK_UP:
                popup_set_sel(top, &p, next_enabled(&p, p.sel, -1));
                break;
            case SDLK_DOWN:
                popup_set_sel(top, &p, next_enabled(&p, p.sel, 1));
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (p.m && p.sel >= 0 && !(p.m->items[p.sel].flags & (MF_GRAYED | MF_DISABLED))) {
                    cmd = p.m->items[p.sel].id;
                    done = 1;
                }
                break;
            default:
                if (ev.key.key < 128 && p.m) {
                    int i;

                    for (i = 0; i < p.m->n; i++)
                        if (mnemonic(p.m->items[i].text) == (int)ev.key.key &&
                            !(p.m->items[i].flags & (MF_GRAYED | MF_DISABLED | MF_SEPARATOR))) {
                            cmd = p.m->items[i].id;
                            done = 1;
                            break;
                        }
                }
                break;
            }
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            done = 1;
            break;
        case SDL_EVENT_QUIT:
            done = 1;
            SDL_PushEvent(&ev);
            break;
        default:
            if (ev.type >= SDL_EVENT_WINDOW_FIRST && ev.type <= SDL_EVENT_WINDOW_LAST)
                msg_handle_event(&ev);
            break;
        }
    }
    top->overlay = NULL;
    free(p.s.bits);
    g_openItem = -1;
    g_openWnd = NULL;
    if (!top->destroyed) {
        menu_bar_draw(top);
        top->dirty = 1;
        menu_select(top, NULL, 0, 0xffff);
        SendMessageA(top->h, WM_EXITMENULOOP, 0, 0);
        if (cmd)
            PostMessageA(top->h, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
    }
    port_modal_depth--;
    return (int)cmd;
}

/* Alt+letter opens the menu with that mnemonic; Alt or F10 alone the first */
int menu_syskey(Wnd *top, WPARAM ch)
{
    PMenu *bar = top ? menu_get(top->menu) : NULL;
    int i;

    if (!bar)
        return 0;
    if (!ch) {
        menu_track_bar(top, 0, 1);
        return 1;
    }
    for (i = 0; i < bar->n; i++)
        if (mnemonic(bar->items[i].text) == SDL_tolower((int)ch)) {
            menu_track_bar(top, i, 1);
            return 1;
        }
    return 0;
}

int menu_key(Wnd *w, int vk, int ch)
{
    (void)vk;
    return menu_syskey(w, (WPARAM)ch);
}

void menu_init(void)
{
}
