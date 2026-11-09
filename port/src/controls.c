/*
 * The standard controls the game's dialogs use, in Windows 95 style:
 * Button (push, default, check box, radio button, group box), Static (text,
 * icon, rectangles and frames), Edit (one line) and ListBox (plain and
 * owner-drawn), and scroll bars for any window with WS_HSCROLL/WS_VSCROLL.
 * Controls ask their parent for colours with WM_CTLCOLOR* and tell it what
 * happened with WM_COMMAND, as in Win32.
 */
#include "port_internal.h"

void dlg_init(void);

#define WM_MOUSEWHEEL 0x020A

static PFont *ctl_font(Wnd *w)
{
    return font_get(w->font ? w->font : font_default());
}

static void notify(Wnd *w, int code)
{
    if (w->parent)
        SendMessageA(w->parent->h, WM_COMMAND, MAKEWPARAM(w->id, code), (LPARAM)w->h);
}

/* the parent's brush and colours for this control (WM_CTLCOLOR*) */
static HBRUSH get_brush(Wnd *w, UINT msg, HDC hdc)
{
    LRESULT r = 0;

    if (w->parent)
        r = SendMessageA(w->parent->h, msg, (WPARAM)hdc, (LPARAM)w->h);
    else
        r = DefWindowProcA(w->h, msg, (WPARAM)hdc, (LPARAM)w->h);
    if (!r)
        r = DefWindowProcA(w->h, msg, (WPARAM)hdc, (LPARAM)w->h);
    return (HBRUSH)(uintptr_t)(uint32_t)r;
}

uint32_t ctl_color(Wnd *w, UINT msg, HDC dc, int *brushOut)
{
    HBRUSH b = get_brush(w, msg, dc);
    PBrush *pb = brush_get(b);

    if (brushOut)
        *brushOut = (int)(uintptr_t)b;
    return pb ? color_xrgb(pb->color) : sys_xrgb(COLOR_BTNFACE);
}

/* ---- Shared drawing ---- */

void draw_button_face(PDC *dc, const RECT *r, int pressed, int isDefault)
{
    RECT b = *r;

    if (isDefault) {
        gdi_frame3d(dc, &b, 0, 0);
        InflateRect(&b, -1, -1);
    }
    gdi_fill(dc, &b, sys_xrgb(COLOR_BTNFACE));
    if (pressed) {
        gdi_frame3d(dc, &b, sys_xrgb(COLOR_BTNSHADOW), sys_xrgb(COLOR_BTNSHADOW));
    } else {
        gdi_frame3d(dc, &b, sys_xrgb(COLOR_BTNHIGHLIGHT), sys_xrgb(COLOR_3DDKSHADOW));
        InflateRect(&b, -1, -1);
        gdi_frame3d(dc, &b, sys_xrgb(COLOR_3DLIGHT), sys_xrgb(COLOR_BTNSHADOW));
    }
}

static void draw_arrow(PDC *dc, const RECT *r, int dir, int enabled, int pressed)
{
    int cx = (r->left + r->right) / 2 + pressed, cy = (r->top + r->bottom) / 2 + pressed, i;
    uint32_t c = enabled ? 0 : sys_xrgb(COLOR_GRAYTEXT);

    for (i = 0; i < 4; i++) {
        switch (dir) {
        case 0:     /* up */
            gdi_hline(dc, cx - i, cx + i + 1, cy - 2 + i, c);
            break;
        case 1:     /* down */
            gdi_hline(dc, cx - i, cx + i + 1, cy + 1 - i, c);
            break;
        case 2:     /* left */
            gdi_vline(dc, cx - 2 + i, cy - i, cy + i + 1, c);
            break;
        default:    /* right */
            gdi_vline(dc, cx + 1 - i, cy - i, cy + i + 1, c);
            break;
        }
    }
}

typedef struct SbParts {
    RECT a1, a2, track, thumb;
} SbParts;

static void sb_parts(const RECT *r, int vert, int min, int max, int pos, SbParts *p)
{
    int len = vert ? r->bottom - r->top : r->right - r->left;
    int th = vert ? r->right - r->left : r->bottom - r->top;
    int arrow = len < 2 * th ? len / 2 : th;
    int tlen = len - 2 * arrow, tpos;

    if (vert) {
        p->a1 = (RECT){r->left, r->top, r->right, r->top + arrow};
        p->a2 = (RECT){r->left, r->bottom - arrow, r->right, r->bottom};
        p->track = (RECT){r->left, r->top + arrow, r->right, r->bottom - arrow};
    } else {
        p->a1 = (RECT){r->left, r->top, r->left + arrow, r->bottom};
        p->a2 = (RECT){r->right - arrow, r->top, r->right, r->bottom};
        p->track = (RECT){r->left + arrow, r->top, r->right - arrow, r->bottom};
    }
    if (tlen < th || max <= min) {
        p->thumb = (RECT){0, 0, 0, 0};
        return;
    }
    tpos = (int)((long long)(pos - min) * (tlen - th) / (max - min));
    if (vert)
        p->thumb = (RECT){r->left, p->track.top + tpos, r->right, p->track.top + tpos + th};
    else
        p->thumb = (RECT){p->track.left + tpos, r->top, p->track.left + tpos + th, r->bottom};
}

void draw_scrollbar(PDC *dc, const RECT *r, int vert, int min, int max, int pos, int page, int enabled)
{
    SbParts p;
    int x, y;
    Region g;
    RECT tr;

    (void)page;
    sb_parts(r, vert, min, max, pos, &p);
    draw_button_face(dc, &p.a1, 0, 0);
    draw_button_face(dc, &p.a2, 0, 0);
    draw_arrow(dc, &p.a1, vert ? 0 : 2, enabled && max > min, 0);
    draw_arrow(dc, &p.a2, vert ? 1 : 3, enabled && max > min, 0);
    /* the track: Windows 95's dither of white and grey */
    tr = p.track;
    g.n = 0;
    for (y = tr.top; y < tr.bottom; y++)
        for (x = tr.left; x < tr.right; x++)
            gdi_pixel(dc, x, y, (x + y) & 1 ? sys_xrgb(COLOR_BTNHIGHLIGHT) : sys_xrgb(COLOR_SCROLLBAR));
    (void)g;
    if (!rect_empty(&p.thumb))
        draw_button_face(dc, &p.thumb, 0, 0);
}

