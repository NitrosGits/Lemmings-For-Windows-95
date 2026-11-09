/*
 * GDI: device contexts, bitmaps (8-bit DIB sections with their colour table,
 * 32-bit "device" bitmaps), blits that convert through colour tables, brushes,
 * pens, palettes and clipping regions. Windows draw into their top-level
 * window's 32-bit framebuffer (wnd.c), which msg.c puts on the screen.
 *
 * The display is true colour, as on today's Windows: an 8-bit DIB shows the
 * colours of its own table, so palettes only keep their entries.
 */
#include "port_internal.h"

/* ---- Rectangles and regions ---- */

int rect_empty(const RECT *r)
{
    return r->left >= r->right || r->top >= r->bottom;
}

int rect_intersect(RECT *d, const RECT *a, const RECT *b)
{
    RECT t;

    t.left = a->left > b->left ? a->left : b->left;
    t.top = a->top > b->top ? a->top : b->top;
    t.right = a->right < b->right ? a->right : b->right;
    t.bottom = a->bottom < b->bottom ? a->bottom : b->bottom;
    if (rect_empty(&t)) {
        memset(d, 0, sizeof *d);
        return 0;
    }
    *d = t;
    return 1;
}

void rgn_set(Region *g, const RECT *r)
{
    g->n = 0;
    if (r && !rect_empty(r))
        g->r[g->n++] = *r;
}

void rgn_intersect(Region *g, const RECT *r)
{
    int i, n = 0;

    for (i = 0; i < g->n; i++)
        if (rect_intersect(&g->r[n], &g->r[i], r))
            n++;
    g->n = n;
}

void rgn_subtract(Region *g, const RECT *s)
{
    Region out;
    int i;

    out.n = 0;
    for (i = 0; i < g->n; i++) {
        RECT a = g->r[i], x;

        if (!rect_intersect(&x, &a, s)) {
            if (out.n < PORT_MAX_RECTS)
                out.r[out.n++] = a;
            continue;
        }
        /* up to four pieces round the hole */
        if (a.top < x.top && out.n < PORT_MAX_RECTS)
            out.r[out.n++] = (RECT){a.left, a.top, a.right, x.top};
        if (x.bottom < a.bottom && out.n < PORT_MAX_RECTS)
            out.r[out.n++] = (RECT){a.left, x.bottom, a.right, a.bottom};
        if (a.left < x.left && out.n < PORT_MAX_RECTS)
            out.r[out.n++] = (RECT){a.left, x.top, x.left, x.bottom};
        if (x.right < a.right && out.n < PORT_MAX_RECTS)
            out.r[out.n++] = (RECT){x.right, x.top, a.right, x.bottom};
    }
    *g = out;
}

/* ---- Colours ---- */

static const COLORREF g_sysColors[25] = {
    0xC0C0C0, 0x808000, 0x800000, 0x808080, 0xC0C0C0, 0xFFFFFF, 0x000000, 0x000000, 0x000000, 0xFFFFFF,
    0xC0C0C0, 0xC0C0C0, 0x808080, 0x800000, 0xFFFFFF, 0xC0C0C0, 0x808080, 0x808080, 0x000000, 0xC0C0C0,
    0xFFFFFF, 0x000000, 0xDFDFDF, 0x000000, 0xE1FFFF,
};

DWORD WINAPI GetSysColor(int index)
{
    return index >= 0 && index < 25 ? g_sysColors[index] : 0;
}

uint32_t color_xrgb(COLORREF c)
{
    return ((c & 0xff) << 16) | (c & 0xff00) | ((c >> 16) & 0xff);
}

uint32_t sys_xrgb(int index)
{
    return color_xrgb(GetSysColor(index));
}

int surf_nearest(const Surface *s, uint32_t xrgb)
{
    int r = (xrgb >> 16) & 0xff, g = (xrgb >> 8) & 0xff, b = xrgb & 0xff;
    int i, best = 0, bestd = 1 << 30, n = s->ncolors ? s->ncolors : 256;

    for (i = 0; i < n; i++) {
        int dr = s->colors[i].rgbRed - r, dg = s->colors[i].rgbGreen - g, db = s->colors[i].rgbBlue - b;
        int d = dr * dr + dg * dg + db * db;

        if (d < bestd) {
            bestd = d;
            best = i;
            if (!d)
                break;
        }
    }
    return best;
}

static inline uint32_t surf_get(const Surface *s, int x, int y)
{
    const BYTE *row = surf_row(s, y);

    if (s->bpp == 8) {
        const RGBQUAD *q = &s->colors[row[x]];
        return ((uint32_t)q->rgbRed << 16) | ((uint32_t)q->rgbGreen << 8) | q->rgbBlue;
    }
    return ((const uint32_t *)row)[x] & 0xffffff;
}

/* ---- Objects ---- */

static Surface g_screenSurf;            /* the desktop: drawing there is dropped */
static HBITMAP g_defaultBitmap;
static HGDIOBJ g_stock[18];

static void *obj_new(int type, size_t size, uintptr_t *h)
{
    PGdi *g = calloc(1, sizeof(PGdi) + size);

    *h = port_handle_new(type, g);
    return g + 1;
}

static PGdi *gdi_header(HGDIOBJ h)
{
    int t = port_handle_type(h);
    PGdi *g;

    if (t < H_BITMAP || t > H_PALETTE)
        return NULL;
    g = (PGdi *)port_handle_get(h, t);
    return g;
}

#define OBJ(type, T, h) ((T *)((PGdi *)port_handle_get(h, type) ? (PGdi *)port_handle_get(h, type) + 1 : NULL))

PBrush *brush_get(HBRUSH h)
{
    static PBrush sys[32];
    uintptr_t v = (uintptr_t)h;

    if (v >= 1 && v <= 31) {    /* (HBRUSH)(COLOR_x + 1) */
        PBrush *b = &sys[v];

        b->style = BS_SOLID;
        b->color = GetSysColor((int)v - 1);
        b->sysColor = (int)v - 1;
        b->stock = 1;
        return b;
    }
    return OBJ(H_BRUSH, PBrush, h);
}

static PBitmap *bitmap_get(HBITMAP h)
{
    return OBJ(H_BITMAP, PBitmap, h);
}

static PPalette *palette_get(HPALETTE h)
{
    return OBJ(H_PALETTE, PPalette, h);
}

