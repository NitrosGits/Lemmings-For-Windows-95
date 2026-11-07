/*
 * The map window: the whole level shrunk into a tool window, scrolling
 * sideways, with the lemmings as dots and the main view as a rectangle.
 * Also the DIB-section bitmaps the icon buttons draw into.
 */
/* Three unused locals in the inlined Bitmap::SetPixel (window.h, SETPIXEL_PAD) stand for symbols
   the original had: inline bodies are expanded at the end of the file, so their registers depend on
   the symbol count (see docs/DECOMP.md and HANDOFF session 33). */
#define SETPIXEL_PAD int __lp0; int __lp1; int __lp2;
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14, pad_top15, pad_top16, pad_top17, pad_top18, pad_top19, pad_top20, pad_top21;

#include <windows.h>
#include <string.h>
#include "winlemm.h"
#include "window.h"

/* Windows' twenty static colours, for when the system palette can't be read */
BYTE g_defaultColours[20][3] = {
    {0x00, 0x00, 0x00}, {0x80, 0x00, 0x00}, {0x00, 0x80, 0x00}, {0x80, 0x80, 0x00},
    {0x00, 0x00, 0x80}, {0x80, 0x00, 0x80}, {0x00, 0x80, 0x80}, {0xc0, 0xc0, 0xc0},
    {0xc0, 0xdc, 0xc0}, {0xa6, 0xca, 0xf0}, {0xff, 0xfb, 0xf0}, {0xa0, 0xa0, 0xa4},
    {0x80, 0x80, 0x80}, {0xff, 0x00, 0x00}, {0x00, 0xff, 0x00}, {0xff, 0xff, 0x00},
    {0x00, 0x00, 0xff}, {0xff, 0x00, 0xff}, {0x00, 0xff, 0xff}, {0xff, 0xff, 0xff},
};
UINT g_mapTimer = 0x1245;

short g_lemmingXY[160];

/* 0x4100a0 */
Bitmap::Bitmap(int width, int height, COLORREF colour)
{
    m_palette = 0;
    m_dib = 0;
    m_rows = 0;
    m_bits = 0;
    width = (width + 3) & ~3;
    Create(width, height);
    m_colour = (BYTE)GetNearestPaletteIndex(m_palette, colour);
    Clear();
}

/* 0x410120 */
Bitmap::~Bitmap()
{
    Free();
}

/* 0x410130 */
void Bitmap::Free()
{
    if (m_palette) {
        DeleteObject(m_palette);
        m_palette = 0;
    }
    if (m_dib) {
        if (m_oldBitmap)
            SelectObject(m_dc, m_oldBitmap);
        DeleteObject(m_dib);
        DeleteDC(m_dc);
        m_dc = 0;
        m_dib = 0;
        m_oldBitmap = 0;
    }
    if (m_rows) {
        delete m_rows;
        m_rows = 0;
    }
}

/* 0x4101b0 */
void Bitmap::Create(int width, int height)
{
    m_width = width;
    m_height = height;
    m_header.biSize = sizeof(BITMAPINFOHEADER);
    m_header.biWidth = width;
    m_header.biHeight = height;
    m_header.biPlanes = 1;
    m_header.biBitCount = 8;
    m_header.biCompression = 0;
    m_header.biSizeImage = 0;
    m_header.biXPelsPerMeter = 0;
    m_header.biYPelsPerMeter = 0;
    m_header.biClrUsed = 0;
    m_header.biClrImportant = 0;
    m_palette = MakePalette();
    m_dc = CreateCompatibleDC(0);
    m_dib = CreateDIBSection(m_dc, (BITMAPINFO *)&m_header, DIB_RGB_COLORS, (void **)&m_bits, 0, 0);
    m_size = m_width * m_height;
    BuildRows();
    m_oldBitmap = (HBITMAP)SelectObject(m_dc, m_dib);
}

/* 0x410250 */
void Bitmap::BuildRows()
{
    int offset;
    int pitch;
    BYTE *bits;
    int i;

    m_rows = new BYTE *[m_height];
    if (m_header.biHeight >= 0) {
        pitch = m_width;
        offset = (m_height - 1) * pitch;
        pitch = -pitch;
    } else {
        pitch = m_width;
        offset = 0;
    }
    bits = m_bits;
    m_pitch = pitch;
    for (i = 0; i < m_height; i++) {
        m_rows[i] = bits + offset;
        offset += pitch;
    }
}