/* the scroll bar loop: arrows and the track repeat while held, the thumb follows the mouse */
void scroll_track(Wnd *w, int bar, int fx, int fy)
{
    UINT msg = bar == SB_VERT ? WM_VSCROLL : WM_HSCROLL;
    RECT wr, r;
    SbParts p;
    int part, code = -1, lastPos, done = 0, vert = bar == SB_VERT;
    DWORD next;
    SDL_Event ev;
    Wnd *top = wnd_toplevel(w);

    if (!wnd_scrollbar_rect(w, bar, &r))
        return;
    wnd_window_fb(w, &wr);
    OffsetRect(&r, wr.left, wr.top);
    sb_parts(&r, vert, w->sb[bar].min, w->sb[bar].max, w->sb[bar].pos, &p);
#define IN(rc) (fx >= (rc).left && fx < (rc).right && fy >= (rc).top && fy < (rc).bottom)
    if (IN(p.thumb))
        part = 4;
    else if (IN(p.a1))
        part = 0;
    else if (IN(p.a2))
        part = 1;
    else if (IN(p.track))
        part = (vert ? fy < p.thumb.top : fx < p.thumb.left) ? 2 : 3;
    else
        return;
    if (w->sb[bar].max <= w->sb[bar].min)
        return;
    lastPos = w->sb[bar].pos;
    if (part != 4) {
        code = part == 0 ? SB_LINEUP : part == 1 ? SB_LINEDOWN : part == 2 ? SB_PAGEUP : SB_PAGEDOWN;
        SendMessageA(w->h, msg, MAKEWPARAM(code, 0), 0);
    }
    next = port_ticks() + 350;
    port_modal_depth++;
    while (!done && wnd_get(w->h)) {
        msg_present();
        if (SDL_WaitEventTimeout(&ev, 20)) {
            if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                fx = (int)ev.motion.x;
                fy = (int)ev.motion.y;
                if (part == 4) {
                    int tlen = vert ? p.track.bottom - p.track.top - (p.thumb.bottom - p.thumb.top)
                                    : p.track.right - p.track.left - (p.thumb.right - p.thumb.left);
                    int off = vert ? fy - p.track.top - (p.thumb.bottom - p.thumb.top) / 2
                                   : fx - p.track.left - (p.thumb.right - p.thumb.left) / 2;
                    int pos;

                    if (tlen <= 0)
                        continue;
                    if (off < 0)
                        off = 0;
                    if (off > tlen)
                        off = tlen;
                    pos = w->sb[bar].min + (int)((long long)off * (w->sb[bar].max - w->sb[bar].min) / tlen);
                    if (pos != lastPos) {
                        lastPos = pos;
                        SendMessageA(w->h, msg, MAKEWPARAM(SB_THUMBTRACK, pos), 0);
                    }
                }
            } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                done = 1;
            } else if (ev.type >= SDL_EVENT_WINDOW_FIRST && ev.type <= SDL_EVENT_WINDOW_LAST) {
                void msg_handle_event(const SDL_Event *e);

                msg_handle_event(&ev);
            }
        }
        if (part != 4 && !done && (int)(port_ticks() - next) >= 0) {
            sb_parts(&r, vert, w->sb[bar].min, w->sb[bar].max, w->sb[bar].pos, &p);
            if (part < 2 ? IN(part == 0 ? p.a1 : p.a2)
                         : IN(p.track) && (part == 2 ? (vert ? fy < p.thumb.top : fx < p.thumb.left)
                                                     : (vert ? fy >= p.thumb.bottom : fx >= p.thumb.right)))
                SendMessageA(w->h, msg, MAKEWPARAM(code, 0), 0);
            next = port_ticks() + 50;
        }
    }
#undef IN
    port_modal_depth--;
    if (!wnd_get(w->h))
        return;
    if (part == 4)
        SendMessageA(w->h, msg, MAKEWPARAM(SB_THUMBPOSITION, lastPos), 0);
    SendMessageA(w->h, msg, MAKEWPARAM(SB_ENDSCROLL, 0), 0);
    (void)top;
}

/* ---- Button ---- */

typedef struct Btn {
    int check;
    int pressed;
    int tracking;
} Btn;

static Btn *btn(Wnd *w)
{
    if (!w->ctl)
        w->ctl = calloc(1, sizeof(Btn));
    return (Btn *)w->ctl;
}

static void ctl_free(Wnd *w)
{
    free(w->ctl);
    w->ctl = NULL;
}

static int btn_type(Wnd *w)
{
    return (int)(w->style & 0xf);
}

static void radio_click(Wnd *w)
{
    Wnd *first = w, *c;

    /* the group: back to the control with WS_GROUP, on to the next one */
    while (first->prev && !(first->style & WS_GROUP))
        first = first->prev;
    for (c = first; c; c = c->next) {
        if (c != first && (c->style & WS_GROUP))
            break;
        if (c->cls == w->cls && (btn_type(c) == BS_AUTORADIOBUTTON || btn_type(c) == BS_RADIOBUTTON))
            SendMessageA(c->h, BM_SETCHECK, c == w, 0);
    }
}

static void btn_click(Wnd *w)
{
    Btn *b = btn(w);

    switch (btn_type(w)) {
    case BS_AUTOCHECKBOX:
        b->check = !b->check;
        wnd_invalidate(w, NULL, 1);
        break;
    case BS_AUTO3STATE:
        b->check = (b->check + 1) % 3;
        wnd_invalidate(w, NULL, 1);
        break;
    case BS_AUTORADIOBUTTON:
        radio_click(w);
        break;
    }
    notify(w, BN_CLICKED);
}

