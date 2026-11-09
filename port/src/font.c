/*
 * Text: one proportional bitmap font, drawn for this project, about the size
 * of MS Sans Serif 8 (8-pixel capitals, 13-pixel cell). Larger requests get
 * a taller cell (the game's metrics stay right) and from a 24-pixel cell
 * doubled pixels; bold is drawn twice. Characters are Windows-1252.
 *
 * Each glyph: its width and up to 10 rows, the first 8 above the baseline
 * (capitals fill all 8, small letters the last 6), then 2 for descenders.
 */
#include "port_internal.h"

HFONT gdi_new_font(PFont **out);
void gdi_set_stock_fonts(HFONT system, HFONT fixed);

static const struct {
    unsigned char ch, w;
    const char *rows;
} g_glyphs[] = {
    {' ', 2, ""},
    {'!', 1, "# # # # # . # . "},
    {'"', 3, "#.# #.# #.#"},
    {'#', 6, "..#.#. ..#.#. ###### .#.#.. ###### .#.#.. .#.#.. ......"},
    {'$', 5, "..#.. .###. #.#.# #.#.. .###. ..#.# #.#.# .###. ..#.."},
    {'%', 7, ".#...#. #.#..#. .#..#.. ...#... ..#.... .#..#.. .#.#.#. #...#.."},
    {'&', 5, ".#... #.#.. #.#.. .#... #.#.# #..#. #..#. .##.#"},
    {'\'', 1, "# # #"},
    {'(', 3, "..# .#. .#. #.. #.. #.. #.. .#. .#. ..#"},
    {')', 3, "#.. .#. .#. ..# ..# ..# ..# .#. .#. #.."},
    {'*', 5, "..#.. #.#.# .###. #.#.# ..#.."},
    {'+', 5, "..... ..... ..#.. ..#.. ##### ..#.. ..#.. ....."},
    {',', 2, ".. .. .. .. .. .. .# .# #."},
    {'-', 3, "... ... ... ... ### ... ... ..."},
    {'.', 1, ". . . . . . . #"},
    {'/', 3, "..# ..# .#. .#. .#. .#. #.. #.."},
    {'0', 5, ".###. #...# #...# #...# #...# #...# #...# .###."},
    {'1', 3, ".#. ##. .#. .#. .#. .#. .#. .#."},
    {'2', 5, ".###. #...# ....# ...#. ..#.. .#... #.... #####"},
    {'3', 5, ".###. #...# ....# ..##. ....# ....# #...# .###."},
    {'4', 5, "...#. ..##. .#.#. #..#. ##### ...#. ...#. ...#."},
    {'5', 5, "##### #.... #.... ####. ....# ....# #...# .###."},
    {'6', 5, ".###. #...# #.... ####. #...# #...# #...# .###."},
    {'7', 5, "##### ....# ...#. ...#. ..#.. ..#.. .#... .#..."},
    {'8', 5, ".###. #...# #...# .###. #...# #...# #...# .###."},
    {'9', 5, ".###. #...# #...# #...# .#### ....# #...# .###."},
    {':', 1, ". . # . . . . #"},
    {';', 2, ".. .. .# .. .. .. .# .# #."},
    {'<', 4, ".... ...# ..#. .#.. #... .#.. ..#. ...#"},
    {'=', 4, ".... .... .... #### .... #### .... ...."},
    {'>', 4, ".... #... .#.. ..#. ...# ..#. .#.. #..."},
    {'?', 4, ".##. #..# ...# ..#. .#.. .#.. .... .#.."},
    {'@', 8, ".######. #......# #..##..# #.#..#.# #.#..#.# #..####. #....... .######."},
    {'A', 5, "..#.. ..#.. .#.#. .#.#. #...# ##### #...# #...#"},
    {'B', 5, "####. #...# #...# ####. #...# #...# #...# ####."},
    {'C', 5, ".###. #...# #.... #.... #.... #.... #...# .###."},
    {'D', 5, "###.. #..#. #...# #...# #...# #...# #..#. ###.."},
    {'E', 4, "#### #... #... ###. #... #... #... ####"},
    {'F', 4, "#### #... #... ###. #... #... #... #..."},
    {'G', 5, ".###. #...# #.... #.... #..## #...# #...# .###."},
    {'H', 5, "#...# #...# #...# ##### #...# #...# #...# #...#"},
    {'I', 1, "# # # # # # # #"},
    {'J', 4, "...# ...# ...# ...# ...# #..# #..# .##."},
    {'K', 5, "#...# #..#. #.#.. ##... #.#.. #..#. #..#. #...#"},
    {'L', 4, "#... #... #... #... #... #... #... ####"},
    {'M', 7, "#.....# ##...## #.#.#.# #.#.#.# #..#..# #..#..# #.....# #.....#"},
    {'N', 5, "#...# ##..# ##..# #.#.# #.#.# #..## #..## #...#"},
    {'O', 6, ".####. #....# #....# #....# #....# #....# #....# .####."},
    {'P', 5, "####. #...# #...# #...# ####. #.... #.... #...."},
    {'Q', 6, ".####. #....# #....# #....# #....# #..#.# #...#. .###.#"},
    {'R', 5, "####. #...# #...# #...# ####. #..#. #...# #...#"},
    {'S', 5, ".###. #...# #.... .###. ....# ....# #...# .###."},
    {'T', 5, "##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#.. ..#.."},
    {'U', 5, "#...# #...# #...# #...# #...# #...# #...# .###."},
    {'V', 5, "#...# #...# #...# .#.#. .#.#. .#.#. ..#.. ..#.."},
    {'W', 9, "#...#...# #...#...# #...#...# .#.#.#.#. .#.#.#.#. .#.#.#.#. ..#...#.. ..#...#.."},
    {'X', 5, "#...# #...# .#.#. ..#.. ..#.. .#.#. #...# #...#"},
    {'Y', 5, "#...# #...# .#.#. .#.#. ..#.. ..#.. ..#.. ..#.."},
    {'Z', 5, "##### ....# ...#. ..#.. ..#.. .#... #.... #####"},
    {'[', 2, "## #. #. #. #. #. #. #. #. ##"},
    {'\\', 3, "#.. #.. .#. .#. .#. .#. ..# ..#"},
    {']', 2, "## .# .# .# .# .# .# .# .# ##"},
    {'^', 5, "..#.. .#.#. #...#"},
    {'_', 5, "..... ..... ..... ..... ..... ..... ..... ..... #####"},
    {'`', 2, "#. .#"},
    {'a', 4, ".... .... .##. ...# .### #..# #..# .###"},
    {'b', 4, "#... #... ###. #..# #..# #..# #..# ###."},
    {'c', 4, ".... .... .##. #..# #... #... #..# .##."},
    {'d', 4, "...# ...# .### #..# #..# #..# #..# .###"},
    {'e', 4, ".... .... .##. #..# #### #... #..# .##."},
    {'f', 3, ".## #.. ### #.. #.. #.. #.. #.."},
    {'g', 4, ".... .... .### #..# #..# #..# #..# .### ...# .##."},
    {'h', 4, "#... #... ###. #..# #..# #..# #..# #..#"},
    {'i', 1, "# . # # # # # #"},
    {'j', 2, ".# .. .# .# .# .# .# .# .# #."},
    {'k', 4, "#... #... #..# #.#. ##.. #.#. #..# #..#"},
    {'l', 1, "# # # # # # # #"},
    {'m', 7, "....... ....... ###.##. #..#..# #..#..# #..#..# #..#..# #..#..#"},
    {'n', 4, ".... .... ###. #..# #..# #..# #..# #..#"},
    {'o', 4, ".... .... .##. #..# #..# #..# #..# .##."},
    {'p', 4, ".... .... ###. #..# #..# #..# #..# ###. #... #..."},
    {'q', 4, ".... .... .### #..# #..# #..# #..# .### ...# ...#"},
    {'r', 3, "... ... #.# ##. #.. #.. #.. #.."},
    {'s', 4, ".... .... .### #... .##. ...# ...# ###."},
    {'t', 3, ".#. .#. ### .#. .#. .#. .#. ..#"},
    {'u', 4, ".... .... #..# #..# #..# #..# #..# .###"},
    {'v', 5, "..... ..... #...# #...# .#.#. .#.#. ..#.. ..#.."},
    {'w', 7, "....... ....... #..#..# #..#..# #..#..# .#.#.#. .#.#.#. .#.#.#."},
    {'x', 4, ".... .... #..# #..# .##. .##. #..# #..#"},
    {'y', 4, ".... .... #..# #..# #..# #..# #..# .### ...# .##."},
    {'z', 4, ".... .... #### ...# ..#. .#.. #... ####"},
    {'{', 3, "..# .#. .#. .#. #.. .#. .#. .#. .#. ..#"},
    {'|', 1, "# # # # # # # # # #"},
    {'}', 3, "#.. .#. .#. .#. ..# .#. .#. .#. .#. #.."},
    {'~', 5, "..... ..... ..... .#..# #.##."},
    {0x99, 9, "###.#...# .#..##.## .#..#.#.# .#..#...#"},   /* trade mark */
    {0xa9, 8, ".######. #......# #.####.# #.#....# #.#....# #.####.# #......# .######."},
    {0xae, 8, ".######. #......# #.###..# #.#..#.# #.###..# #.#..#.# #......# .######."},
    {0x91, 1, "# #"},
    {0x92, 1, "# #"},
    {0x96, 4, ".... .... .... .... #### .... .... ...."},
    {0xe9, 4, "..#. .#.. .##. #..# #### #... #..# .##."},
    {0xb0, 3, ".#. #.# .#."},
};

