/*
 * The platform layer's internals (PORTING.md, section 3). Shared by the
 * layer's files only; the game sees port/include.
 */
#ifndef PORT_INTERNAL_H
#define PORT_INTERNAL_H

#include <windows.h>
#include "plat.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

size_t port_strlcpy(char *dst, const char *src, size_t size);
size_t port_strlcat(char *dst, const char *src, size_t size);
void port_utf8(const char *s, char *out, size_t size);     /* the game's code page to UTF-8 */

/* ---- Handles ----
 * Every handle the game sees (windows, DCs, GDI objects, menus, cursors,
 * accelerators) is a 32-bit value: an index into one table in the low 16 bits
 * and a generation count in the high 16 bits, as Win64 keeps USER and GDI
 * handles to 32 significant bits. The game relies on it: it keeps a window in
 * an int, returns a brush as a BOOL and rebuilds a control's HWND from
 * LOWORD(lParam), so a lookup also accepts the index alone. */
enum {
    H_FREE, H_WND, H_DC, H_BITMAP, H_BRUSH, H_PEN, H_FONT, H_PALETTE, H_MENU, H_CURSOR, H_ACCEL,
    H_WAVEOUT, H_MMIO
};
uintptr_t port_handle_new(int type, void *obj);
void *port_handle_get(const void *h, int type);    /* NULL if not that type */
int port_handle_type(const void *h);
void port_handle_free(const void *h);

/* ---- Errors and tracing ---- */
extern DWORD port_last_error;
void port_trace(const char *fmt, ...);      /* LEMMINGS_TRACE=1 */
void port_fatal(const char *fmt, ...);

/* ---- Rectangles and regions ---- */
#define PORT_MAX_RECTS 96
typedef struct Region {
    int n;
    RECT r[PORT_MAX_RECTS];
} Region;
void rgn_set(Region *g, const RECT *r);
void rgn_intersect(Region *g, const RECT *r);
void rgn_subtract(Region *g, const RECT *r);
int rect_intersect(RECT *d, const RECT *a, const RECT *b);
int rect_empty(const RECT *r);

/* ---- Surfaces: pixels a DC draws into ----
 * 8 bits (a colour table) or 32 bits (0x00RRGGBB). DIBs are bottom-up when
 * their header's height is positive, like Windows' DIB sections. */
typedef struct Surface {
    int w, h;
    int bpp;                /* 8 or 32 */
    int pitch;              /* bytes per row */
    int bottomUp;
    BYTE *bits;
    int ownBits;
    RGBQUAD colors[256];
    int ncolors;
} Surface;
static inline BYTE *surf_row(const Surface *s, int y)
{
    return s->bits + (size_t)(s->bottomUp ? s->h - 1 - y : y) * s->pitch;
}
uint32_t color_xrgb(COLORREF c);
int surf_nearest(const Surface *s, uint32_t xrgb);  /* nearest colour-table index */

/* ---- GDI objects ---- */
typedef struct Bitmap_ {
    Surface s;
    BITMAPINFOHEADER header;
    int dib;                /* a DIB section (the bits belong to the game too) */
    int selected;           /* selected into a DC */
} PBitmap;
typedef struct {
    int style;              /* BS_SOLID, BS_NULL, BS_HATCHED */
    COLORREF color;
    int hatch;
    int sysColor;           /* >= 0: a system colour brush (never deleted) */
    int stock;
} PBrush;
typedef struct {
    int style;
    COLORREF color;
    int stock;
} PPen;
typedef struct {
    int height;             /* cell height in pixels */
    int ascent;
    int scale;              /* 1 or 2 */
    int bold;
    int stock;
} PFont;
typedef struct {
    int n;
    PALETTEENTRY e[256];
    int stock;
} PPalette;
typedef struct PGdi {       /* deferred deletion while selected */
    int deleted;
    int selected;
} PGdi;

/* ---- Windows ---- */
typedef struct WndClass {
    char name[64];
    WNDCLASSA wc;
    int builtin;
} WndClass;

typedef struct ScrollBar {
    int min, max, pos;
    int shown;
} ScrollBar;