static void btn_paint(Wnd *w)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(w->h, &ps);
    PDC *dc = dc_get(hdc);
    Btn *b = btn(w);
    RECT r;
    PFont *f = ctl_font(w);
    int type = btn_type(w), enabled = !(w->style & WS_DISABLED), focus = port_focus == w;
    uint32_t bg, text = sys_xrgb(enabled ? COLOR_BTNTEXT : COLOR_GRAYTEXT);
    const char *t = w->text;
    int tw = font_text_width(f, t, -1);

    GetClientRect(w->h, &r);
    if (type == BS_OWNERDRAW) {
        DRAWITEMSTRUCT dis;

        memset(&dis, 0, sizeof dis);
        dis.CtlType = ODT_BUTTON;
        dis.CtlID = (UINT)w->id;
        dis.itemAction = ODA_DRAWENTIRE;
        dis.itemState = (b->pressed ? ODS_SELECTED : 0) | (focus ? ODS_FOCUS : 0) | (enabled ? 0 : ODS_DISABLED);
        dis.hwndItem = w->h;
        dis.hDC = hdc;
        dis.rcItem = r;
        if (w->parent)
            SendMessageA(w->parent->h, WM_DRAWITEM, (WPARAM)w->id, (LPARAM)&dis);
        EndPaint(w->h, &ps);
        return;
    }
    if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON || type == BS_USERBUTTON) {
        int def = type == BS_DEFPUSHBUTTON || (focus && type == BS_PUSHBUTTON);
        int tx, ty;

        bg = ctl_color(w, WM_CTLCOLORBTN, hdc, NULL);
        (void)bg;
        draw_button_face(dc, &r, b->pressed, def);
        tx = (r.right - tw) / 2 + b->pressed;
        ty = (r.bottom - f->height) / 2 + b->pressed;
        if (!enabled)
            font_draw_prefixed(dc, f, tx + 1, ty + 1, t, sys_xrgb(COLOR_BTNHIGHLIGHT), 1);
        font_draw_prefixed(dc, f, tx, ty, t, text, 1);
        if (focus) {
            RECT fr = r;

            InflateRect(&fr, def ? -4 : -3, def ? -4 : -3);
            DrawFocusRect(hdc, &fr);
        }
        EndPaint(w->h, &ps);
        return;
    }
    bg = ctl_color(w, WM_CTLCOLORSTATIC, hdc, NULL);
    if (type == BS_GROUPBOX) {
        int ty = f->height / 2;

        /* a group box draws only its frame and caption: the controls inside show through */
        gdi_frame3d(dc, &(RECT){0, ty, r.right - 1, r.bottom - 1}, sys_xrgb(COLOR_BTNSHADOW),
                    sys_xrgb(COLOR_BTNSHADOW));
        gdi_frame3d(dc, &(RECT){1, ty + 1, r.right, r.bottom}, sys_xrgb(COLOR_BTNHIGHLIGHT),
                    sys_xrgb(COLOR_BTNHIGHLIGHT));
        if (*t) {
            gdi_fill(dc, &(RECT){6, 0, 6 + tw + 4, f->height}, bg);
            font_draw_prefixed(dc, f, 8, 0, t, text, 1);
        }
        EndPaint(w->h, &ps);
        return;
    }
    /* check boxes and radio buttons */
    {
        int bs = 13, by = (r.bottom - bs) / 2, tx = bs + 4, ty = (r.bottom - f->height) / 2;
        int radio = type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
        uint32_t face = b->pressed || !enabled ? sys_xrgb(COLOR_BTNFACE) : sys_xrgb(COLOR_WINDOW);

        gdi_fill(dc, &r, bg);
        if (radio) {
            static const char *ring[12] = {
                "....aaaa....", "..aabbbbaa..", ".abb....bba.", ".ab......cd.", "ab........cd",
                "ab........cd", "ab........cd", "ab........cd", ".ab......cd.", ".acc....ccd.", "..ddcccdd...",
                "....dddd....",
            };
            int x, y;

            for (y = 0; y < 12; y++)
                for (x = 0; x < 12; x++) {
                    char c = ring[y][x];
                    uint32_t col = c == 'a' ? sys_xrgb(COLOR_BTNSHADOW) : c == 'b' ? 0
                                 : c == 'c' ? sys_xrgb(COLOR_3DLIGHT) : c == 'd' ? sys_xrgb(COLOR_BTNHIGHLIGHT) : face;

                    if (c != '.' || (x > 1 && x < 10 && y > 1 && y < 10 && (x - 5.5) * (x - 5.5) + (y - 5.5) * (y - 5.5) < 20))
                        gdi_pixel(dc, x, by + y, col);
                }
            if (b->check)
                for (y = 4; y < 8; y++)
                    gdi_hline(dc, y == 4 || y == 7 ? 5 : 4, y == 4 || y == 7 ? 7 : 8, by + y, text);
        } else {
            RECT box = {0, by, bs, by + bs};

            gdi_frame3d(dc, &box, sys_xrgb(COLOR_BTNSHADOW), sys_xrgb(COLOR_BTNHIGHLIGHT));
            InflateRect(&box, -1, -1);
            gdi_frame3d(dc, &box, 0, sys_xrgb(COLOR_3DLIGHT));
            InflateRect(&box, -1, -1);
            gdi_fill(dc, &box, face);
            if (b->check) {
                int i;

                for (i = 0; i < 7; i++) {
                    int x = 3 + i, y = by + (i < 3 ? 5 + i : 9 - (i - 2));

                    gdi_vline(dc, x, y, y + 3, b->check == 2 ? sys_xrgb(COLOR_GRAYTEXT) : text);
                }
            }
        }
        if (!enabled)
            font_draw_prefixed(dc, f, tx + 1, ty + 1, t, sys_xrgb(COLOR_BTNHIGHLIGHT), 1);
        font_draw_prefixed(dc, f, tx, ty, t, text, 1);
        if (focus && *t) {
            RECT fr = {tx - 1, ty - 1, tx + tw + 1, ty + f->height + 1};

            DrawFocusRect(hdc, &fr);
        }
    }
    EndPaint(w->h, &ps);
}