#define ROWS 10

typedef struct Glyph {
    int w;
    uint16_t rows[ROWS];        /* bit x set: pixel x */
} Glyph;

static Glyph g_font[256];
static HFONT g_dialogFont, g_systemFont;

void font_init(void)
{
    PFont *f;
    size_t i;

    for (i = 0; i < 256; i++)
        g_font[i].w = 0;
    for (i = 0; i < sizeof g_glyphs / sizeof g_glyphs[0]; i++) {
        Glyph *g = &g_font[g_glyphs[i].ch];
        const char *p = g_glyphs[i].rows;
        int r = 0, x = 0;

        g->w = g_glyphs[i].w;
        memset(g->rows, 0, sizeof g->rows);
        for (; *p && r < ROWS; p++) {
            if (*p == ' ') {
                r++;
                x = 0;
                continue;
            }
            if (*p == '#')
                g->rows[r] |= (uint16_t)(1 << x);
            x++;
        }
    }
    /* anything else shows as a box */
    for (i = 0; i < 256; i++) {
        if (!g_font[i].w && i > ' ') {
            Glyph *g = &g_font[i];
            int r;

            g->w = 4;
            for (r = 1; r < 8; r++)
                g->rows[r] = (r == 1 || r == 7) ? 0xf : 0x9;
        }
    }
    g_dialogFont = gdi_new_font(&f);
    f->height = 13;
    f->ascent = 11;
    f->scale = 1;
    f->stock = 1;
    g_systemFont = gdi_new_font(&f);
    f->height = 16;
    f->ascent = 13;
    f->scale = 1;
    f->bold = 1;
    f->stock = 1;
    gdi_set_stock_fonts(g_systemFont, g_dialogFont);
}

