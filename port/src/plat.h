/*
 * What the Win32 layer needs from the operating system: one backend per
 * platform implements this (port/macos/plat_cocoa.m). Everything above it -
 * windows, messages, GDI, dialogs, files, registry - is shared.
 *
 * Coordinates are screen points, origin at the top left of the main display;
 * a window's rectangle is its content (the system's frame and caption are
 * outside it). Backends report input and window changes through the input_*
 * functions (msg.c).
 */
#ifndef PORT_PLAT_H
#define PORT_PLAT_H

#include <stdint.h>

typedef struct PlatWindow PlatWindow;
typedef struct PlatStream PlatStream;
struct Wnd;
struct Surface;

/* ---- The application ---- */
void plat_init(void);
void plat_quit(void);
uint64_t plat_ticks(void);                  /* milliseconds */
void plat_sleep(uint32_t ms);
char *plat_pref_dir(void);                  /* malloc'd, ends in '/' */
char *plat_base_dir(void);                  /* the program's folder, ends in '/' */
uint64_t plat_ram_bytes(void);
uint64_t plat_ram_available(void);          /* what programs could have now; 0 if not known */
int plat_os_name(char *out, int size);      /* "macOS 15.1 (24B83)"; 0 if not known */
int plat_open_url(const char *url);
void plat_usable_bounds(int *x, int *y, int *w, int *h);
/* a message box: type as MessageBox's; returns IDOK, IDCANCEL, IDYES or IDNO */
int plat_message_box(PlatWindow *owner, const char *text, const char *caption, unsigned type);

/* ---- Windows ---- */
enum {
    PW_CAPTION = 1,                         /* a system frame and caption */
    PW_RESIZABLE = 2,
    PW_TOPMOST = 4,
    PW_UTILITY = 8,                         /* a tool window or dialog of its owner */
    PW_DIALOG = 16
};
PlatWindow *plat_window_create(struct Wnd *w, int x, int y, int cw, int ch, int flags, PlatWindow *owner,
                               const char *title);
void plat_window_destroy(PlatWindow *p);
void plat_window_show(PlatWindow *p, int show, int activate);
void plat_window_minimize(PlatWindow *p);
void plat_window_maximize(PlatWindow *p);
void plat_window_restore(PlatWindow *p);
void plat_window_raise(PlatWindow *p);
void plat_window_set_title(PlatWindow *p, const char *title);
void plat_window_set_rect(PlatWindow *p, int x, int y, int cw, int ch);
void plat_window_set_minmax(PlatWindow *p, int minw, int minh, int maxw, int maxh);
/* shows the framebuffer, and over it the overlay (an open menu) at ox, oy */
void plat_window_present(PlatWindow *p, const struct Surface *fb, const struct Surface *overlay, int ox, int oy);
int plat_window_is_key(PlatWindow *p);
/* what the window shows (its framebuffer and any native controls) as a PNG */
int plat_window_snapshot(PlatWindow *p, const char *path);
int plat_app_active(void);

/* ---- The system's look (macOS) ----
 * The colour the system gives a COLOR_* index now (light or dark), as
 * 0x00RRGGBB; 0 if it has none (the layer then keeps Windows 95's). When the
 * look changes the backend calls input_syscolors_changed(). */
int plat_syscolor(int index, uint32_t *xrgb);
/* the harness: the look by name ("light", "dark", "system"); 0 if there is no such thing */
int plat_set_look(const char *name);