static LRESULT CALLBACK ButtonProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    Wnd *w = wnd_get(h);
    Btn *b;
    int type;

    if (!w)
        return 0;
    b = btn(w);
    w->ctlFree = ctl_free;
    type = btn_type(w);
    switch (msg) {
    case WM_PAINT:
        btn_paint(w);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        if (type == BS_GROUPBOX)
            return 0;
        b->tracking = 1;
        b->pressed = 1;
        SetCapture(h);
        if (port_focus != w)
            SetFocus(h);
        wnd_invalidate(w, NULL, 1);
        return 0;
    case WM_MOUSEMOVE:
        if (b->tracking) {
            RECT r;
            POINT pt = {(short)LOWORD(lp), (short)HIWORD(lp)};
            int in;

            GetClientRect(h, &r);
            in = PtInRect(&r, pt);
            if (in != b->pressed) {
                b->pressed = in;
                wnd_invalidate(w, NULL, 1);
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if (b->tracking) {
            int click = b->pressed;

            b->tracking = 0;
            b->pressed = 0;
            ReleaseCapture();
            wnd_invalidate(w, NULL, 1);
            if (click)
                btn_click(w);
        }
        return 0;
    case WM_CAPTURECHANGED:
        if (b->tracking && (HWND)lp != h) {
            b->tracking = b->pressed = 0;
            wnd_invalidate(w, NULL, 1);
        }
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_SPACE && !b->pressed) {
            b->pressed = 1;
            wnd_invalidate(w, NULL, 1);
        }
        return 0;
    case WM_KEYUP:
        if (wp == VK_SPACE && b->pressed) {
            b->pressed = 0;
            wnd_invalidate(w, NULL, 1);
            btn_click(w);
        }
        return 0;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_ENABLE:
        wnd_invalidate(w, NULL, 1);
        return 0;
    case WM_NCHITTEST:
        if (type == BS_GROUPBOX)
            return HTTRANSPARENT;
        break;
    case WM_GETDLGCODE:
        if (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON)
            return DLGC_BUTTON | DLGC_RADIOBUTTON;
        if (type == BS_DEFPUSHBUTTON)
            return DLGC_BUTTON | DLGC_DEFPUSHBUTTON;
        if (type == BS_PUSHBUTTON)
            return DLGC_BUTTON | DLGC_UNDEFPUSHBUTTON;
        return type == BS_GROUPBOX ? DLGC_STATIC : DLGC_BUTTON;
    case BM_GETCHECK:
        return b->check;
    case BM_SETCHECK:
        if (b->check != (int)wp) {
            b->check = (int)wp;
            wnd_invalidate(w, NULL, 1);
        }
        return 0;
    case BM_GETSTATE:
        return b->check | (b->pressed ? 4 : 0) | (port_focus == w ? 8 : 0);
    case BM_SETSTATE:
        b->pressed = wp != 0;
        wnd_invalidate(w, NULL, 1);
        return 0;
    case BM_SETSTYLE:
        w->style = (w->style & ~0xffffu) | (DWORD)(wp & 0xffff);
        if (LOWORD(lp))
            wnd_invalidate(w, NULL, 1);
        return 0;
    case BM_CLICK:
        if (!(w->style & WS_DISABLED))
            btn_click(w);
        return 0;
    case WM_SETTEXT:
        DefWindowProcA(h, msg, wp, lp);
        wnd_invalidate(w, NULL, 1);
        return TRUE;
    }
    return DefWindowProcA(h, msg, wp, lp);
}

/* ---- Static ---- */

typedef struct TextCtx {
    PDC *dc;
    PFont *f;
    int y, width, align, prefix;
    uint32_t color;
} TextCtx;

static int static_line(void *ctx, const char *s, int n)
{
    TextCtx *c = ctx;
    char line[512];
    int w, x;

    if (n >= (int)sizeof line)
        n = sizeof line - 1;
    memcpy(line, s, (size_t)n);
    line[n] = 0;
    w = font_text_width(c->f, line, -1);
    x = c->align == SS_CENTER ? (c->width - w) / 2 : c->align == SS_RIGHT ? c->width - w : 0;
    font_draw_prefixed(c->dc, c->f, x, c->y, line, c->color, c->prefix);
    c->y += c->f->height;
    return 0;
}

static LRESULT CALLBACK StaticProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    Wnd *w = wnd_get(h);
    int type;

    if (!w)
        return 0;
    type = (int)(w->style & 0x1f);
    switch (msg) {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(h, &ps);
            PDC *dc = dc_get(hdc);
            RECT r;

            GetClientRect(h, &r);
            switch (type) {
            case SS_ICON:
                {
                    HICON icon = LoadIconA(GetModuleHandleA(NULL), w->text);

                    icon_draw(dc, icon, 0, 0);
                }
                break;
            case SS_BLACKRECT:
                gdi_fill(dc, &r, sys_xrgb(COLOR_WINDOWFRAME));
                break;
            case SS_GRAYRECT:
                gdi_fill(dc, &r, sys_xrgb(COLOR_BTNSHADOW));
                break;
            case SS_WHITERECT:
                gdi_fill(dc, &r, sys_xrgb(COLOR_WINDOW));
                break;
            case SS_BLACKFRAME:
                gdi_frame3d(dc, &r, sys_xrgb(COLOR_WINDOWFRAME), sys_xrgb(COLOR_WINDOWFRAME));
                break;
            case SS_GRAYFRAME:
                gdi_frame3d(dc, &r, sys_xrgb(COLOR_BTNSHADOW), sys_xrgb(COLOR_BTNSHADOW));
                break;
            case SS_WHITEFRAME:
                gdi_frame3d(dc, &r, sys_xrgb(COLOR_WINDOW), sys_xrgb(COLOR_WINDOW));
                break;
            default:
                {
                    TextCtx c;
                    uint32_t bg = ctl_color(w, WM_CTLCOLORSTATIC, hdc, NULL);

                    gdi_fill(dc, &r, bg);
                    c.dc = dc;
                    c.f = ctl_font(w);
                    c.y = 0;
                    c.width = r.right;
                    c.align = type == SS_CENTER || type == SS_RIGHT ? type : SS_LEFT;
                    c.prefix = !(w->style & SS_NOPREFIX);
                    c.color = (w->style & WS_DISABLED) ? sys_xrgb(COLOR_GRAYTEXT) : color_xrgb(dc->text);
                    if (type == SS_LEFTNOWORDWRAP || type == SS_SIMPLE)
                        static_line(&c, w->text, (int)strlen(w->text));
                    else
                        font_wrap(c.f, w->text, r.right, static_line, &c);
                }
                break;
            }
            EndPaint(h, &ps);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SETTEXT:
        DefWindowProcA(h, msg, wp, lp);
        wnd_invalidate(w, NULL, 1);
        return TRUE;
    case WM_ENABLE:
        wnd_invalidate(w, NULL, 1);
        return 0;
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    case WM_GETDLGCODE:
        return DLGC_STATIC;
    }
    return DefWindowProcA(h, msg, wp, lp);
}