HFONT font_default(void)
{
    return g_dialogFont;
}

HFONT WINAPI CreateFontA(int h, int w, int esc, int orient, int weight, DWORD italic, DWORD underline,
                         DWORD strike, DWORD charset, DWORD outPrec, DWORD clipPrec, DWORD quality, DWORD pitch,
                         LPCSTR face)
{
    PFont *f;
    HFONT hf = gdi_new_font(&f);
    int cell;

    (void)w;
    (void)esc;
    (void)orient;
    (void)italic;
    (void)underline;
    (void)strike;
    (void)charset;
    (void)outPrec;
    (void)clipPrec;
    (void)quality;
    (void)pitch;
    (void)face;
    if (h < 0)
        cell = -h + (-h >= 13 ? 3 : 2);     /* character height to cell height */
    else
        cell = h ? h : 13;
    if (cell < 10)
        cell = 10;
    f->scale = cell >= 24 ? 2 : 1;
    f->height = cell;
    f->ascent = cell - 2 * f->scale;
    f->bold = weight >= FW_SEMIBOLD;
    return hf;
}

int font_char_width(PFont *f, unsigned char c)
{
    return (g_font[c].w + 1 + (f && f->bold)) * (f ? f->scale : 1);
}

int font_text_width(PFont *f, const char *s, int n)
{
    int w = 0, i;

    if (n < 0)
        n = (int)strlen(s);
    for (i = 0; i < n; i++)
        w += font_char_width(f, (unsigned char)s[i]);
    return w;
}

int font_avg_width(PFont *f)
{
    int sum = 0, c;

    for (c = 'a'; c <= 'z'; c++)
        sum += font_char_width(f, (unsigned char)c);
    return (sum + 13) / 26;
}

void font_draw(PDC *dc, PFont *f, int x, int y, const char *s, int n, uint32_t color)
{
    int sc = f->scale, top = y + f->ascent - 8 * sc, i;
    Surface *surf = dc->s;
    int c8 = surf->bpp == 8 ? surf_nearest(surf, color) : 0;
    int any = 0;

    if (n < 0)
        n = (int)strlen(s);
    for (i = 0; i < n; i++) {
        const Glyph *g = &g_font[(unsigned char)s[i]];
        int r, bx, k;

        for (r = 0; r < ROWS; r++) {
            uint16_t bits = g->rows[r];

            if (f->bold)
                bits |= (uint16_t)(bits << 1);
            for (bx = 0; bits >> bx; bx++) {
                int px, py, sy, sx;

                if (!(bits & (1 << bx)))
                    continue;
                for (sy = 0; sy < sc; sy++)
                    for (sx = 0; sx < sc; sx++) {
                        px = x + bx * sc + sx + dc->ox;
                        py = top + r * sc + sy + dc->oy;
                        for (k = 0; k < dc->clip.n; k++) {
                            const RECT *c = &dc->clip.r[k];

                            if (px >= c->left && px < c->right && py >= c->top && py < c->bottom) {
                                BYTE *row = surf_row(surf, py);

                                if (surf->bpp == 8)
                                    row[px] = (BYTE)c8;
                                else
                                    ((uint32_t *)row)[px] = color;
                                any = 1;
                                break;
                            }
                        }
                    }
            }
        }
        x += font_char_width(f, (unsigned char)s[i]);
    }
    if (any)
        dc_mark_dirty(dc);
}