PFont *font_get(HFONT h)
{
    PFont *f = OBJ(H_FONT, PFont, h);

    return f ? f : OBJ(H_FONT, PFont, font_default());
}

HBRUSH WINAPI CreateSolidBrush(COLORREF c)
{
    uintptr_t h;
    PBrush *b = obj_new(H_BRUSH, sizeof(PBrush), &h);

    b->style = BS_SOLID;
    b->color = c & 0xffffff;
    b->sysColor = -1;
    return (HBRUSH)h;
}

HBRUSH WINAPI CreateHatchBrush(int style, COLORREF c)
{
    uintptr_t h;
    PBrush *b = obj_new(H_BRUSH, sizeof(PBrush), &h);

    b->style = BS_HATCHED;
    b->hatch = style;
    b->color = c & 0xffffff;
    b->sysColor = -1;
    return (HBRUSH)h;
}

HBRUSH WINAPI CreateBrushIndirect(const LOGBRUSH *lb)
{
    if (lb->lbStyle == BS_HATCHED)
        return CreateHatchBrush((int)lb->lbHatch, lb->lbColor);
    if (lb->lbStyle == BS_NULL)
        return (HBRUSH)GetStockObject(NULL_BRUSH);
    return CreateSolidBrush(lb->lbColor);
}

HBRUSH WINAPI GetSysColorBrush(int index)
{
    return (HBRUSH)(uintptr_t)(index + 1);
}

HPEN WINAPI CreatePen(int style, int width, COLORREF c)
{
    uintptr_t h;
    PPen *p = obj_new(H_PEN, sizeof(PPen), &h);

    (void)width;
    p->style = style;
    p->color = c & 0xffffff;
    return (HPEN)h;
}

HPALETTE WINAPI CreatePalette(const LOGPALETTE *lp)
{
    uintptr_t h;
    PPalette *p = obj_new(H_PALETTE, sizeof(PPalette), &h);

    p->n = lp->palNumEntries > 256 ? 256 : lp->palNumEntries;
    memcpy(p->e, lp->palPalEntry, (size_t)p->n * sizeof(PALETTEENTRY));
    return (HPALETTE)h;
}

static HBITMAP new_bitmap(int w, int h, int bpp, int bottomUp, const RGBQUAD *colors, int ncolors, PBitmap **out)
{
    uintptr_t hv;
    PBitmap *b = obj_new(H_BITMAP, sizeof(PBitmap), &hv);

    if (w < 1)
        w = 1;
    if (h < 1)
        h = 1;
    b->s.w = w;
    b->s.h = h;
    b->s.bpp = bpp;
    b->s.pitch = ((w * bpp + 31) / 32) * 4;
    b->s.bottomUp = bottomUp;
    b->s.bits = calloc((size_t)b->s.pitch, (size_t)h);
    b->s.ownBits = 1;
    if (colors && ncolors) {
        memcpy(b->s.colors, colors, (size_t)ncolors * sizeof(RGBQUAD));
        b->s.ncolors = ncolors;
    }
    b->header.biSize = sizeof(BITMAPINFOHEADER);
    b->header.biWidth = w;
    b->header.biHeight = bottomUp ? h : -h;
    b->header.biPlanes = 1;
    b->header.biBitCount = (WORD)bpp;
    if (out)
        *out = b;
    return (HBITMAP)hv;
}

static int dib_colors(const BITMAPINFOHEADER *bh)
{
    if (bh->biBitCount > 8)
        return 0;
    return bh->biClrUsed ? (int)bh->biClrUsed : 1 << bh->biBitCount;
}

/* A surface over a DIB's pixels (not copied) */
static int dib_view(Surface *s, const BITMAPINFO *bmi, const void *bits, UINT usage, HPALETTE pal)
{
    const BITMAPINFOHEADER *bh = &bmi->bmiHeader;
    int n;

    memset(s, 0, sizeof *s);
    if (bh->biSize == sizeof(BITMAPCOREHEADER)) {
        const BITMAPCOREHEADER *ch = (const BITMAPCOREHEADER *)bh;
        const RGBTRIPLE *t = (const RGBTRIPLE *)(ch + 1);
        int i;

        s->w = ch->bcWidth;
        s->h = ch->bcHeight;
        s->bpp = ch->bcBitCount;
        s->bottomUp = 1;
        n = s->bpp <= 8 ? 1 << s->bpp : 0;
        for (i = 0; i < n; i++) {
            s->colors[i].rgbRed = t[i].rgbtRed;
            s->colors[i].rgbGreen = t[i].rgbtGreen;
            s->colors[i].rgbBlue = t[i].rgbtBlue;
        }
        s->ncolors = n;
    } else {
        s->w = bh->biWidth;
        s->h = bh->biHeight < 0 ? -bh->biHeight : bh->biHeight;
        s->bottomUp = bh->biHeight > 0;
        s->bpp = bh->biBitCount;
        n = dib_colors(bh);
        if (n > 256)
            n = 256;
        if (usage == DIB_PAL_COLORS && pal) {
            PPalette *p = palette_get(pal);
            const WORD *idx = (const WORD *)((const BYTE *)bh + bh->biSize);
            int i;

            for (i = 0; i < n && p; i++) {
                PALETTEENTRY e = p->e[idx[i] < p->n ? idx[i] : 0];
                s->colors[i] = (RGBQUAD){e.peBlue, e.peGreen, e.peRed, 0};
            }
        } else
            memcpy(s->colors, (const BYTE *)bh + bh->biSize, (size_t)n * sizeof(RGBQUAD));
        s->ncolors = n;
        if (bh->biCompression != BI_RGB) {
            port_trace("DIB compression %u not supported", bh->biCompression);
            return 0;
        }
    }
    s->pitch = ((s->w * s->bpp + 31) / 32) * 4;
    s->bits = (BYTE *)bits;
    return s->bpp == 1 || s->bpp == 4 || s->bpp == 8 || s->bpp == 24 || s->bpp == 32;
}

/* a pixel of a 1/4/8/24/32-bit surface as 0xRRGGBB */
static uint32_t any_get(const Surface *s, int x, int y)
{
    const BYTE *row = surf_row(s, y);
    int i;

    switch (s->bpp) {
    case 1:
        i = (row[x >> 3] >> (7 - (x & 7))) & 1;
        break;
    case 4:
        i = (row[x >> 1] >> (x & 1 ? 0 : 4)) & 15;
        break;
    case 8:
        i = row[x];
        break;
    case 24:
        return ((uint32_t)row[x * 3 + 2] << 16) | ((uint32_t)row[x * 3 + 1] << 8) | row[x * 3];
    default:
        return ((const uint32_t *)row)[x] & 0xffffff;
    }
    return ((uint32_t)s->colors[i].rgbRed << 16) | ((uint32_t)s->colors[i].rgbGreen << 8) | s->colors[i].rgbBlue;
}