/* ---- Edit (one line) ---- */

typedef struct Edit {
    char text[512];
    int len;
    int caret, anchor;          /* the selection is anchor..caret */
    int scroll;                 /* first character shown */
    int limit;
    int blink;
    int dragging;
} Edit;

static Edit *edit(Wnd *w)
{
    if (!w->ctl) {
        Edit *e = calloc(1, sizeof(Edit));

        e->limit = 511;
        SDL_strlcpy(e->text, w->text, sizeof e->text);
        e->len = (int)strlen(e->text);
        w->ctl = e;
    }
    return (Edit *)w->ctl;
}

static void edit_visible(Wnd *w, Edit *e)
{
    RECT r;
    PFont *f = ctl_font(w);

    GetClientRect(w->h, &r);
    if (e->caret < e->scroll)
        e->scroll = e->caret;
    while (e->scroll < e->caret && font_text_width(f, e->text + e->scroll, e->caret - e->scroll) > r.right - 4)
        e->scroll++;
}

static void edit_changed(Wnd *w)
{
    notify(w, EN_UPDATE);
    notify(w, EN_CHANGE);
}

static void edit_replace(Wnd *w, Edit *e, const char *s, int n)
{
    int a = e->anchor < e->caret ? e->anchor : e->caret, b = e->anchor < e->caret ? e->caret : e->anchor;

    if (e->len - (b - a) + n > e->limit)
        n = e->limit - (e->len - (b - a));
    if (n < 0)
        n = 0;
    memmove(e->text + a + n, e->text + b, (size_t)(e->len - b + 1));
    memcpy(e->text + a, s, (size_t)n);
    e->len += n - (b - a);
    e->caret = e->anchor = a + n;
    edit_visible(w, e);
    wnd_invalidate(w, NULL, 1);
    edit_changed(w);
}

static int edit_pos_at(Wnd *w, Edit *e, int x)
{
    PFont *f = ctl_font(w);
    int i, px = 2;

    for (i = e->scroll; i < e->len; i++) {
        int cw = font_char_width(f, (unsigned char)e->text[i]);

        if (x < px + cw / 2)
            return i;
        px += cw;
    }
    return e->len;
}

static LRESULT CALLBACK EditProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    Wnd *w = wnd_get(h);
    Edit *e;

    if (!w)
        return 0;
    e = edit(w);
    w->ctlFree = ctl_free;
    switch (msg) {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(h, &ps);
            PDC *dc = dc_get(hdc);
            PFont *f = ctl_font(w);
            RECT r;
            uint32_t bg, fg;
            int a = e->anchor < e->caret ? e->anchor : e->caret, b = e->anchor < e->caret ? e->caret : e->anchor;
            int x = 2, y, i, focus = port_focus == w;

            GetClientRect(h, &r);
            bg = (w->style & WS_DISABLED) ? sys_xrgb(COLOR_BTNFACE) : ctl_color(w, WM_CTLCOLOREDIT, hdc, NULL);
            fg = (w->style & WS_DISABLED) ? sys_xrgb(COLOR_GRAYTEXT) : color_xrgb(dc->text);
            gdi_fill(dc, &r, bg);
            y = (r.bottom - f->height) / 2;
            for (i = e->scroll; i < e->len && x < r.right; i++) {
                int cw = font_char_width(f, (unsigned char)e->text[i]);
                int sel = focus && i >= a && i < b;

                if (sel)
                    gdi_fill(dc, &(RECT){x, y, x + cw, y + f->height}, sys_xrgb(COLOR_HIGHLIGHT));
                font_draw(dc, f, x, y, e->text + i, 1, sel ? sys_xrgb(COLOR_HIGHLIGHTTEXT) : fg);
                if (i == e->caret - 1 && 0)
                    ;
                x += cw;
            }
            if (focus && e->blink == 0 && a == b) {
                int cx = 2 + font_text_width(f, e->text + e->scroll, e->caret - e->scroll);

                gdi_vline(dc, cx, y, y + f->height, fg);
            }
            EndPaint(h, &ps);
        }
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SETFOCUS:
        e->blink = 0;
        SetTimer(h, 1, 530, NULL);
        wnd_invalidate(w, NULL, 1);
        notify(w, EN_SETFOCUS);
        return 0;
    case WM_KILLFOCUS:
        KillTimer(h, 1);
        wnd_invalidate(w, NULL, 1);
        notify(w, EN_KILLFOCUS);
        return 0;
    case WM_TIMER:
        e->blink ^= 1;
        wnd_invalidate(w, NULL, 1);
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        SetFocus(h);
        if (msg == WM_LBUTTONDBLCLK) {
            e->anchor = 0;
            e->caret = e->len;
        } else {
            e->caret = e->anchor = edit_pos_at(w, e, (short)LOWORD(lp));
            e->dragging = 1;
            SetCapture(h);
        }
        e->blink = 0;
        wnd_invalidate(w, NULL, 1);
        return 0;
    case WM_MOUSEMOVE:
        if (e->dragging) {
            e->caret = edit_pos_at(w, e, (short)LOWORD(lp));
            edit_visible(w, e);
            wnd_invalidate(w, NULL, 1);
        }
        return 0;
    case WM_LBUTTONUP:
        if (e->dragging) {
            e->dragging = 0;
            ReleaseCapture();
        }
        return 0;
    case WM_CHAR:
        if (wp == 8) {
            if (e->anchor == e->caret && e->caret > 0)
                e->anchor = e->caret - 1;
            if (e->anchor != e->caret)
                edit_replace(w, e, "", 0);
        } else if (wp >= 32 && wp < 256 && wp != 127) {
            char c = (char)wp;

            edit_replace(w, e, &c, 1);
        }
        e->blink = 0;
        return 0;
    case WM_KEYDOWN:
        {
            int shift = port_key_down(VK_SHIFT);

            switch (wp) {
            case VK_LEFT:
                if (e->caret > 0)
                    e->caret--;
                break;
            case VK_RIGHT:
                if (e->caret < e->len)
                    e->caret++;
                break;
            case VK_HOME:
                e->caret = 0;
                break;
            case VK_END:
                e->caret = e->len;
                break;
            case VK_DELETE:
                if (e->anchor == e->caret && e->caret < e->len)
                    e->anchor = e->caret + 1;
                if (e->anchor != e->caret)
                    edit_replace(w, e, "", 0);
                return 0;
            default:
                return 0;
            }
            if (!shift)
                e->anchor = e->caret;
            e->blink = 0;
            edit_visible(w, e);
            wnd_invalidate(w, NULL, 1);
        }
        return 0;
    case EM_SETSEL:
        {
            int a = (int)(INT_PTR)wp, b = (int)(INT_PTR)lp;

            if (a < 0) {
                e->anchor = e->caret;
            } else {
                if (b < 0 || b > e->len)
                    b = e->len;
                if (a > e->len)
                    a = e->len;
                e->anchor = a;
                e->caret = b;
            }
            edit_visible(w, e);
            wnd_invalidate(w, NULL, 1);
        }
        return 0;
    case EM_GETSEL:
        {
            int a = e->anchor < e->caret ? e->anchor : e->caret, b = e->anchor < e->caret ? e->caret : e->anchor;

            if (wp)
                *(DWORD *)wp = (DWORD)a;
            if (lp)
                *(DWORD *)lp = (DWORD)b;
            return MAKELRESULT(a, b);
        }
    case EM_LIMITTEXT:
        e->limit = wp && wp < 511 ? (int)wp : 511;
        return 0;
    case WM_SETTEXT:
        SDL_strlcpy(e->text, lp ? (const char *)lp : "", sizeof e->text);
        e->len = (int)strlen(e->text);
        e->caret = e->anchor = e->scroll = 0;
        DefWindowProcA(h, msg, wp, lp);
        wnd_invalidate(w, NULL, 1);
        edit_changed(w);
        return TRUE;
    case WM_GETTEXT:
        if (wp > 0) {
            SDL_strlcpy((char *)lp, e->text, (size_t)wp);
            return (LRESULT)strlen((char *)lp);
        }
        return 0;
    case WM_GETTEXTLENGTH:
        return e->len;
    case WM_GETDLGCODE:
        return DLGC_WANTCHARS | DLGC_HASSETSEL | DLGC_WANTARROWS;
    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT) {
            SetCursor(LoadCursorA(NULL, IDC_IBEAM));
            return TRUE;
        }
        break;
    }
    return DefWindowProcA(h, msg, wp, lp);
}

