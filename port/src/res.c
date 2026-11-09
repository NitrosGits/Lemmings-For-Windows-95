/*
 * Resources from the generated tables (tools/port/rc2c.py): strings, raw
 * resources (the level tree's bitmap), cursors and icons. The game's .cur and
 * .ico files become SDL colour cursors and 32-bit images.
 */
#include "port_internal.h"
#include "port_res.h"

static int name_matches(const char *rname, int rid, LPCSTR name)
{
    if (IS_INTRESOURCE(name))
        return rid == (int)(uintptr_t)name;
    if (name[0] == '#')
        return rid == atoi(name + 1);
    return rname && !strcasecmp(rname, name);
}

const void *res_find_file(const char *type, const char *name, unsigned *size)
{
    const PortResFile *f;

    for (f = port_res_files; f->data; f++) {
        if (!strcmp(f->type, type) && name_matches(f->name, f->id, name)) {
            if (size)
                *size = f->size;
            return f->data;
        }
    }
    return NULL;
}

int WINAPI LoadStringA(HINSTANCE inst, UINT id, LPSTR buf, int size)
{
    const PortResString *s;

    (void)inst;
    if (size <= 0)
        return 0;
    for (s = port_res_strings; s->text; s++) {
        if (s->id == id) {
            port_strlcpy(buf, s->text, (size_t)size);
            return (int)strlen(buf);
        }
    }
    buf[0] = 0;
    port_last_error = ERROR_RESOURCE_NAME_NOT_FOUND;
    return 0;
}

HRSRC WINAPI FindResourceA(HMODULE module, LPCSTR name, LPCSTR type)
{
    const PortResFile *f;
    const char *t = type == RT_BITMAP ? "BITMAP" : type == RT_CURSOR || type == RT_GROUP_CURSOR ? "CURSOR"
                  : type == RT_ICON || type == RT_GROUP_ICON ? "ICON" : NULL;

    (void)module;
    if (!t)
        return NULL;
    for (f = port_res_files; f->data; f++)
        if (!strcmp(f->type, t) && name_matches(f->name, f->id, name))
            return (HRSRC)f;
    return NULL;
}

HGLOBAL WINAPI LoadResource(HMODULE module, HRSRC res)
{
    const PortResFile *f = (const PortResFile *)res;

    (void)module;
    if (!f)
        return NULL;
    /* a BITMAP resource is the .bmp file without its BITMAPFILEHEADER */
    if (!strcmp(f->type, "BITMAP"))
        return (HGLOBAL)(f->data + sizeof(BITMAPFILEHEADER));
    return (HGLOBAL)f->data;
}

LPVOID WINAPI LockResource(HGLOBAL res)
{
    return res;
}

BOOL WINAPI FreeResource(HGLOBAL res)
{
    (void)res;
    return FALSE;
}

DWORD WINAPI SizeofResource(HMODULE module, HRSRC res)
{
    const PortResFile *f = (const PortResFile *)res;

    (void)module;
    if (!f)
        return 0;
    return !strcmp(f->type, "BITMAP") ? f->size - (DWORD)sizeof(BITMAPFILEHEADER) : f->size;
}

/* ---- Cursors and icons ---- */

typedef struct Image {          /* 0xAARRGGBB, rows top down */
    int w, h;
    uint32_t *px;
} Image;

typedef struct PCursor {
    void *cursor;               /* the backend's */
    Image *image;               /* icons and resource cursors */
    int hx, hy;
    char name[64];
    intptr_t id;
} PCursor;

static PCursor *g_cache[64];
static int g_ncache;