HBITMAP WINAPI CreateDIBSection(HDC hdc, const BITMAPINFO *bmi, UINT usage, void **bits, HANDLE section,
                                DWORD offset)
{
    const BITMAPINFOHEADER *bh = &bmi->bmiHeader;
    Surface view;
    PBitmap *b;
    HBITMAP h;
    PDC *dc = dc_get(hdc);

    (void)section;
    (void)offset;
    if (!dib_view(&view, bmi, NULL, usage, dc ? dc->palette : NULL) || (view.bpp != 8 && view.bpp != 32)) {
        port_trace("CreateDIBSection: %d bits not supported", bh->biBitCount);
        if (bits)
            *bits = NULL;
        return NULL;
    }
    h = new_bitmap(view.w, view.h, view.bpp, view.bottomUp, view.colors, view.ncolors, &b);
    b->dib = 1;
    b->header = *bh;
    if (bits)
        *bits = b->s.bits;
    return h;
}

HBITMAP WINAPI CreateCompatibleBitmap(HDC hdc, int w, int h)
{
    (void)hdc;
    return new_bitmap(w, h, 32, 0, NULL, 0, NULL);
}

HBITMAP WINAPI CreateBitmap(int w, int h, UINT planes, UINT bpp, const void *data)
{
    PBitmap *b;
    HBITMAP hb = new_bitmap(w, h, 32, 0, NULL, 0, &b);
    int x, y;

    (void)planes;
    if (data && bpp == 1) {
        const BYTE *p = data;
        int pitch = ((w + 15) / 16) * 2;

        for (y = 0; y < h; y++)
            for (x = 0; x < w; x++)
                ((uint32_t *)surf_row(&b->s, y))[x] = (p[y * pitch + x / 8] >> (7 - x % 8)) & 1 ? 0xffffff : 0;
    }
    return hb;
}

HBITMAP WINAPI CreateDIBitmap(HDC hdc, const BITMAPINFOHEADER *header, DWORD init, const void *bits,
                              const BITMAPINFO *bmi, UINT usage)
{
    PDC *dc = dc_get(hdc);
    int w = header->biWidth, h = header->biHeight < 0 ? -header->biHeight : header->biHeight;
    PBitmap *b;
    HBITMAP hb;
    Surface src;
    int x, y;

    if (header->biSize == sizeof(BITMAPCOREHEADER)) {
        w = ((const BITMAPCOREHEADER *)header)->bcWidth;
        h = ((const BITMAPCOREHEADER *)header)->bcHeight;
    }
    hb = new_bitmap(w, h, 32, 0, NULL, 0, &b);
    if ((init & CBM_INIT) && bits && bmi && dib_view(&src, bmi, bits, usage, dc ? dc->palette : NULL)) {
        for (y = 0; y < h && y < src.h; y++) {
            uint32_t *row = (uint32_t *)surf_row(&b->s, y);

            for (x = 0; x < w && x < src.w; x++)
                row[x] = any_get(&src, x, y);
        }
    }
    return hb;
}

HGDIOBJ WINAPI GetStockObject(int index)
{
    return index >= 0 && index < 18 ? g_stock[index] : NULL;
}

static void obj_free(HGDIOBJ h)
{
    int t = port_handle_type(h);
    PGdi *g = gdi_header(h);

    if (!g)
        return;
    if (t == H_BITMAP) {
        PBitmap *b = (PBitmap *)(g + 1);

        if (b->s.ownBits)
            free(b->s.bits);
    }
    port_handle_free(h);
    free(g);
}

BOOL WINAPI DeleteObject(HGDIOBJ h)
{
    PGdi *g = gdi_header(h);
    int t = port_handle_type(h);

    if (!g)
        return (uintptr_t)h >= 1 && (uintptr_t)h <= 31;
    if ((t == H_BRUSH && ((PBrush *)(g + 1))->stock) || (t == H_PEN && ((PPen *)(g + 1))->stock) ||
        (t == H_FONT && ((PFont *)(g + 1))->stock) || (t == H_PALETTE && ((PPalette *)(g + 1))->stock) ||
        h == g_defaultBitmap)
        return TRUE;
    if (g->selected) {
        g->deleted = 1;         /* freed when it is deselected */
        return TRUE;
    }
    obj_free(h);
    return TRUE;
}

static void sel_count(HGDIOBJ h, int d)
{
    PGdi *g = gdi_header(h);

    if (!g)
        return;
    g->selected += d;
    if (g->selected <= 0) {
        g->selected = 0;
        if (g->deleted)
            obj_free(h);
    }
}

int WINAPI GetObjectA(HGDIOBJ h, int size, LPVOID buf)
{
    switch (port_handle_type(h)) {
    case H_BITMAP:
        {
            PBitmap *b = bitmap_get((HBITMAP)h);
            BITMAP bm;

            memset(&bm, 0, sizeof bm);
            bm.bmWidth = b->s.w;
            bm.bmHeight = b->s.h;
            bm.bmWidthBytes = b->s.pitch;
            bm.bmPlanes = 1;
            bm.bmBitsPixel = (WORD)b->s.bpp;
            bm.bmBits = b->dib ? b->s.bits : NULL;
            if (size > (int)sizeof bm)
                size = sizeof bm;
            memcpy(buf, &bm, (size_t)size);
            return size;
        }
    case H_PALETTE:
        if (size >= 2)
            *(WORD *)buf = (WORD)palette_get((HPALETTE)h)->n;
        return 2;
    }
    return 0;
}

/* ---- DCs ---- */

PDC *dc_get(HDC h)
{
    return (PDC *)port_handle_get(h, H_DC);
}

static PDC *dc_new(int kind)
{
    PDC *dc = calloc(1, sizeof *dc);

    dc->h = (HDC)port_handle_new(H_DC, dc);
    dc->kind = kind;
    dc->brush = (HBRUSH)g_stock[WHITE_BRUSH];
    dc->pen = (HPEN)g_stock[BLACK_PEN];
    dc->font = (HFONT)g_stock[SYSTEM_FONT];
    dc->palette = (HPALETTE)g_stock[DEFAULT_PALETTE];
    dc->text = 0;
    dc->bk = 0xffffff;
    dc->bkMode = OPAQUE;
    dc->s = &g_screenSurf;
    return dc;
}