typedef struct Wnd Wnd;
struct Wnd {
    HWND h;
    WndClass *cls;
    WNDPROC proc;
    DWORD style, exStyle;
    Wnd *parent;            /* WS_CHILD: the parent; otherwise NULL */
    Wnd *owner;             /* a top-level window's owner */
    Wnd *child;             /* first child: the top of the z-order */
    Wnd *next, *prev;       /* siblings (top-level windows: the top-level list) */
    RECT rc;                /* the window: parent's client coordinates (child) or screen (top-level) */
    int id;
    HMENU menu;
    char *text;
    BYTE *extra;
    int nextra;
    LONG_PTR user;
    int destroyed;
    int state;              /* SIZE_RESTORED, SIZE_MINIMIZED, SIZE_MAXIMIZED */
    ScrollBar sb[2];
    /* painting */
    int invalid;
    RECT inv;               /* client coordinates */
    int erase;
    HFONT font;             /* WM_SETFONT */
    /* top-level windows: their system window and framebuffer */
    PlatWindow *plat;
    Surface fb;
    int dirty;
    Surface *overlay;       /* open menu, drawn over the framebuffer */
    int overlayX, overlayY;
    int minW, minH, maxW, maxH;
    /* dialogs and controls */
    struct Dlg *dlg;
    void *ctl;
    void (*ctlFree)(Wnd *w);
    struct Peer *peer;      /* the system's view showing this control (peer.c) */
};

extern Wnd *port_top;       /* top-level windows, front first */
extern Wnd *port_desktop;
extern Wnd *port_capture;
extern Wnd *port_focus;
extern Wnd *port_active;
Wnd *wnd_get(HWND h);
Wnd *wnd_toplevel(Wnd *w);
int wnd_visible(Wnd *w);    /* it and every parent visible */
void wnd_client_offset(Wnd *w, int *left, int *top, int *right, int *bottom);   /* non-client sizes */
void wnd_window_fb(Wnd *w, RECT *r);    /* the window rect in its top-level's framebuffer */
void wnd_client_fb(Wnd *w, RECT *r);    /* the client rect in the framebuffer */
void wnd_visible_region(Wnd *w, int client, Region *g);
void wnd_invalidate(Wnd *w, const RECT *client, int erase);
void wnd_redraw_all(Wnd *w);
Wnd *wnd_from_point(Wnd *top, int fx, int fy, int *hit);   /* framebuffer coordinates */
WndClass *wnd_find_class(const char *name);
void wnd_register_builtin(const char *name, WNDPROC proc, UINT style, int extra, int cursor);
void wnd_send_size(Wnd *w);
void wnd_nc_paint(Wnd *w);
void wnd_top_resized(Wnd *w, int width, int height);
void wnd_surface_resize(Wnd *w, int width, int height);
LRESULT wnd_send(Wnd *w, UINT msg, WPARAM wp, LPARAM lp);
int wnd_scrollbar_rect(Wnd *w, int bar, RECT *r);   /* in window coordinates */
void wnd_scroll_track(Wnd *w, int bar, int x, int y);
int wnd_is_dialog(Wnd *w);

/* ---- DCs ---- */
typedef struct PDC {
    HDC h;
    int kind;               /* DCK_* */
    Wnd *wnd;
    Surface *s;             /* what it draws into */
    int ox, oy;             /* the DC's origin in the surface */
    Region clip;            /* surface coordinates */
    HBITMAP bitmap;         /* memory DCs */
    HBRUSH brush;
    HPEN pen;
    HFONT font;
    HPALETTE palette;
    COLORREF text, bk;
    int bkMode;
    POINT cur;
} PDC;
enum { DCK_CLIENT, DCK_WINDOW, DCK_MEMORY, DCK_SCREEN };
PDC *dc_get(HDC h);
HDC dc_for_window(Wnd *w, int client, const RECT *paint);
void dc_mark_dirty(PDC *dc);
void gdi_init(void);
void gdi_fill(PDC *dc, const RECT *r, uint32_t xrgb);  /* DC coordinates */
void gdi_fill_brush(PDC *dc, const RECT *r, HBRUSH brush);
void gdi_frame3d(PDC *dc, const RECT *r, uint32_t tl, uint32_t br);
void gdi_hline(PDC *dc, int x0, int x1, int y, uint32_t c);
void gdi_vline(PDC *dc, int x, int y0, int y1, uint32_t c);
void gdi_pixel(PDC *dc, int x, int y, uint32_t c);
uint32_t sys_xrgb(int index);
PBrush *brush_get(HBRUSH h);