/* ---- ListBox ---- */

typedef struct LbItem {
    char *text;
    ULONG_PTR data;
} LbItem;

typedef struct ListBox {
    LbItem *items;
    int n, cap;
    int sel;                    /* -1: none */
    int caret;
    int top;
    int itemH;
    int redraw;
    int measured;
    int tracking;
} ListBox;

static int lb_strings(Wnd *w)
{
    return !(w->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || (w->style & LBS_HASSTRINGS);
}

static void lb_free(Wnd *w)
{
    ListBox *l = (ListBox *)w->ctl;
    int i;

    if (!l)
        return;
    for (i = 0; i < l->n; i++)
        free(l->items[i].text);
    free(l->items);
    free(l);
    w->ctl = NULL;
}

static ListBox *lb(Wnd *w)
{
    if (!w->ctl) {
        ListBox *l = calloc(1, sizeof(ListBox));

        l->sel = -1;
        l->redraw = 1;
        l->itemH = ctl_font(w)->height;
        w->ctl = l;
        w->ctlFree = lb_free;
    }
    return (ListBox *)w->ctl;
}

static int lb_visible(Wnd *w, ListBox *l)
{
    RECT r;

    GetClientRect(w->h, &r);
    return l->itemH > 0 ? r.bottom / l->itemH : 1;
}

static void lb_update_scroll(Wnd *w, ListBox *l)
{
    int vis = lb_visible(w, l), max = l->n - vis;

    if (max < 0)
        max = 0;
    if (l->top > max)
        l->top = max;
    if (l->top < 0)
        l->top = 0;
    if (w->style & WS_VSCROLL) {
        SetScrollRange(w->h, SB_VERT, 0, max, FALSE);
        SetScrollPos(w->h, SB_VERT, l->top, TRUE);
    }
}

static void lb_changed(Wnd *w, ListBox *l)
{
    lb_update_scroll(w, l);
    if (l->redraw)
        wnd_invalidate(w, NULL, 1);
}

static void lb_measure(Wnd *w, ListBox *l)
{
    if (l->measured)
        return;
    l->measured = 1;
    if ((w->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) && w->parent) {
        MEASUREITEMSTRUCT mis;

        memset(&mis, 0, sizeof mis);
        mis.CtlType = ODT_LISTBOX;
        mis.CtlID = (UINT)w->id;
        mis.itemHeight = (UINT)l->itemH;
        SendMessageA(w->parent->h, WM_MEASUREITEM, (WPARAM)w->id, (LPARAM)&mis);
        if (mis.itemHeight)
            l->itemH = (int)mis.itemHeight;
    }
}

/* without LBS_NOINTEGRALHEIGHT the box shows only whole items */
static void lb_integral(Wnd *w, ListBox *l)
{
    int lft, t, r, b, ch;

    if ((w->style & LBS_NOINTEGRALHEIGHT) || l->itemH <= 0)
        return;
    wnd_client_offset(w, &lft, &t, &r, &b);
    ch = w->rc.bottom - w->rc.top - t - b;
    if (ch % l->itemH && ch > l->itemH)
        SetWindowPos(w->h, NULL, 0, 0, w->rc.right - w->rc.left, ch / l->itemH * l->itemH + t + b,
                     SWP_NOMOVE | SWP_NOZORDER);
}

static int lb_insert(Wnd *w, ListBox *l, int at, LPARAM lp)
{
    LbItem it;

    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 16;
        l->items = realloc(l->items, (size_t)l->cap * sizeof(LbItem));
    }
    it.text = lb_strings(w) ? SDL_strdup(lp ? (const char *)lp : "") : NULL;
    it.data = lb_strings(w) ? 0 : (ULONG_PTR)lp;
    if (at < 0 || at > l->n) {
        at = l->n;
        if ((w->style & LBS_SORT) && lb_strings(w))
            for (at = 0; at < l->n && SDL_strcasecmp(l->items[at].text, it.text) <= 0; at++)
                ;
    }
    memmove(l->items + at + 1, l->items + at, (size_t)(l->n - at) * sizeof(LbItem));
    l->items[at] = it;
    l->n++;
    if (l->sel >= at)
        l->sel++;
    lb_changed(w, l);
    return at;
}