HDC dc_for_window(Wnd *w, int client, const RECT *paint)
{
    PDC *dc;
    Wnd *top;
    RECT r;

    if (!w || w == port_desktop) {
        dc = dc_new(DCK_SCREEN);
        dc->clip.n = 0;
        return dc->h;
    }
    dc = dc_new(client ? DCK_CLIENT : DCK_WINDOW);
    dc->wnd = w;
    top = wnd_toplevel(w);
    dc->s = &top->fb;
    if (client)
        wnd_client_fb(w, &r);
    else
        wnd_window_fb(w, &r);
    dc->ox = r.left;
    dc->oy = r.top;
    wnd_visible_region(w, client, &dc->clip);
    if (paint) {
        RECT p = *paint;

        p.left += dc->ox;
        p.right += dc->ox;
        p.top += dc->oy;
        p.bottom += dc->oy;
        rgn_intersect(&dc->clip, &p);
    }
    if (w->font)
        dc->font = w->font;
    return dc->h;
}

void dc_mark_dirty(PDC *dc)
{
    if ((dc->kind == DCK_CLIENT || dc->kind == DCK_WINDOW) && dc->wnd)
        wnd_toplevel(dc->wnd)->dirty = 1;
}

static void dc_free(PDC *dc)
{
    if (dc->bitmap)
        sel_count(dc->bitmap, -1);
    sel_count(dc->brush, -1);
    sel_count(dc->pen, -1);
    sel_count(dc->font, -1);
    sel_count(dc->palette, -1);
    port_handle_free(dc->h);
    free(dc);
}

HDC WINAPI GetDC(HWND hwnd)
{
    return dc_for_window(hwnd ? wnd_get(hwnd) : NULL, 1, NULL);
}

HDC WINAPI GetWindowDC(HWND hwnd)
{
    return dc_for_window(hwnd ? wnd_get(hwnd) : NULL, 0, NULL);
}

int WINAPI ReleaseDC(HWND hwnd, HDC hdc)
{
    PDC *dc = dc_get(hdc);

    (void)hwnd;
    if (!dc || dc->kind == DCK_MEMORY)
        return 0;
    dc_free(dc);
    return 1;
}

HDC WINAPI CreateDCA(LPCSTR driver, LPCSTR device, LPCSTR output, const void *init)
{
    (void)driver;
    (void)device;
    (void)output;
    (void)init;
    return dc_new(DCK_SCREEN)->h;
}

HDC WINAPI CreateCompatibleDC(HDC hdc)
{
    PDC *dc = dc_new(DCK_MEMORY);

    (void)hdc;
    dc->bitmap = g_defaultBitmap;
    dc->s = &bitmap_get(g_defaultBitmap)->s;
    rgn_set(&dc->clip, &(RECT){0, 0, 1, 1});
    return dc->h;
}

BOOL WINAPI DeleteDC(HDC hdc)
{
    PDC *dc = dc_get(hdc);

    if (!dc)
        return FALSE;
    dc_free(dc);
    return TRUE;
}

HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ obj)
{
    PDC *dc = dc_get(hdc);
    HGDIOBJ old = NULL;
    HGDIOBJ *slot = NULL;
    int t = port_handle_type(obj);

    if (!dc)
        return NULL;
    if (!t && (uintptr_t)obj >= 1 && (uintptr_t)obj <= 31)
        t = H_BRUSH;            /* a system colour brush */
    switch (t) {
    case H_BITMAP:
        {
            PBitmap *b = bitmap_get((HBITMAP)obj);

            if (dc->kind != DCK_MEMORY)
                return NULL;
            old = dc->bitmap;
            sel_count(obj, 1);
            dc->bitmap = (HBITMAP)obj;
            dc->s = &b->s;
            rgn_set(&dc->clip, &(RECT){0, 0, b->s.w, b->s.h});
            if (old)
                sel_count(old, -1);
            return old;
        }
    case H_BRUSH:
        slot = (HGDIOBJ *)&dc->brush;
        break;
    case H_PEN:
        slot = (HGDIOBJ *)&dc->pen;
        break;
    case H_FONT:
        slot = (HGDIOBJ *)&dc->font;
        break;
    case H_PALETTE:
        slot = (HGDIOBJ *)&dc->palette;
        break;
    default:
        return NULL;
    }
    old = *slot;
    sel_count(obj, 1);
    *slot = obj;
    sel_count(old, -1);
    return old;
}

int WINAPI GetDeviceCaps(HDC hdc, int index)
{
    (void)hdc;
    switch (index) {
    case BITSPIXEL:
        return 8;               /* the game wants a 256-colour display */
    case PLANES:
        return 1;
    case NUMCOLORS:
        return 20;
    case SIZEPALETTE:
        return 256;
    case NUMRESERVED:
        return 20;
    case COLORRES:
        return 18;
    case RASTERCAPS:
        return RC_PALETTE | 0x0001 | 0x0008 | 0x0080 | 0x0800;
    case LOGPIXELSX:
    case LOGPIXELSY:
        return 96;
    case HORZRES:
        return GetSystemMetrics(SM_CXSCREEN);
    case VERTRES:
        return GetSystemMetrics(SM_CYSCREEN);
    }
    return 0;
}

BOOL WINAPI GdiFlush(void)
{
    return TRUE;
}

/* ---- Drawing ---- */

/* the DC's clip rectangles crossed with r (DC coordinates), in surface coordinates */
static int clip_rects(PDC *dc, const RECT *r, Region *out)
{
    RECT s = {r->left + dc->ox, r->top + dc->oy, r->right + dc->ox, r->bottom + dc->oy};

    *out = dc->clip;
    rgn_intersect(out, &s);
    if (out->n)
        dc_mark_dirty(dc);
    return out->n;
}

static void fill_surface(Surface *s, const RECT *r, uint32_t xrgb)
{
    int y;

    if (s->bpp == 8) {
        int c = surf_nearest(s, xrgb);

        for (y = r->top; y < r->bottom; y++)
            memset(surf_row(s, y) + r->left, c, (size_t)(r->right - r->left));
    } else {
        for (y = r->top; y < r->bottom; y++) {
            uint32_t *p = (uint32_t *)surf_row(s, y) + r->left;
            int x;

            for (x = r->left; x < r->right; x++)
                *p++ = xrgb;
        }
    }
}