/* ---- Events and input state ---- */
int plat_pump(int timeout_ms);              /* OS events into input_* calls; returns how many */
int plat_key_down(int vk);
int plat_mods(void);                        /* PM_* below */
unsigned plat_mouse_buttons(void);          /* 1 left, 2 right, 4 middle */
void plat_mouse_pos(int *x, int *y);
void plat_set_mouse_pos(int x, int y);
void plat_capture_mouse(int on);
/* the character a key makes with these modifiers (TranslateMessage), 0 if none */
int plat_key_char(int scancode, int mods);
/* the harness's own input, as if from the user */
void plat_inject_mouse(PlatWindow *p, int x, int y, int button, int down, int clicks, int moveOnly);
void plat_inject_key(PlatWindow *p, const char *name, int down);
/* a drag with the left button from x1,y1 to x2,y2, in steps */
void plat_inject_drag(PlatWindow *p, int x1, int y1, int x2, int y2, int steps);
enum { PM_SHIFT = 1, PM_CTRL = 2, PM_ALT = 4, PM_CAPS = 8, PM_CMD = 16 };

/* ---- Cursors ---- */
void *plat_cursor_system(int idc);          /* IDC_* number */
void *plat_cursor_image(const uint32_t *argb, int w, int h, int hx, int hy);
void plat_cursor_set(void *c);              /* NULL hides it */
void plat_cursor_show(int show);

/* ---- The system's menu bar (macOS) ----
 * With one, a window's menu goes there instead of being drawn in the window
 * (menu.c). Menus and windows are passed as their 32-bit handles. */
int plat_menubar_native(void);
void plat_menubar_set(uint32_t hwnd, uint32_t hmenu);   /* hmenu 0: hwnd's menu goes */
/* the harness: chooses "Menu/Item" (the application menu is "App") */
int plat_menu_choose(const char *path);

/* ---- Native controls (macOS) ----
 * The system's own views over a dialog's buttons, edit fields and list boxes.
 * The layer's control stays the truth (its state, messages, notifications):
 * peer.c shows its state in the view, and the view reports what the user does
 * (input_control*). Frames are in the top-level window's framebuffer. */
typedef struct PlatControl PlatControl;
enum {
    PC_BUTTON, PC_DEFBUTTON, PC_CHECKBOX, PC_RADIO, PC_GROUPBOX, PC_EDIT, PC_LIST, PC_LIST_OWNERDRAW,
    PC_LABEL, PC_LABEL_CENTER, PC_LABEL_RIGHT,  /* static text: wrapped lines, left, centred, right */
    PC_LABEL_LINE,                              /* one line, left */
    PC_ICON,                                    /* a dialog's picture of the program: its icon */
    PC_ICONBUTTON,                              /* the game's skill panel: a picture, a number, on or off */
    PC_STATUSBAR                                /* the game's status bar: a message and counters */
};
PlatControl *plat_control_create(PlatWindow *top, uint32_t hwnd, int kind);  /* NULL: the layer draws it */
void plat_control_destroy(PlatControl *c);
void plat_control_frame(PlatControl *c, int x, int y, int w, int h, int visible, int enabled);
void plat_control_text(PlatControl *c, const char *utf8);
void plat_control_check(PlatControl *c, int check);
void plat_control_focus(PlatWindow *top, PlatControl *c);      /* NULL: the window itself */
void plat_list_reload(PlatControl *c, int rows, int rowHeight);
void plat_list_select(PlatControl *c, int row);               /* -1: none */
void plat_list_top(PlatControl *c, int row);                  /* the first row shown */
/* an icon button's face: its picture (w x h pixels, 0xAARRGGBB, not premultiplied; NULL keeps the
   one it has), its number ("" for none) and whether it is on. Shown only: clicks go to the layer. */
void plat_iconbutton_set(PlatControl *c, const uint32_t *argb, int w, int h, const char *count, int on);
/* a status bar's parts from the left: the first takes the room the others leave */
void plat_statusbar_set(PlatControl *c, int n, const char *const *utf8);
/* the harness: what a label, icon button or status bar shows, as text */
void plat_control_describe(PlatControl *c, char *out, int size);
/* a window's scroll bar as the system's scroller (frame, visible and enabled
   through plat_control_frame); value and proportion (the knob) 0..1 */