int font_draw_prefixed(PDC *dc, PFont *f, int x, int y, const char *s, uint32_t color, int prefix)
{
    int x0 = x;

    for (; *s; s++) {
        if (prefix && *s == '&' && s[1]) {
            s++;
            if (*s != '&') {
                int cw = font_char_width(f, (unsigned char)*s);

                font_draw(dc, f, x, y, s, 1, color);
                gdi_hline(dc, x, x + cw - f->scale, y + f->ascent + f->scale - 1, color);
                x += cw;
                continue;
            }
        }
        font_draw(dc, f, x, y, s, 1, color);
        x += font_char_width(f, (unsigned char)*s);
    }
    return x - x0;
}

/* Calls line() for each line of s wrapped to width (at spaces, and at \n) */
int font_wrap(PFont *f, const char *s, int width, int (*line)(void *ctx, const char *s, int n), void *ctx)
{
    int lines = 0;

    while (*s) {
        const char *p = s, *brk = NULL;
        int w = 0;

        while (*p && *p != '\n' && *p != '\r') {
            int cw = font_char_width(f, (unsigned char)*p);

            if (*p == ' ')
                brk = p;
            if (w + cw > width && p > s) {
                if (brk)
                    p = brk;
                break;
            }
            w += cw;
            p++;
        }
        line(ctx, s, (int)(p - s));
        lines++;
        if (*p == '\r')
            p++;
        if (*p == '\n' || *p == ' ')
            p++;
        s = p;
    }
    return lines;
}

/* ---- gdi32 text ---- */

BOOL WINAPI ExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT *r, LPCSTR text, UINT n, const INT *dx)
{
    PDC *dc = dc_get(hdc);
    PFont *f;
    Region saved;

    (void)dx;
    if (!dc)
        return FALSE;
    f = font_get(dc->font);
    if (r && (options & ETO_OPAQUE))
        gdi_fill(dc, r, color_xrgb(dc->bk));
    if (!text || !n)
        return TRUE;
    saved = dc->clip;
    if (r && (options & ETO_CLIPPED)) {
        RECT c = *r;

        OffsetRect(&c, dc->ox, dc->oy);
        rgn_intersect(&dc->clip, &c);
    }
    if (dc->bkMode == OPAQUE && !(r && (options & ETO_OPAQUE))) {
        RECT b = {x, y, x + font_text_width(f, text, (int)n), y + f->height};

        gdi_fill(dc, &b, color_xrgb(dc->bk));
    }
    font_draw(dc, f, x, y, text, (int)n, color_xrgb(dc->text));
    dc->clip = saved;
    return TRUE;
}

BOOL WINAPI TextOutA(HDC hdc, int x, int y, LPCSTR text, int n)
{
    return ExtTextOutA(hdc, x, y, 0, NULL, text, (UINT)n, NULL);
}

BOOL WINAPI GetTextExtentPointA(HDC hdc, LPCSTR text, int n, LPSIZE size)
{
    PDC *dc = dc_get(hdc);
    PFont *f = font_get(dc ? dc->font : NULL);

    size->cx = font_text_width(f, text, n);
    size->cy = f->height;
    return TRUE;
}

BOOL WINAPI GetTextExtentPoint32A(HDC hdc, LPCSTR text, int n, LPSIZE size)
{
    return GetTextExtentPointA(hdc, text, n, size);
}

BOOL WINAPI GetTextMetricsA(HDC hdc, LPTEXTMETRIC tm)
{
    PDC *dc = dc_get(hdc);
    PFont *f = font_get(dc ? dc->font : NULL);

    memset(tm, 0, sizeof *tm);
    tm->tmHeight = f->height;
    tm->tmAscent = f->ascent;
    tm->tmDescent = f->height - f->ascent;
    tm->tmInternalLeading = f->height - 11 * f->scale > 0 ? f->height - 11 * f->scale : 0;
    tm->tmExternalLeading = 0;
    tm->tmAveCharWidth = font_avg_width(f);
    tm->tmMaxCharWidth = font_char_width(f, 'W');
    tm->tmWeight = f->bold ? FW_BOLD : FW_NORMAL;
    tm->tmDigitizedAspectX = tm->tmDigitizedAspectY = 96;
    tm->tmFirstChar = 0x20;
    tm->tmLastChar = 0xff;
    tm->tmDefaultChar = 0x80;
    tm->tmBreakChar = 0x20;
    tm->tmPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    return TRUE;
}