void gdi_fill(PDC *dc, const RECT *r, uint32_t xrgb)
{
    Region g;
    int i;

    if (!clip_rects(dc, r, &g))
        return;
    for (i = 0; i < g.n; i++)
        fill_surface(dc->s, &g.r[i], xrgb);
}

static int hatch_bit(int style, int x, int y)
{
    x &= 7;
    y &= 7;
    switch (style) {
    case HS_HORIZONTAL:
        return y == 4;
    case HS_VERTICAL:
        return x == 4;
    case HS_FDIAGONAL:
        return x == y;
    case HS_BDIAGONAL:
        return x == 7 - y;
    case HS_CROSS:
        return x == 4 || y == 4;
    default:
        return x == y || x == 7 - y;
    }
}

void gdi_fill_brush(PDC *dc, const RECT *r, HBRUSH brush)
{
    PBrush *b = brush_get(brush);
    Region g;
    int i, x, y;

    if (!b || b->style == BS_NULL)
        return;
    if (b->style == BS_SOLID) {
        gdi_fill(dc, r, color_xrgb(b->color));
        return;
    }
    if (!clip_rects(dc, r, &g))
        return;
    for (i = 0; i < g.n; i++)
        for (y = g.r[i].top; y < g.r[i].bottom; y++)
            for (x = g.r[i].left; x < g.r[i].right; x++) {
                uint32_t c = hatch_bit(b->hatch, x - dc->ox, y - dc->oy) ? color_xrgb(b->color)
                                                                          : color_xrgb(dc->bk);
                if (dc->s->bpp == 8)
                    surf_row(dc->s, y)[x] = (BYTE)surf_nearest(dc->s, c);
                else
                    ((uint32_t *)surf_row(dc->s, y))[x] = c;
            }
}

void gdi_hline(PDC *dc, int x0, int x1, int y, uint32_t c)
{
    RECT r = {x0, y, x1, y + 1};

    gdi_fill(dc, &r, c);
}

void gdi_vline(PDC *dc, int x, int y0, int y1, uint32_t c)
{
    RECT r = {x, y0, x + 1, y1};

    gdi_fill(dc, &r, c);
}

void gdi_pixel(PDC *dc, int x, int y, uint32_t c)
{
    RECT r = {x, y, x + 1, y + 1};

    gdi_fill(dc, &r, c);
}

void gdi_frame3d(PDC *dc, const RECT *r, uint32_t tl, uint32_t br)
{
    gdi_hline(dc, r->left, r->right - 1, r->top, tl);
    gdi_vline(dc, r->left, r->top, r->bottom - 1, tl);
    gdi_hline(dc, r->left, r->right, r->bottom - 1, br);
    gdi_vline(dc, r->right - 1, r->top, r->bottom, br);
}

static void invert_surface(Surface *s, const RECT *r)
{
    int x, y;

    for (y = r->top; y < r->bottom; y++) {
        BYTE *row = surf_row(s, y);

        for (x = r->left; x < r->right; x++) {
            if (s->bpp == 8)
                row[x] ^= 0xff;
            else
                ((uint32_t *)row)[x] ^= 0xffffff;
        }
    }
}

BOOL WINAPI PatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop)
{
    PDC *dc = dc_get(hdc);
    RECT r;
    Region g;
    int i;

    if (!dc)
        return FALSE;
    if (w < 0) {
        x += w;
        w = -w;
    }
    if (h < 0) {
        y += h;
        h = -h;
    }
    r = (RECT){x, y, x + w, y + h};
    switch (rop) {
    case PATCOPY:
        gdi_fill_brush(dc, &r, dc->brush);
        break;
    case BLACKNESS:
        gdi_fill(dc, &r, 0);
        break;
    case WHITENESS:
        gdi_fill(dc, &r, 0xffffff);
        break;
    case DSTINVERT:
        if (clip_rects(dc, &r, &g))
            for (i = 0; i < g.n; i++)
                invert_surface(dc->s, &g.r[i]);
        break;
    default:
        port_trace("PatBlt rop %08x", rop);
        gdi_fill_brush(dc, &r, dc->brush);
        break;
    }
    return TRUE;
}

int WINAPI FillRect(HDC hdc, const RECT *r, HBRUSH brush)
{
    PDC *dc = dc_get(hdc);

    if (dc)
        gdi_fill_brush(dc, r, brush);
    return 1;
}

int WINAPI FrameRect(HDC hdc, const RECT *r, HBRUSH brush)
{
    PDC *dc = dc_get(hdc);
    PBrush *b = brush_get(brush);

    if (dc && b)
        gdi_frame3d(dc, r, color_xrgb(b->color), color_xrgb(b->color));
    return 1;
}

BOOL WINAPI InvertRect(HDC hdc, const RECT *r)
{
    PDC *dc = dc_get(hdc);
    Region g;
    int i;

    if (dc && clip_rects(dc, r, &g))
        for (i = 0; i < g.n; i++)
            invert_surface(dc->s, &g.r[i]);
    return TRUE;
}

BOOL WINAPI DrawFocusRect(HDC hdc, const RECT *r)
{
    PDC *dc = dc_get(hdc);
    Region g;
    int i, x, y;

    if (!dc)
        return FALSE;
    /* a dotted outline, inverted: drawn twice it is gone */
    for (i = 0; i < 4; i++) {
        RECT e = i == 0 ? (RECT){r->left, r->top, r->right, r->top + 1}
               : i == 1 ? (RECT){r->left, r->bottom - 1, r->right, r->bottom}
               : i == 2 ? (RECT){r->left, r->top + 1, r->left + 1, r->bottom - 1}
                        : (RECT){r->right - 1, r->top + 1, r->right, r->bottom - 1};
        int k;

        if (!clip_rects(dc, &e, &g))
            continue;
        for (k = 0; k < g.n; k++)
            for (y = g.r[k].top; y < g.r[k].bottom; y++)
                for (x = g.r[k].left; x < g.r[k].right; x++)
                    if (((x - dc->ox) + (y - dc->oy)) & 1) {
                        BYTE *row = surf_row(dc->s, y);

                        if (dc->s->bpp == 8)
                            row[x] ^= 0xff;
                        else
                            ((uint32_t *)row)[x] ^= 0xffffff;
                    }
    }
    return TRUE;
}