PlatControl *plat_scroller_create(PlatWindow *top, uint32_t hwnd, int vertical);
void plat_scroller_set(PlatControl *c, double value, double proportion);
void plat_scroller_knob(PlatControl *c, int *x, int *y);         /* the knob's centre (the harness) */

/* ---- Sound: streams mixed to the default output ---- */
int plat_audio_open(void);
void plat_audio_close(void);
PlatStream *plat_stream_create(int rate, int bits, int channels);
int plat_stream_put(PlatStream *s, const void *data, int len);
int plat_stream_queued(PlatStream *s);      /* bytes not played yet: 0 once all are */
void plat_stream_clear(PlatStream *s);
void plat_stream_gain(PlatStream *s, float gain);
void plat_stream_destroy(PlatStream *s);
/* the harness: seconds of output so far, seconds with sound in them, the peak (0..1) */
void plat_audio_level(double *rendered, double *heard, float *peak);

/* ---- What backends call (msg.c) ---- */
void input_mouse(struct Wnd *top, int fx, int fy, int button, int down, int clicks, int injected);
void input_mouse_move(struct Wnd *top, int fx, int fy, int injected);
void input_wheel(int delta, int injected);
void input_key(int vk, int scancode, int down, int repeat, int mods, int injected);
void input_window_moved(struct Wnd *w, int x, int y);
void input_window_resized(struct Wnd *w, int cw, int ch);
void input_window_state(struct Wnd *w, int state);  /* SIZE_RESTORED, _MINIMIZED, _MAXIMIZED */
void input_window_focus(struct Wnd *w);
void input_window_close(struct Wnd *w);
void input_window_exposed(struct Wnd *w);
void input_app_deactivated(void);
void input_quit(void);
void input_command(int id);                 /* a menu command to the main window */
void input_syscolors_changed(void);         /* the system's colours changed (light, dark) */
void input_tracking_idle(void);             /* now and then while the system tracks a menu or drag */

/* ---- What native controls report and ask (peer.c) ---- */
enum {
    PCE_CLICK,                              /* a button pressed */
    PCE_SELECT, PCE_DBLCLK,                 /* a list's row (arg) chosen, double-clicked */
    PCE_TOP,                                /* a list scrolled: arg is the first row shown */
    PCE_FOCUS,                              /* the view took the keyboard */
    PCE_KEY                                 /* a dialog key (VK_RETURN, VK_ESCAPE, VK_TAB) for the layer */
};
void input_control(uint32_t hwnd, int event, int arg);
void input_control_text(uint32_t hwnd, const char *utf8);     /* an edit field's new text */
/* a scroller used: code SB_*; for SB_THUMBTRACK/SB_THUMBPOSITION where the knob is (0..1) */
void input_scroll(uint32_t hwnd, int vertical, int code, double value);
int list_native_text(uint32_t hwnd, int row, char *utf8, int size);
/* an owner-drawn row through WM_DRAWITEM, 0x00RRGGBB pixels; 0 if not drawn */
int list_native_draw(uint32_t hwnd, int row, int w, int h, uint32_t *xrgb);

/* ---- What a system menu bar asks and reports (menu.c) ---- */
int menu_native_count(uint32_t hmenu);      /* -1: no such menu */
/* item pos: its text (UTF-8, without '&'), command, MF_* flags and submenu */
int menu_native_item(uint32_t hmenu, int pos, char *text, int size, unsigned *id, unsigned *flags,
                     uint32_t *sub);
/* sub (item pos of parent) is opening: WM_INITMENU(POPUP); 0 if hwnd can't take commands now */
int menu_native_open(uint32_t hwnd, uint32_t parent, int pos, uint32_t sub);
void menu_native_select(uint32_t hwnd, uint32_t hmenu, unsigned idOrPos, unsigned flags);
void menu_native_close(uint32_t hwnd);
void menu_native_command(uint32_t hwnd, unsigned id);

#endif