/* 0x4102c0: the system's static colours round a grey ramp */
HPALETTE Bitmap::MakePalette()
{
    struct {
        WORD palVersion;
        WORD palNumEntries;
        PALETTEENTRY palPalEntry[256];
    } pal;
    HDC hdc;
    RGBQUAD *colours;
    int i;
    UINT n;

    hdc = GetDC(0);
    n = GetSystemPaletteEntries(hdc, 0, 10, &pal.palPalEntry[0]);
    n |= GetSystemPaletteEntries(hdc, 246, 10, &pal.palPalEntry[246]);
    if (!n) {
        BYTE *c = g_defaultColours[0];

        for (i = 0; i < 10; i++) {
            pal.palPalEntry[i].peRed = *c++;
            pal.palPalEntry[i].peGreen = *c++;
            pal.palPalEntry[i].peBlue = *c++;
        }
        for (i = 246; i < 256; i++) {
            pal.palPalEntry[i].peRed = *c++;
            pal.palPalEntry[i].peGreen = *c++;
            pal.palPalEntry[i].peBlue = *c++;
        }
    }
    ReleaseDC(0, hdc);
    colours = m_colours;
    pal.palVersion = 0x300;
    pal.palNumEntries = 256;
    for (i = 0; i < 10; i++) {
        colours[i].rgbRed = pal.palPalEntry[i].peRed;
        colours[i].rgbGreen = pal.palPalEntry[i].peGreen;
        colours[i].rgbBlue = pal.palPalEntry[i].peBlue;
        colours[i].rgbReserved = 0;
        pal.palPalEntry[i].peFlags = 0;
        colours[i + 246].rgbRed = pal.palPalEntry[i + 246].peRed;
        colours[i + 246].rgbGreen = pal.palPalEntry[i + 246].peGreen;
        colours[i + 246].rgbBlue = pal.palPalEntry[i + 246].peBlue;
        colours[i + 246].rgbReserved = 0;
        pal.palPalEntry[i + 246].peFlags = 0;
    }
    for (i = 10; i < 246; i++) {
        pal.palPalEntry[i].peRed = i;
        colours[i].rgbRed = i;
        pal.palPalEntry[i].peGreen = i;
        colours[i].rgbGreen = i;
        pal.palPalEntry[i].peBlue = i;
        colours[i].rgbBlue = i;
        colours[i].rgbReserved = 0;
        pal.palPalEntry[i].peFlags = PC_NOCOLLAPSE;
    }
    return CreatePalette((LOGPALETTE *)&pal);
}

/* 0x410420 */
void Bitmap::Resize(int width, int height)
{
    width = (width + 3) & ~3;
    if (m_width != width || m_height != height) {
        Free();
        Create(width, height);
    }
}

/* 0x410460 */
void Bitmap::Draw(HDC hdc, int x, int y, int w, int h, int sx, int sy)
{
    BitBlt(hdc, x, y, w, h, m_dc, sx, sy, SRCCOPY);
}

/* 0x4104a0 */
MapWnd::MapWnd()
{
}

/* 0x4104c0 */
MapWnd::~MapWnd()
{
}

/* 0x4104e0 */
void MapWnd::OnCreate()
{
    RECT r;

    r.left = 4;
    r.top = 4;
    r.right = m_rect.right - 8;
    r.bottom = m_rect.bottom - 8;
    m_view.Create(&r, m_hwnd, "internal map window");
    m_timer = SetTimer(m_hwnd, g_mapTimer, 64, 0);
}

/* 0x410540 */
void MapWnd::OnDestroy()
{
    if (m_timer) {
        KillTimer(m_hwnd, g_mapTimer);
        m_timer = 0;
    }
    m_view.Destroy();
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_mapwndwndproc0, pad_mapwndwndproc1, pad_mapwndwndproc2, pad_mapwndwndproc3, pad_mapwndwndproc4, pad_mapwndwndproc5;

/* 0x410570 */
LRESULT MapWnd::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_TIMER:
        if (wParam == g_mapTimer)
            m_view.Update();
        return 0;
    case 0x401:                 /* a level was loaded */
        m_view.Init();
        break;
    case 0x402:                 /* the level is going */
        m_view.Free();
        break;
    }
    return DockWnd::WndProc(msg, wParam, lParam);
}

/* 0x4105e0 */
MapView::MapView()
{
    m_cursor = LoadCursor(0, IDC_ARROW);
    SetCursor(m_cursor);
}

/* 0x410610 */
MapView::~MapView()
{
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_mapviewwndproc0, pad_mapviewwndproc1, pad_mapviewwndproc2, pad_mapviewwndproc3, pad_mapviewwndproc4, pad_mapviewwndproc5, pad_mapviewwndproc6, pad_mapviewwndproc7;

/* 0x410620 */
LRESULT MapView::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    RECT r;
    int x;

    switch (msg) {
    case WM_SETFOCUS:
        SendMessage(g_hwnd, WM_NCACTIVATE, 1, 0);
        return 0;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON) {
            x = m_xstep * LOWORD(lParam) / 256 - (short)((short)(g_dataViewWidth / g_scale) / 2);
            ScrollTo(g_hwndView, (short)x * g_scale - g_tempScrollX);
            Update();
            r.top = 0;
            r.left = 0;
            r.right = m_rect.right;
            r.bottom = m_rect.bottom;
            Redraw(&r);
        }
        break;
    case WM_LBUTTONDOWN:
        x = m_xstep * LOWORD(lParam) / 256 - (short)((short)(g_dataViewWidth / g_scale) / 2);
        ScrollTo(g_hwndView, (short)x * g_scale - g_tempScrollX);
        return 0;
    }
    return Window::WndProc(msg, wParam, lParam);
}