BOOL WINAPI Rectangle(HDC hdc, int left, int top, int right, int bottom)
{
    PDC *dc = dc_get(hdc);
    PPen *p;
    RECT r = {left, top, right, bottom};

    if (!dc)
        return FALSE;
    p = OBJ(H_PEN, PPen, dc->pen);
    if (p && p->style != PS_NULL) {
        RECT in = {left + 1, top + 1, right - 1, bottom - 1};

        gdi_fill_brush(dc, &in, dc->brush);
        gdi_frame3d(dc, &r, color_xrgb(p->color), color_xrgb(p->color));
    } else
        gdi_fill_brush(dc, &r, dc->brush);
    return TRUE;
}

BOOL WINAPI MoveToEx(HDC hdc, int x, int y, LPPOINT old)
{
    PDC *dc = dc_get(hdc);

    if (!dc)
        return FALSE;
    if (old)
        *old = dc->cur;
    dc->cur.x = x;
    dc->cur.y = y;
    return TRUE;
}

BOOL WINAPI LineTo(HDC hdc, int x, int y)
{
    PDC *dc = dc_get(hdc);
    PPen *p;
    uint32_t c;

    if (!dc)
        return FALSE;
    p = OBJ(H_PEN, PPen, dc->pen);
    c = p ? color_xrgb(p->color) : 0;
    if (!p || p->style != PS_NULL) {
        if (y == dc->cur.y)
            gdi_hline(dc, x < dc->cur.x ? x + 1 : dc->cur.x, x < dc->cur.x ? dc->cur.x + 1 : x, y, c);
        else if (x == dc->cur.x)
            gdi_vline(dc, x, y < dc->cur.y ? y + 1 : dc->cur.y, y < dc->cur.y ? dc->cur.y + 1 : y, c);
        else {
            int x0 = dc->cur.x, y0 = dc->cur.y, dx = abs(x - x0), dy = -abs(y - y0);
            int sx = x0 < x ? 1 : -1, sy = y0 < y ? 1 : -1, err = dx + dy;

            while (x0 != x || y0 != y) {
                int e2 = 2 * err;

                gdi_pixel(dc, x0, y0, c);
                if (e2 >= dy) {
                    err += dy;
                    x0 += sx;
                }
                if (e2 <= dx) {
                    err += dx;
                    y0 += sy;
                }
            }
        }
    }
    dc->cur.x = x;
    dc->cur.y = y;
    return TRUE;
}

COLORREF WINAPI SetPixel(HDC hdc, int x, int y, COLORREF c)
{
    PDC *dc = dc_get(hdc);

    if (dc)
        gdi_pixel(dc, x, y, color_xrgb(c));
    return c;
}

COLORREF WINAPI GetPixel(HDC hdc, int x, int y)
{
    PDC *dc = dc_get(hdc);
    uint32_t c;

    if (!dc)
        return 0xffffffff;
    x += dc->ox;
    y += dc->oy;
    if (x < 0 || y < 0 || x >= dc->s->w || y >= dc->s->h)
        return 0xffffffff;
    c = surf_get(dc->s, x, y);
    return RGB(c >> 16, (c >> 8) & 0xff, c & 0xff);
}

/* ---- Blits ---- */

/* Copies src (sx, sy, sw x sh, surface coordinates) scaled to dst rectangle d
   (surface coordinates), within the clip rectangle c. */
static void blit_scaled(Surface *dst, const RECT *d, const RECT *c, const Surface *src, int sx, int sy, int sw,
                        int sh)
{
    int dw = d->right - d->left, dh = d->bottom - d->top;
    int x, y;
    uint32_t lut32[256];
    BYTE lut8[256];
    int identity = 0;

    if (dw <= 0 || dh <= 0 || sw == 0 || sh == 0)
        return;
    if (src->bpp == 8 && dst->bpp == 32) {
        for (x = 0; x < 256; x++)
            lut32[x] = ((uint32_t)src->colors[x].rgbRed << 16) | ((uint32_t)src->colors[x].rgbGreen << 8) |
                       src->colors[x].rgbBlue;
    } else if (src->bpp == 8 && dst->bpp == 8) {
        identity = !memcmp(src->colors, dst->colors, sizeof src->colors);
        if (!identity)
            for (x = 0; x < 256; x++)
                lut8[x] = (BYTE)surf_nearest(dst, ((uint32_t)src->colors[x].rgbRed << 16) |
                                                      ((uint32_t)src->colors[x].rgbGreen << 8) |
                                                      src->colors[x].rgbBlue);
    }
    for (y = c->top; y < c->bottom; y++) {
        int yy = sy + (int)((long long)(y - d->top) * sh / dh);
        BYTE *drow;
        const BYTE *srow;

        if (sh < 0)
            yy = sy + sh + (int)((long long)(y - d->top) * -sh / dh);
        if (yy < 0 || yy >= src->h)
            continue;
        drow = surf_row(dst, y);
        srow = surf_row(src, yy);
        /* the common case: same size, same depth */
        if (sw == dw && src->bpp == dst->bpp && (dst->bpp == 32 || identity)) {
            int x0 = c->left, x1 = c->right, so = sx + (x0 - d->left);

            if (so < 0) {
                x0 -= so;
                so = 0;
            }
            if (so + (x1 - x0) > src->w)
                x1 = x0 + (src->w - so);
            if (x1 > x0)
                memcpy(drow + x0 * (dst->bpp / 8), srow + so * (src->bpp / 8), (size_t)(x1 - x0) * (dst->bpp / 8));
            continue;
        }
        for (x = c->left; x < c->right; x++) {
            int xx = sx + (int)((long long)(x - d->left) * sw / dw);

            if (xx < 0 || xx >= src->w)
                continue;
            if (dst->bpp == 32) {
                ((uint32_t *)drow)[x] = src->bpp == 8 ? lut32[srow[xx]] : any_get(src, xx, yy);
            } else if (src->bpp == 8)
                drow[x] = identity ? srow[xx] : lut8[srow[xx]];
            else
                drow[x] = (BYTE)surf_nearest(dst, any_get(src, xx, yy));
        }
    }
}