static void lb_ensure_visible(Wnd *w, ListBox *l, int i)
{
    int vis = lb_visible(w, l);

    if (i < 0)
        return;
    if (i < l->top)
        l->top = i;
    else if (i >= l->top + vis)
        l->top = i - vis + 1;
    lb_update_scroll(w, l);
}

static void lb_select(Wnd *w, ListBox *l, int i, int notifyIt)
{
    if (i >= l->n)
        i = l->n - 1;
    if (i < -1)
        i = -1;
    l->caret = i;
    if (i == l->sel)
        return;
    l->sel = i;
    lb_ensure_visible(w, l, i);
    wnd_invalidate(w, NULL, 1);
    if (notifyIt && (w->style & LBS_NOTIFY))
        notify(w, LBN_SELCHANGE);
}

static void lb_paint(Wnd *w, ListBox *l)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(w->h, &ps);
    PDC *dc = dc_get(hdc);
    PFont *f = ctl_font(w);
    RECT r;
    int i, y, ownerDraw = (w->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) != 0;
    HBRUSH br;
    uint32_t bg, fg;

    GetClientRect(w->h, &r);
    br = get_brush(w, WM_CTLCOLORLISTBOX, hdc);
    bg = brush_get(br) ? color_xrgb(brush_get(br)->color) : sys_xrgb(COLOR_WINDOW);
    fg = color_xrgb(dc->text);
    for (i = l->top, y = 0; i < l->n && y < r.bottom; i++, y += l->itemH) {
        RECT ir = {0, y, r.right, y + l->itemH};
        int sel = i == l->sel;

        if (ownerDraw) {
            DRAWITEMSTRUCT dis;

            memset(&dis, 0, sizeof dis);
            dis.CtlType = ODT_LISTBOX;
            dis.CtlID = (UINT)w->id;
            dis.itemID = (UINT)i;
            dis.itemAction = ODA_DRAWENTIRE;
            dis.itemState = (sel ? ODS_SELECTED : 0) | (sel && port_focus == w ? ODS_FOCUS : 0) |
                            ((w->style & WS_DISABLED) ? ODS_DISABLED : 0);
            dis.hwndItem = w->h;
            dis.hDC = hdc;
            dis.rcItem = ir;
            dis.itemData = lb_strings(w) ? 0 : l->items[i].data;
            if (lb_strings(w))
                dis.itemData = l->items[i].data;
            if (w->parent)
                SendMessageA(w->parent->h, WM_DRAWITEM, (WPARAM)w->id, (LPARAM)&dis);
            continue;
        }
        gdi_fill(dc, &ir, sel ? sys_xrgb(COLOR_HIGHLIGHT) : bg);
        font_draw(dc, f, 2, y + (l->itemH - f->height) / 2, l->items[i].text, -1,
                  sel ? sys_xrgb(COLOR_HIGHLIGHTTEXT) : fg);
        if (sel && port_focus == w)
            DrawFocusRect(hdc, &ir);
    }
    if (y < r.bottom)
        FillRect(hdc, &(RECT){0, y, r.right, r.bottom}, br);
    EndPaint(w->h, &ps);
}

static int lb_item_at(ListBox *l, int y)
{
    int i;

    if (l->itemH <= 0)
        return -1;
    i = l->top + (y < 0 ? -1 : y / l->itemH);
    return i >= 0 && i < l->n ? i : -1;
}