/* 0x410790 */
void MapView::Init()
{
    m_level = new Bitmap(m_rect.right, m_rect.bottom, 0);
    m_view = new Bitmap(m_rect.right, m_rect.bottom, 0);
    m_surface = GetSurfacePointer(g_renderer, g_slot);
    m_modulo = GetSurfaceModulo(g_renderer, g_slot);
    Scale();
}

/* 0x410830 */
void MapView::Free()
{
    if (m_level) {
        delete m_level;
        m_level = 0;
    }
    if (m_view) {
        delete m_view;
        m_view = 0;
    }
}

/* 0x410880 */
void MapView::OnCreate()
{
    Init();
}

/* 0x410890 */
void MapView::OnDestroy()
{
    Free();
}

/* 0x4108a0: shrink the level into m_level */
void MapView::Scale()
{
    int w = m_rect.right;
    int h = m_rect.bottom;
    BYTE *dst;
    int pitch;
    int sy;
    int j;

    m_width = w;
    m_height = h;
    m_xstep = g_resolutionNew * 1668 * 256 / w;
    m_ystep = g_resolutionNew * 160 * 256 / h;
    dst = m_level->m_rows[h - 1];
    pitch = m_level->m_pitch;
    sy = 0;
    for (j = 0; j < m_height; j++) {
        BYTE *src;
        BYTE *d;
        int sx;
        int i;

        if (m_modulo < 0)
            src = m_surface - sy / 256 * m_modulo;
        else
            src = m_surface + (g_resolutionNew * 160 - sy / 256 - 1) * m_modulo;
        d = dst;
        sx = 0;
        for (i = 0; i < m_width; i++) {
            *d++ = src[sx / 256];
            sx += m_xstep;
        }
        dst -= pitch;
        sy += m_ystep;
    }
    m_scrollX = 0;
    m_scrollPos = 0;
}

/* 0x4109c0: shrink the next strip of the level, as the map scrolls */
void MapView::Scroll()
{
    BYTE *dst = m_level->m_rows[m_height - 1] + m_scrollX;
    int pitch = m_level->m_pitch;
    int sy;
    int pos;
    int j;

    sy = 0;
    for (j = 0; j < m_height; j++) {
        BYTE *d;
        BYTE *src;
        int n;

        if (m_modulo < 0)
            src = m_surface - sy / 256 * m_modulo;
        else
            src = m_surface + (g_resolutionNew * 160 - sy / 256 - 1) * m_modulo;
        d = dst;
        pos = m_scrollPos;
        for (n = 0; n < g_resolutionNew * 32 * 256; n += m_xstep) {
            *d++ = src[pos / 256];
            pos += m_xstep;
        }
        dst -= pitch;
        sy += m_ystep;
    }
    m_scrollPos = pos;
    if (pos > g_resolutionNew * 1668 * 256 - 0x2000) {
        m_scrollPos = 0;
        m_scrollX = 0;
    } else
        m_scrollX = pos / m_xstep;
}

/* 0x410af0 */
void MapView::Update()
{
    if (m_level && m_view) {
        Scroll();
        memcpy(m_view->m_bits, m_level->m_bits, m_level->m_size);
        DrawLemmings();
        DrawViewRect();
        InvalidateRect(m_hwnd, 0, FALSE);
    }
}

/* 0x410b50 */
void MapView::Paint(HDC hdc, RECT *rect)
{
    if (m_level && m_view)
        m_view->Draw(hdc, rect->left, rect->top, rect->right, rect->bottom, rect->left, rect->top);
}

/* 0x410b90 */
void MapView::DrawLemmings()
{
    int n = GetLemmingXY(g_lemmingXY, 80);
    int i;

    for (i = 0; i < n; i++) {
        int x = (WORD)g_lemmingXY[i * 2] * g_resolutionNew * 256 / m_xstep;
        int y = (WORD)g_lemmingXY[i * 2 + 1] * g_resolutionNew * 256 / m_ystep;

        m_view->SetPixel(x, y, 0xfa);
        m_view->SetPixel(x, y - 1, 0xfa);
        m_view->SetPixel(x + 1, y - 1, 0xfa);
        m_view->SetPixel(x + 1, y, 0xfa);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_mapviewdrawviewrect0, pad_mapviewdrawviewrect1, pad_mapviewdrawviewrect2;

/* 0x410c90: the main view's outline */
void MapView::DrawViewRect()
{
    int w = (short)(g_dataViewWidth / g_scale) * 256 / m_xstep;
    int h = (short)(g_extraViewHeight / g_scale) * 256 / m_ystep;
    int x = (short)(g_tempScrollX / g_scale) * 256 / m_xstep;
    BYTE *top = m_view->m_rows[0] + x;
    BYTE *bottom = m_view->m_rows[m_height - 2] + x;
    int i;

    memset(top, 0xfb, w);
    memset(top + m_view->m_pitch, 0xfb, w);
    memset(bottom, 0xfb, w);
    memset(bottom + m_view->m_pitch, 0xfb, w);
    for (i = 0; i < h; i++) {
        top[0] = 0xfb;
        top[1] = 0xfb;
        top[w - 2] = 0xfb;
        top[w - 1] = 0xfb;
        top += m_view->m_pitch;
    }
}