static BOOL do_blit(PDC *dc, int x, int y, int w, int h, const Surface *src, int sx, int sy, int sw, int sh,
                    DWORD rop)
{
    RECT d;
    Region g;
    int i;

    if (rop != SRCCOPY)
        port_trace("blit rop %08x", rop);
    if (w < 0 || h < 0) {
        if (w < 0) {
            x += w;
            w = -w;
            sx += sw;
            sw = -sw;
        }
        if (h < 0) {
            y += h;
            h = -h;
            sy += sh;
            sh = -sh;
        }
    }
    d = (RECT){x, y, x + w, y + h};
    if (!clip_rects(dc, &d, &g))
        return TRUE;
    OffsetRect(&d, dc->ox, dc->oy);
    for (i = 0; i < g.n; i++)
        blit_scaled(dc->s, &d, &g.r[i], src, sx, sy, sw, sh);
    return TRUE;
}

BOOL WINAPI BitBlt(HDC dst, int x, int y, int w, int h, HDC src, int sx, int sy, DWORD rop)
{
    PDC *d = dc_get(dst), *s = dc_get(src);

    if (!d)
        return FALSE;
    if (!s || rop == PATCOPY || rop == BLACKNESS || rop == WHITENESS || rop == DSTINVERT)
        return PatBlt(dst, x, y, w, h, rop);
    return do_blit(d, x, y, w, h, s->s, sx + s->ox, sy + s->oy, w, h, rop);
}

BOOL WINAPI StretchBlt(HDC dst, int x, int y, int w, int h, HDC src, int sx, int sy, int sw, int sh, DWORD rop)
{
    PDC *d = dc_get(dst), *s = dc_get(src);

    if (!d || !s)
        return FALSE;
    return do_blit(d, x, y, w, h, s->s, sx + s->ox, sy + s->oy, sw, sh, rop);
}

int WINAPI StretchDIBits(HDC hdc, int x, int y, int w, int h, int sx, int sy, int sw, int sh, const void *bits,
                         const BITMAPINFO *bmi, UINT usage, DWORD rop)
{
    PDC *dc = dc_get(hdc);
    Surface src;

    if (!dc || !dib_view(&src, bmi, bits, usage, dc->palette))
        return 0;
    /* a bottom-up DIB's source y counts from its bottom row */
    if (src.bottomUp)
        sy = src.h - sy - sh;
    do_blit(dc, x, y, w, h, &src, sx, sy, sw, sh, rop);
    return sh;
}

int WINAPI SetDIBitsToDevice(HDC hdc, int x, int y, DWORD w, DWORD h, int sx, int sy, UINT start, UINT lines,
                             const void *bits, const BITMAPINFO *bmi, UINT usage)
{
    (void)start;
    return StretchDIBits(hdc, x, y, (int)w, (int)h, sx, sy, (int)w, (int)h, bits, bmi, usage, SRCCOPY) ? (int)lines
                                                                                                         : 0;
}

int WINAPI GetDIBits(HDC hdc, HBITMAP hbm, UINT start, UINT lines, LPVOID bits, LPBITMAPINFO bmi, UINT usage)
{
    PBitmap *b = bitmap_get(hbm);
    BITMAPINFOHEADER *bh = &bmi->bmiHeader;
    UINT y;
    int x;

    (void)hdc;
    (void)usage;
    if (!b)
        return 0;
    if (!bits) {
        bh->biWidth = b->s.w;
        bh->biHeight = b->s.h;
        bh->biPlanes = 1;
        bh->biBitCount = 24;
        return 1;
    }
    for (y = start; y < start + lines && (int)y < b->s.h; y++) {
        int pitch = ((bh->biWidth * bh->biBitCount + 31) / 32) * 4;
        BYTE *o = (BYTE *)bits + (size_t)(y - start) * pitch;
        int sy = bh->biHeight > 0 ? b->s.h - 1 - (int)y : (int)y;

        for (x = 0; x < b->s.w && x < bh->biWidth; x++) {
            uint32_t c = surf_get(&b->s, x, sy);

            if (bh->biBitCount == 24) {
                o[x * 3] = c & 0xff;
                o[x * 3 + 1] = (c >> 8) & 0xff;
                o[x * 3 + 2] = (c >> 16) & 0xff;
            } else if (bh->biBitCount == 32)
                ((uint32_t *)o)[x] = c;
        }
    }
    return (int)lines;
}

UINT WINAPI SetDIBColorTable(HDC hdc, UINT start, UINT n, const RGBQUAD *colors)
{
    PDC *dc = dc_get(hdc);
    PBitmap *b = dc ? bitmap_get(dc->bitmap) : NULL;

    if (!b || b->s.bpp != 8 || start >= 256)
        return 0;
    if (start + n > 256)
        n = 256 - start;
    memcpy(b->s.colors + start, colors, n * sizeof(RGBQUAD));
    return n;
}

/* ---- Text state ---- */

COLORREF WINAPI SetTextColor(HDC hdc, COLORREF c)
{
    PDC *dc = dc_get(hdc);
    COLORREF old;

    if (!dc)
        return 0;
    old = dc->text;
    dc->text = c & 0xffffff;
    return old;
}

COLORREF WINAPI GetTextColor(HDC hdc)
{
    PDC *dc = dc_get(hdc);

    return dc ? dc->text : 0;
}

COLORREF WINAPI SetBkColor(HDC hdc, COLORREF c)
{
    PDC *dc = dc_get(hdc);
    COLORREF old;

    if (!dc)
        return 0;
    old = dc->bk;
    dc->bk = c & 0xffffff;
    return old;
}

COLORREF WINAPI GetBkColor(HDC hdc)
{
    PDC *dc = dc_get(hdc);

    return dc ? dc->bk : 0;
}

int WINAPI SetBkMode(HDC hdc, int mode)
{
    PDC *dc = dc_get(hdc);
    int old;

    if (!dc)
        return 0;
    old = dc->bkMode;
    dc->bkMode = mode;
    return old;
}

int WINAPI SetStretchBltMode(HDC hdc, int mode)
{
    (void)hdc;
    (void)mode;
    return 1;
}

/* ---- Palettes ---- */