/* ---- Text ---- */
void font_init(void);
PFont *font_get(HFONT h);
HFONT font_default(void);           /* the dialog font (MS Sans Serif 8) */
int font_text_width(PFont *f, const char *s, int n);
void font_draw(PDC *dc, PFont *f, int x, int y, const char *s, int n, uint32_t color);
int font_avg_width(PFont *f);
int font_char_width(PFont *f, unsigned char c);
/* draws text with & mnemonics (underlined), clipped to r; returns its width */
int font_draw_prefixed(PDC *dc, PFont *f, int x, int y, const char *s, uint32_t color, int prefix);
int font_wrap(PFont *f, const char *s, int width, int (*line)(void *ctx, const char *s, int n), void *ctx);

/* ---- Messages ---- */
/* modal loops (a menu, a scroll bar) see input first: 1 if they took it */
typedef struct PortInput {
    int type;
    Wnd *top;
    int fx, fy;
    int button;
    int vk;
} PortInput;
enum { PI_MOVE, PI_DOWN, PI_UP, PI_KEY, PI_FOCUS_LOST, PI_QUIT };
extern int (*port_input_hook)(const PortInput *in);
void msg_init(void);
void msg_post(HWND h, UINT msg, WPARAM wp, LPARAM lp);
int msg_pump(int wait);             /* the backend's events into messages; returns the number handled */
void msg_present(void);
void msg_window_gone(Wnd *w);       /* drop its timers and queued messages */
void msg_set_cursor_for(Wnd *top, int fx, int fy);
DWORD port_ticks(void);
extern int port_modal_depth;
extern int port_quit;
extern POINT port_mouse;            /* screen */
int port_key_down(int vk);

/* ---- Menus ---- */
void menu_init(void);
int menu_bar_height(Wnd *w);
void menu_bar_draw(Wnd *w);
int menu_bar_click(Wnd *w, int fx, int fy);         /* 1 if the menu took it */
int menu_key(Wnd *w, int vk, int ch);                /* Alt+letter, F10 */
int menu_item_enabled(HMENU m, UINT id);

/* ---- Dialogs and controls ---- */
void controls_init(void);
int dlg_is_dialog_message(Wnd *dlg, MSG *msg);
void dlg_units(HFONT font, int *bx, int *by);
uint32_t ctl_color(Wnd *w, UINT msg, HDC dc, int *brushOut);
void draw_button_face(PDC *dc, const RECT *r, int pressed, int isDefault);
void draw_scrollbar(PDC *dc, const RECT *r, int vert, int min, int max, int pos, int page, int enabled);

/* ---- Resources ---- */
const void *res_find_file(const char *type, const char *name, unsigned *size);
HCURSOR cursor_system(LPCSTR id);
void cursor_apply(HCURSOR c);
int cursor_hotspot_size(HCURSOR c, int *w, int *h);
void icon_draw(PDC *dc, HICON icon, int x, int y);

/* ---- Files ---- */
void file_init(const char *dataDir, const char *prefDir);
int file_host_path(const char *winPath, char *out, size_t size, int forWrite);
const char *file_data_dir(void);
const char *file_pref_dir(void);

/* ---- Registry ---- */
void reg_init(void);
void reg_save(void);

/* ---- Sound and music ---- */
void wave_init(void);
void wave_quit(void);
void mci_poll(void);                /* posts MM_MCINOTIFY when a tune ends */
void mci_quit(void);

/* ---- Test harness (LEMMINGS_HARNESS=1) ---- */
extern int port_harness;
void harness_init(void);
void harness_poll(void);
/* peer.c: native views for dialog controls */
void peer_create(Wnd *w);
void peer_destroy(Wnd *w);
void peer_sync_all(void);
void peer_focus(Wnd *w);
int peer_paint(Wnd *w);     /* WM_PAINT of a control with a view: 1 if handled */
int lb_serial(Wnd *w);      /* controls.c: changes when a list box's rows do */
void port_harness_log(const char *fmt, ...);
int harness_cursor(POINT *pt);
int harness_key(int vk);
void harness_update(void);

#endif