static LRESULT CALLBACK ListBoxProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    Wnd *w = wnd_get(h);
    ListBox *l;
    int i;

    if (!w)
        return 0;
    l = lb(w);
    switch (msg) {
    case WM_CREATE:
        lb_measure(w, l);
        lb_integral(w, l);
        lb_update_scroll(w, l);
        return 0;
    case WM_SIZE:
        lb_update_scroll(w, l);
        return 0;
    case WM_PAINT:
        lb_paint(w, l);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SETFONT:
        DefWindowProcA(h, msg, wp, lp);
        if (!(w->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)))
            l->itemH = ctl_font(w)->height;
        lb_changed(w, l);
        return 0;
    case WM_SETREDRAW:
        l->redraw = wp != 0;
        return 0;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        SetFocus(h);
        i = lb_item_at(l, (short)HIWORD(lp));
        if (i >= 0)
            lb_select(w, l, i, 1);
        if (msg == WM_LBUTTONDBLCLK && i >= 0 && (w->style & LBS_NOTIFY))
            notify(w, LBN_DBLCLK);
        else if (msg == WM_LBUTTONDOWN) {
            l->tracking = 1;
            SetCapture(h);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (l->tracking) {
            i = lb_item_at(l, (short)HIWORD(lp));
            if (i >= 0)
                lb_select(w, l, i, 1);
        }
        return 0;
    case WM_LBUTTONUP:
        if (l->tracking) {
            l->tracking = 0;
            ReleaseCapture();
        }
        return 0;
    case WM_MOUSEWHEEL:
        l->top -= (short)HIWORD(wp) / 40;
        lb_update_scroll(w, l);
        wnd_invalidate(w, NULL, 1);
        return 0;
    case WM_VSCROLL:
        {
            int vis = lb_visible(w, l);

            switch (LOWORD(wp)) {
            case SB_LINEUP:
                l->top--;
                break;
            case SB_LINEDOWN:
                l->top++;
                break;
            case SB_PAGEUP:
                l->top -= vis > 1 ? vis - 1 : 1;
                break;
            case SB_PAGEDOWN:
                l->top += vis > 1 ? vis - 1 : 1;
                break;
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
                l->top = (short)HIWORD(wp);
                break;
            case SB_TOP:
                l->top = 0;
                break;
            case SB_BOTTOM:
                l->top = l->n;
                break;
            default:
                return 0;
            }
            lb_update_scroll(w, l);
            wnd_invalidate(w, NULL, 1);
        }
        return 0;
    case WM_KEYDOWN:
        {
            int vis = lb_visible(w, l), to = l->caret;

            if ((w->style & LBS_WANTKEYBOARDINPUT) && lb_strings(w) && w->parent) {
                LRESULT r = SendMessageA(w->parent->h, WM_VKEYTOITEM, MAKEWPARAM(wp, l->caret), (LPARAM)h);

                if (r == -2)
                    return 0;
                if (r >= 0) {
                    lb_select(w, l, (int)r, 1);
                    return 0;
                }
            }
            switch (wp) {
            case VK_UP:
                to--;
                break;
            case VK_DOWN:
                to++;
                break;
            case VK_PRIOR:
                to -= vis - 1;
                break;
            case VK_NEXT:
                to += vis - 1;
                break;
            case VK_HOME:
                to = 0;
                break;
            case VK_END:
                to = l->n - 1;
                break;
            default:
                return 0;
            }
            if (to < 0)
                to = 0;
            if (l->n)
                lb_select(w, l, to, 1);
        }
        return 0;
    case WM_CHAR:
        if ((w->style & LBS_WANTKEYBOARDINPUT) && !lb_strings(w) && w->parent) {
            LRESULT r = SendMessageA(w->parent->h, WM_CHARTOITEM, MAKEWPARAM(wp, l->caret), (LPARAM)h);

            if (r >= 0)
                lb_select(w, l, (int)r, 1);
            return 0;
        }
        if (lb_strings(w) && wp > 32) {
            int k;

            for (k = 1; k <= l->n; k++) {
                int j = (l->caret + k + l->n) % l->n;

                if (SDL_tolower((unsigned char)l->items[j].text[0]) == SDL_tolower((int)wp)) {
                    lb_select(w, l, j, 1);
                    break;
                }
            }
        }
        return 0;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        wnd_invalidate(w, NULL, 1);
        if (w->style & LBS_NOTIFY)
            notify(w, msg == WM_SETFOCUS ? LBN_SETFOCUS : LBN_KILLFOCUS);
        return 0;
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;
    case LB_ADDSTRING:
        return lb_insert(w, l, -1, lp);
    case LB_INSERTSTRING:
        return lb_insert(w, l, (int)(INT_PTR)wp < 0 ? l->n : (int)wp, lp);
    case LB_DELETESTRING:
        i = (int)wp;
        if (i < 0 || i >= l->n)
            return LB_ERR;
        free(l->items[i].text);
        memmove(l->items + i, l->items + i + 1, (size_t)(l->n - i - 1) * sizeof(LbItem));
        l->n--;
        if (l->sel == i)
            l->sel = -1;
        else if (l->sel > i)
            l->sel--;
        lb_changed(w, l);
        return l->n;
    case LB_RESETCONTENT:
        for (i = 0; i < l->n; i++)
            free(l->items[i].text);
        l->n = 0;
        l->sel = -1;
        l->top = 0;
        lb_changed(w, l);
        return 0;
    case LB_GETCOUNT:
        return l->n;
    case LB_GETCURSEL:
        return l->sel;
    case LB_SETCURSEL:
        lb_select(w, l, (int)(INT_PTR)wp, 0);
        return (int)(INT_PTR)wp < 0 || (int)wp >= l->n ? LB_ERR : (LRESULT)wp;
    case LB_SETSEL:
        return LB_ERR;          /* only for multiple-selection list boxes */
    case LB_GETSEL:
        return (int)wp == l->sel;
    case LB_GETTEXT:
        i = (int)wp;
        if (i < 0 || i >= l->n)
            return LB_ERR;
        if (lb_strings(w)) {
            strcpy((char *)lp, l->items[i].text);
            return (LRESULT)strlen(l->items[i].text);
        }
        memcpy((void *)lp, &l->items[i].data, sizeof(DWORD));
        return sizeof(DWORD);
    case LB_GETTEXTLEN:
        i = (int)wp;
        if (i < 0 || i >= l->n)
            return LB_ERR;
        return lb_strings(w) ? (LRESULT)strlen(l->items[i].text) : (LRESULT)sizeof(DWORD);
    case LB_GETITEMDATA:
        i = (int)wp;
        return i >= 0 && i < l->n ? (LRESULT)l->items[i].data : LB_ERR;
    case LB_SETITEMDATA:
        i = (int)wp;
        if (i < 0 || i >= l->n)
            return LB_ERR;
        l->items[i].data = (ULONG_PTR)lp;
        return 0;
    case LB_GETTOPINDEX:
        return l->top;
    case LB_SETTOPINDEX:
        l->top = (int)wp;
        lb_update_scroll(w, l);
        wnd_invalidate(w, NULL, 1);
        return 0;
    case LB_SETITEMHEIGHT:
        if (LOWORD(lp)) {
            l->itemH = LOWORD(lp);
            lb_integral(w, l);
            lb_changed(w, l);
        }
        return 0;
    case LB_GETITEMHEIGHT:
        return l->itemH;
    case LB_GETITEMRECT:
        {
            RECT *r = (RECT *)lp, c;

            GetClientRect(h, &c);
            *r = (RECT){0, ((int)wp - l->top) * l->itemH, c.right, ((int)wp - l->top + 1) * l->itemH};
        }
        return 0;
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
        if (!lb_strings(w))
            return LB_ERR;
        for (i = 0; i < l->n; i++) {
            int j = ((int)(INT_PTR)wp + 1 + i) % l->n;
            const char *s = (const char *)lp;

            if (j < 0)
                j = 0;
            if (msg == LB_FINDSTRINGEXACT ? !SDL_strcasecmp(l->items[j].text, s)
                                          : !SDL_strncasecmp(l->items[j].text, s, strlen(s))) {
                if (msg == LB_SELECTSTRING)
                    lb_select(w, l, j, 0);
                return j;
            }
        }
        return LB_ERR;
    case LB_SETCARETINDEX:
        l->caret = (int)wp;
        return 0;
    case LB_GETCARETINDEX:
        return l->caret;
    }
    return DefWindowProcA(h, msg, wp, lp);
}

void controls_init(void)
{
    wnd_register_builtin("Button", ButtonProc, CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW, 8, 1);
    wnd_register_builtin("Static", StaticProc, 0, 8, 1);
    wnd_register_builtin("Edit", EditProc, CS_DBLCLKS, 8, 0);
    wnd_register_builtin("ListBox", ListBoxProc, CS_DBLCLKS, 8, 1);
    dlg_init();
}