static const PALETTEENTRY g_static[20] = {
    {0, 0, 0, 0}, {0x80, 0, 0, 0}, {0, 0x80, 0, 0}, {0x80, 0x80, 0, 0}, {0, 0, 0x80, 0}, {0x80, 0, 0x80, 0},
    {0, 0x80, 0x80, 0}, {0xc0, 0xc0, 0xc0, 0}, {0xc0, 0xdc, 0xc0, 0}, {0xa6, 0xca, 0xf0, 0},
    {0xff, 0xfb, 0xf0, 0}, {0xa0, 0xa0, 0xa4, 0}, {0x80, 0x80, 0x80, 0}, {0xff, 0, 0, 0}, {0, 0xff, 0, 0},
    {0xff, 0xff, 0, 0}, {0, 0, 0xff, 0}, {0xff, 0, 0xff, 0}, {0, 0xff, 0xff, 0}, {0xff, 0xff, 0xff, 0},
};
static PALETTEENTRY g_system[256];      /* the "hardware" palette last realised */

HPALETTE WINAPI SelectPalette(HDC hdc, HPALETTE pal, BOOL background)
{
    PDC *dc = dc_get(hdc);
    HPALETTE old;

    (void)background;
    if (!dc || !palette_get(pal))
        return NULL;
    old = dc->palette;
    sel_count(pal, 1);
    dc->palette = pal;
    sel_count(old, -1);
    return old;
}

UINT WINAPI RealizePalette(HDC hdc)
{
    PDC *dc = dc_get(hdc);
    PPalette *p = dc ? palette_get(dc->palette) : NULL;
    int i;

    if (!p || p->stock)
        return 0;
    /* what a 256-colour display would now show, for GetSystemPaletteEntries */
    for (i = 10; i < 246 && i < p->n; i++)
        g_system[i] = p->e[i];
    return 0;
}

BOOL WINAPI UnrealizeObject(HGDIOBJ obj)
{
    (void)obj;
    return TRUE;
}

UINT WINAPI GetPaletteEntries(HPALETTE pal, UINT start, UINT n, LPPALETTEENTRY e)
{
    PPalette *p = palette_get(pal);

    if (!p)
        return 0;
    if (!e)
        return (UINT)p->n;
    if (start >= (UINT)p->n)
        return 0;
    if (start + n > (UINT)p->n)
        n = (UINT)p->n - start;
    memcpy(e, p->e + start, n * sizeof *e);
    return n;
}

UINT WINAPI SetPaletteEntries(HPALETTE pal, UINT start, UINT n, const PALETTEENTRY *e)
{
    PPalette *p = palette_get(pal);

    if (!p || start >= (UINT)p->n)
        return 0;
    if (start + n > (UINT)p->n)
        n = (UINT)p->n - start;
    memcpy(p->e + start, e, n * sizeof *e);
    return n;
}

BOOL WINAPI AnimatePalette(HPALETTE pal, UINT start, UINT n, const PALETTEENTRY *e)
{
    return SetPaletteEntries(pal, start, n, e) != 0;
}

UINT WINAPI GetSystemPaletteEntries(HDC hdc, UINT start, UINT n, LPPALETTEENTRY e)
{
    UINT i;

    (void)hdc;
    for (i = 0; i < n && start + i < 256; i++)
        e[i] = g_system[start + i];
    return i;
}

UINT WINAPI GetNearestPaletteIndex(HPALETTE pal, COLORREF c)
{
    PPalette *p = palette_get(pal);
    int i, best = 0, bestd = 1 << 30;

    if (!p)
        return 0;
    for (i = 0; i < p->n; i++) {
        int dr = p->e[i].peRed - (int)GetRValue(c), dg = p->e[i].peGreen - (int)GetGValue(c);
        int db = p->e[i].peBlue - (int)GetBValue(c), d = dr * dr + dg * dg + db * db;

        if (d < bestd) {
            bestd = d;
            best = i;
        }
    }
    return (UINT)best;
}

int WINAPI UpdateColors(HDC hdc)
{
    (void)hdc;
    return 1;
}

/* ---- Set-up ---- */

void gdi_init(void)
{
    static const COLORREF brushes[5] = {0xffffff, 0xc0c0c0, 0x808080, 0x404040, 0};
    LOGPALETTE *lp;
    PBitmap *b;
    int i;

    memset(&g_screenSurf, 0, sizeof g_screenSurf);
    g_screenSurf.w = g_screenSurf.h = 1;
    g_screenSurf.bpp = 32;
    g_screenSurf.pitch = 4;
    g_screenSurf.bits = calloc(1, 4);
    for (i = 0; i < 5; i++) {
        g_stock[i] = CreateSolidBrush(brushes[i]);
        brush_get((HBRUSH)g_stock[i])->stock = 1;
    }
    g_stock[NULL_BRUSH] = CreateSolidBrush(0);
    brush_get((HBRUSH)g_stock[NULL_BRUSH])->style = BS_NULL;
    brush_get((HBRUSH)g_stock[NULL_BRUSH])->stock = 1;
    g_stock[WHITE_PEN] = CreatePen(PS_SOLID, 1, 0xffffff);
    g_stock[BLACK_PEN] = CreatePen(PS_SOLID, 1, 0);
    g_stock[NULL_PEN] = CreatePen(PS_NULL, 1, 0);
    for (i = WHITE_PEN; i <= NULL_PEN; i++)
        OBJ(H_PEN, PPen, g_stock[i])->stock = 1;
    lp = calloc(1, sizeof(LOGPALETTE) + 20 * sizeof(PALETTEENTRY));
    lp->palVersion = 0x300;
    lp->palNumEntries = 20;
    memcpy(lp->palPalEntry, g_static, sizeof g_static);
    g_stock[DEFAULT_PALETTE] = CreatePalette(lp);
    palette_get((HPALETTE)g_stock[DEFAULT_PALETTE])->stock = 1;
    free(lp);
    memcpy(g_system, g_static, 10 * sizeof(PALETTEENTRY));
    memcpy(g_system + 246, g_static + 10, 10 * sizeof(PALETTEENTRY));
    g_defaultBitmap = new_bitmap(1, 1, 32, 0, NULL, 0, &b);
}

/* the fonts come from font.c once it has run */
void gdi_set_stock_fonts(HFONT system, HFONT fixed)
{
    int i;

    for (i = OEM_FIXED_FONT; i <= DEFAULT_GUI_FONT; i++)
        if (i != DEFAULT_PALETTE)
            g_stock[i] = (i == OEM_FIXED_FONT || i == ANSI_FIXED_FONT || i == SYSTEM_FIXED_FONT) ? fixed : system;
}

HFONT gdi_new_font(PFont **out)
{
    uintptr_t h;
    PFont *f = obj_new(H_FONT, sizeof(PFont), &h);

    *out = f;
    return (HFONT)h;
}