static uint16_t rd16(const BYTE *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint32_t rd32(const BYTE *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* an image of a .cur/.ico file (XOR bitmap with colour table, then AND mask) as ARGB */
static Image *icon_image(const BYTE *file, unsigned size, int *hx, int *hy)
{
    int count = rd16(file + 4), best = -1, bestScore = -1, i;
    const BYTE *e, *img;
    int w, h, bpp, ncolors, x, y, xorPitch, andPitch;
    const BYTE *colors, *xorBits, *andBits;
    Image *s;

    for (i = 0; i < count; i++) {
        e = file + 6 + i * 16;
        w = e[0] ? e[0] : 256;
        {
            int score = (w == 32 ? 1000 : w) + (rd16(file + 2) == 1 ? rd16(e + 6) : 0);

            if (score > bestScore) {
                bestScore = score;
                best = i;
            }
        }
    }
    if (best < 0)
        return NULL;
    e = file + 6 + best * 16;
    if (rd32(e + 12) + 40 > size)
        return NULL;
    img = file + rd32(e + 12);
    if (hx)
        *hx = rd16(file + 2) == 2 ? rd16(e + 4) : 0;
    if (hy)
        *hy = rd16(file + 2) == 2 ? rd16(e + 6) : 0;
    w = (int)rd32(img + 4);
    h = (int)rd32(img + 8) / 2;
    bpp = rd16(img + 14);
    ncolors = bpp <= 8 ? (rd32(img + 32) ? (int)rd32(img + 32) : 1 << bpp) : 0;
    colors = img + rd32(img);
    xorBits = colors + ncolors * 4;
    xorPitch = ((w * bpp + 31) / 32) * 4;
    andBits = xorBits + xorPitch * h;
    andPitch = ((w + 31) / 32) * 4;
    s = malloc(sizeof *s);
    s->w = w;
    s->h = h;
    s->px = calloc((size_t)w * (size_t)h, 4);
    for (y = 0; y < h; y++) {
        const BYTE *xr = xorBits + (size_t)(h - 1 - y) * xorPitch;
        const BYTE *ar = andBits + (size_t)(h - 1 - y) * andPitch;
        uint32_t *o = s->px + (size_t)y * w;

        for (x = 0; x < w; x++) {
            int mask = (ar[x >> 3] >> (7 - (x & 7))) & 1;
            uint32_t c;
            int idx;

            switch (bpp) {
            case 1:
                idx = (xr[x >> 3] >> (7 - (x & 7))) & 1;
                break;
            case 4:
                idx = (xr[x >> 1] >> (x & 1 ? 0 : 4)) & 15;
                break;
            case 8:
                idx = xr[x];
                break;
            default:
                idx = -1;
                break;
            }
            if (idx >= 0)
                c = ((uint32_t)colors[idx * 4 + 2] << 16) | ((uint32_t)colors[idx * 4 + 1] << 8) | colors[idx * 4];
            else if (bpp == 24)
                c = ((uint32_t)xr[x * 3 + 2] << 16) | ((uint32_t)xr[x * 3 + 1] << 8) | xr[x * 3];
            else
                c = rd32(xr + x * 4) & 0xffffff;
            if (!mask)
                o[x] = 0xff000000 | c;
            else if (c)
                o[x] = 0xff000000;      /* an inverted pixel: shown black */
            else
                o[x] = 0;
        }
    }
    return s;
}

static PCursor *cursor_get(HCURSOR h)
{
    return (PCursor *)port_handle_get(h, H_CURSOR);
}

static HCURSOR g_cacheHandles[64];

static HCURSOR remember(PCursor *c)
{
    HCURSOR h = (HCURSOR)port_handle_new(H_CURSOR, c);

    if (g_ncache < 64) {
        g_cacheHandles[g_ncache] = h;
        g_cache[g_ncache++] = c;
    }
    return h;
}

static HCURSOR lookup(const char *kind, LPCSTR name)
{
    int i;
    intptr_t id = IS_INTRESOURCE(name) ? (intptr_t)(uintptr_t)name : -1;

    for (i = 0; i < g_ncache; i++) {
        if (strncmp(g_cache[i]->name, kind, 1))
            continue;
        if (id >= 0 ? g_cache[i]->id == id : !strcasecmp(g_cache[i]->name + 1, name))
            return g_cacheHandles[i];
    }
    return NULL;
}

HCURSOR cursor_system(LPCSTR id)
{
    PCursor *c;
    HCURSOR h = lookup("S", id);

    if (h)
        return h;
    c = calloc(1, sizeof *c);
    c->cursor = plat_cursor_system((int)(uintptr_t)id);
    c->id = (intptr_t)(uintptr_t)id;
    strcpy(c->name, "S");
    return remember(c);
}

HCURSOR WINAPI LoadCursorA(HINSTANCE inst, LPCSTR name)
{
    const BYTE *f;
    unsigned size;
    PCursor *c;
    HCURSOR h;

    if (!inst || (IS_INTRESOURCE(name) && (uintptr_t)name >= 32512))
        return cursor_system(name);
    if ((h = lookup("C", name)) != NULL)
        return h;
    f = res_find_file("CURSOR", name, &size);
    if (!f)
        return NULL;
    c = calloc(1, sizeof *c);
    c->image = icon_image(f, size, &c->hx, &c->hy);
    if (c->image)
        c->cursor = plat_cursor_image(c->image->px, c->image->w, c->image->h, c->hx, c->hy);
    c->id = IS_INTRESOURCE(name) ? (intptr_t)(uintptr_t)name : -1;
    snprintf(c->name, sizeof c->name, "C%s", IS_INTRESOURCE(name) ? "" : name);
    return remember(c);
}

HICON WINAPI LoadIconA(HINSTANCE inst, LPCSTR name)
{
    const BYTE *f;
    unsigned size;
    PCursor *c;
    HICON h;

    if (!inst || IS_INTRESOURCE(name))
        name = "LEMMINGS";      /* the system's icons: the game's own will do */
    if ((h = lookup("I", name)) != NULL)
        return h;
    f = res_find_file("ICON", name, &size);
    if (!f)
        return NULL;
    c = calloc(1, sizeof *c);
    c->image = icon_image(f, size, NULL, NULL);
    c->id = -1;
    snprintf(c->name, sizeof c->name, "I%s", name);
    return remember(c);
}

BOOL WINAPI DestroyCursor(HCURSOR c)
{
    (void)c;
    return TRUE;                /* shared, like Windows' resource cursors */
}

BOOL WINAPI DestroyIcon(HICON i)
{
    (void)i;
    return TRUE;
}

void cursor_apply(HCURSOR h)
{
    PCursor *c = cursor_get(h);

    if (!h) {
        plat_cursor_set(NULL);
        return;
    }
    if (c && c->cursor)
        plat_cursor_set(c->cursor);
}

int cursor_hotspot_size(HCURSOR h, int *w, int *hh)
{
    PCursor *c = cursor_get(h);

    if (!c || !c->image)
        return 0;
    *w = c->image->w;
    *hh = c->image->h;
    return 1;
}

void icon_draw(PDC *dc, HICON icon, int x, int y)
{
    PCursor *c = cursor_get(icon);
    int i, j, k;

    if (!c || !c->image)
        return;
    for (j = 0; j < c->image->h; j++) {
        const uint32_t *row = c->image->px + (size_t)j * c->image->w;

        for (i = 0; i < c->image->w; i++) {
            int px = x + i + dc->ox, py = y + j + dc->oy;

            if (!(row[i] >> 24))
                continue;
            for (k = 0; k < dc->clip.n; k++) {
                const RECT *r = &dc->clip.r[k];

                if (px >= r->left && px < r->right && py >= r->top && py < r->bottom) {
                    if (dc->s->bpp == 32)
                        ((uint32_t *)surf_row(dc->s, py))[px] = row[i] & 0xffffff;
                    else
                        surf_row(dc->s, py)[px] = (BYTE)surf_nearest(dc->s, row[i] & 0xffffff);
                    break;
                }
            }
        }
    }
    dc_mark_dirty(dc);
}

BOOL WINAPI DrawIcon(HDC hdc, int x, int y, HICON icon)
{
    PDC *dc = dc_get(hdc);

    if (dc)
        icon_draw(dc, icon, x, y);
    return dc != NULL;
}
