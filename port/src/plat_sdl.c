/*
 * The SDL3 backend of plat.h (temporary: the macOS port moves to AppKit,
 * port/macos/plat_cocoa.m, and this file goes once that passes the checks).
 */
#include "port_internal.h"

#include <SDL3/SDL.h>

struct PlatWindow {
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *tex;
    int tw, th;
    Wnd *wnd;
};

#define INJECTED 0x504f5254u

void port_utf8(const char *s, char *out, size_t size);

/* ---- The application ---- */

void plat_init(void)
{
    SDL_SetAppMetadata("Lemmings", "1.0", "io.github.nitrosgits.lemmings-win95");
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        port_fatal("SDL_Init: %s", SDL_GetError());
}

void plat_quit(void)
{
    SDL_Quit();
}

uint64_t plat_ticks(void)
{
    return SDL_GetTicks();
}

void plat_sleep(uint32_t ms)
{
    SDL_Delay(ms);
}

char *plat_pref_dir(void)
{
    return SDL_GetPrefPath("Visual Sciences", "Lemmings");
}

char *plat_base_dir(void)
{
    const char *b = SDL_GetBasePath();

    return b ? strdup(b) : NULL;
}

uint64_t plat_ram_bytes(void)
{
    return (uint64_t)SDL_GetSystemRAM() * 1024 * 1024;
}

int plat_open_url(const char *url)
{
    return SDL_OpenURL(url);
}

void plat_usable_bounds(int *x, int *y, int *w, int *h)
{
    SDL_Rect b;

    if (!SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &b))
        b = (SDL_Rect){0, 0, 1280, 800};
    *x = b.x;
    *y = b.y;
    *w = b.w;
    *h = b.h;
}

int plat_message_box(PlatWindow *owner, const char *text, const char *caption, unsigned type)
{
    SDL_MessageBoxButtonData buttons[3];
    SDL_MessageBoxData data;
    int n = 0, pressed = IDOK;

    switch (type & 0xf) {
    case MB_OKCANCEL:
        buttons[n++] = (SDL_MessageBoxButtonData){SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDOK, "OK"};
        buttons[n++] = (SDL_MessageBoxButtonData){SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel"};
        break;
    case MB_YESNO:
        buttons[n++] = (SDL_MessageBoxButtonData){SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES, "Yes"};
        buttons[n++] = (SDL_MessageBoxButtonData){SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDNO, "No"};
        break;
    case MB_YESNOCANCEL:
        buttons[n++] = (SDL_MessageBoxButtonData){SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, IDYES, "Yes"};
        buttons[n++] = (SDL_MessageBoxButtonData){0, IDNO, "No"};
        buttons[n++] = (SDL_MessageBoxButtonData){SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDCANCEL, "Cancel"};
        break;
    default:
        buttons[n++] = (SDL_MessageBoxButtonData){SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT |
                                                      SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, IDOK, "OK"};
        break;
    }
    memset(&data, 0, sizeof data);
    data.flags = (type & 0xf0) == MB_ICONHAND ? SDL_MESSAGEBOX_ERROR
               : (type & 0xf0) == MB_ICONEXCLAMATION ? SDL_MESSAGEBOX_WARNING : SDL_MESSAGEBOX_INFORMATION;
    data.window = owner ? owner->win : NULL;
    data.title = caption;
    data.message = text;
    data.numbuttons = n;
    data.buttons = buttons;
    if (!SDL_ShowMessageBox(&data, &pressed))
        pressed = buttons[0].buttonID;
    return pressed;
}

/* ---- Windows ---- */

static Wnd *wnd_of(SDL_WindowID id)
{
    SDL_Window *s = SDL_GetWindowFromID(id);
    PlatWindow *p = s ? SDL_GetPointerProperty(SDL_GetWindowProperties(s), "port.plat", NULL) : NULL;

    return p ? p->wnd : NULL;
}

PlatWindow *plat_window_create(Wnd *w, int x, int y, int cw, int ch, int flags, PlatWindow *owner,
                               const char *title)
{
    SDL_WindowFlags f = SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_PropertiesID props;
    PlatWindow *p = calloc(1, sizeof *p);

    if (!(flags & PW_CAPTION))
        f |= SDL_WINDOW_BORDERLESS;
    if (flags & PW_RESIZABLE)
        f |= SDL_WINDOW_RESIZABLE;
    if (flags & PW_TOPMOST)
        f |= SDL_WINDOW_ALWAYS_ON_TOP;
    if (flags & PW_UTILITY)
        f |= SDL_WINDOW_UTILITY;
    props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, x);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, y);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, cw);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, ch);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, (Sint64)f);
    p->win = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!p->win) {
        free(p);
        return NULL;
    }
    p->wnd = w;
    SDL_SetPointerProperty(SDL_GetWindowProperties(p->win), "port.plat", p);
    if (owner)
        SDL_SetWindowParent(p->win, owner->win);
    p->ren = SDL_CreateRenderer(p->win, NULL);
    if (!p->ren)
        port_fatal("SDL_CreateRenderer: %s", SDL_GetError());
    SDL_SetRenderVSync(p->ren, 0);
    return p;
}

void plat_window_destroy(PlatWindow *p)
{
    if (!p)
        return;
    if (p->tex)
        SDL_DestroyTexture(p->tex);
    SDL_DestroyRenderer(p->ren);
    SDL_DestroyWindow(p->win);
    free(p);
}

void plat_window_show(PlatWindow *p, int show, int activate)
{
    if (show) {
        SDL_ShowWindow(p->win);
        if (activate)
            SDL_RaiseWindow(p->win);
    } else
        SDL_HideWindow(p->win);
}

void plat_window_minimize(PlatWindow *p)
{
    SDL_MinimizeWindow(p->win);
}

void plat_window_maximize(PlatWindow *p)
{
    SDL_MaximizeWindow(p->win);
}

void plat_window_restore(PlatWindow *p)
{
    SDL_RestoreWindow(p->win);
}

void plat_window_raise(PlatWindow *p)
{
    SDL_RaiseWindow(p->win);
}

void plat_window_set_title(PlatWindow *p, const char *title)
{
    SDL_SetWindowTitle(p->win, title);
}

void plat_window_set_rect(PlatWindow *p, int x, int y, int cw, int ch)
{
    int ox, oy, ow, oh;

    SDL_GetWindowPosition(p->win, &ox, &oy);
    SDL_GetWindowSize(p->win, &ow, &oh);
    if (ow != cw || oh != ch)
        SDL_SetWindowSize(p->win, cw, ch);
    if (ox != x || oy != y)
        SDL_SetWindowPosition(p->win, x, y);
}

void plat_window_set_minmax(PlatWindow *p, int minw, int minh, int maxw, int maxh)
{
    if (minw || minh)
        SDL_SetWindowMinimumSize(p->win, minw, minh);
    if (maxw || maxh)
        SDL_SetWindowMaximumSize(p->win, maxw, maxh);
}

void plat_window_present(PlatWindow *p, const Surface *fb, const Surface *overlay, int ox, int oy)
{
    if (!p->tex || p->tw != fb->w || p->th != fb->h) {
        if (p->tex)
            SDL_DestroyTexture(p->tex);
        p->tex = SDL_CreateTexture(p->ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, fb->w, fb->h);
        if (!p->tex)
            return;
        p->tw = fb->w;
        p->th = fb->h;
        SDL_SetTextureScaleMode(p->tex, SDL_SCALEMODE_NEAREST);
        SDL_SetRenderLogicalPresentation(p->ren, fb->w, fb->h, SDL_LOGICAL_PRESENTATION_STRETCH);
    }
    SDL_UpdateTexture(p->tex, NULL, fb->bits, fb->pitch);
    SDL_RenderClear(p->ren);
    SDL_RenderTexture(p->ren, p->tex, NULL, NULL);
    if (overlay) {
        SDL_Texture *t = SDL_CreateTexture(p->ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STATIC, overlay->w,
                                           overlay->h);

        if (t) {
            SDL_FRect d = {(float)ox, (float)oy, (float)overlay->w, (float)overlay->h};

            SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
            SDL_UpdateTexture(t, NULL, overlay->bits, overlay->pitch);
            SDL_RenderTexture(p->ren, t, NULL, &d);
            SDL_DestroyTexture(t);
        }
    }
    SDL_RenderPresent(p->ren);
}

int plat_window_snapshot(PlatWindow *p, const char *path)
{
    (void)p;
    (void)path;
    return 0;
}

int plat_window_is_key(PlatWindow *p)
{
    return SDL_GetKeyboardFocus() == p->win;
}

int plat_app_active(void)
{
    return SDL_GetKeyboardFocus() != NULL;
}

/* ---- Keys ---- */

static int vk_of(SDL_Keycode k, SDL_Scancode sc)
{
    if (k >= 'a' && k <= 'z')
        return (int)(k - 'a' + 'A');
    if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z)
        return 'A' + (sc - SDL_SCANCODE_A);
    if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_9)
        return '1' + (sc - SDL_SCANCODE_1);
    if (sc == SDL_SCANCODE_0)
        return '0';
    if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12)
        return VK_F1 + (sc - SDL_SCANCODE_F1);
    if (sc >= SDL_SCANCODE_KP_1 && sc <= SDL_SCANCODE_KP_9)
        return VK_NUMPAD0 + 1 + (sc - SDL_SCANCODE_KP_1);
    switch (sc) {
    case SDL_SCANCODE_KP_0: return VK_NUMPAD0;
    case SDL_SCANCODE_KP_PLUS: return VK_ADD;
    case SDL_SCANCODE_KP_MINUS: return VK_SUBTRACT;
    case SDL_SCANCODE_KP_MULTIPLY: return VK_MULTIPLY;
    case SDL_SCANCODE_KP_DIVIDE: return VK_DIVIDE;
    case SDL_SCANCODE_KP_PERIOD: return VK_DECIMAL;
    case SDL_SCANCODE_KP_ENTER: return VK_RETURN;
    case SDL_SCANCODE_RETURN: return VK_RETURN;
    case SDL_SCANCODE_ESCAPE: return VK_ESCAPE;
    case SDL_SCANCODE_BACKSPACE: return VK_BACK;
    case SDL_SCANCODE_TAB: return VK_TAB;
    case SDL_SCANCODE_SPACE: return VK_SPACE;
    case SDL_SCANCODE_LEFT: return VK_LEFT;
    case SDL_SCANCODE_RIGHT: return VK_RIGHT;
    case SDL_SCANCODE_UP: return VK_UP;
    case SDL_SCANCODE_DOWN: return VK_DOWN;
    case SDL_SCANCODE_HOME: return VK_HOME;
    case SDL_SCANCODE_END: return VK_END;
    case SDL_SCANCODE_PAGEUP: return VK_PRIOR;
    case SDL_SCANCODE_PAGEDOWN: return VK_NEXT;
    case SDL_SCANCODE_INSERT: return VK_INSERT;
    case SDL_SCANCODE_DELETE: return VK_DELETE;
    case SDL_SCANCODE_PAUSE: return VK_PAUSE;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return VK_SHIFT;
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL: return VK_CONTROL;
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT: return VK_MENU;
    case SDL_SCANCODE_MINUS: return 0xBD;
    case SDL_SCANCODE_EQUALS: return 0xBB;
    case SDL_SCANCODE_COMMA: return 0xBC;
    case SDL_SCANCODE_PERIOD: return 0xBE;
    case SDL_SCANCODE_SLASH: return 0xBF;
    case SDL_SCANCODE_SEMICOLON: return 0xBA;
    case SDL_SCANCODE_GRAVE: return 0xC0;
    case SDL_SCANCODE_LEFTBRACKET: return 0xDB;
    case SDL_SCANCODE_BACKSLASH: return 0xDC;
    case SDL_SCANCODE_RIGHTBRACKET: return 0xDD;
    case SDL_SCANCODE_APOSTROPHE: return 0xDE;
    default: return 0;
    }
}

static SDL_Scancode scancode_of(int vk)
{
    if (vk >= 'A' && vk <= 'Z')
        return (SDL_Scancode)(SDL_SCANCODE_A + vk - 'A');
    if (vk >= '1' && vk <= '9')
        return (SDL_Scancode)(SDL_SCANCODE_1 + vk - '1');
    if (vk >= VK_F1 && vk <= VK_F12)
        return (SDL_Scancode)(SDL_SCANCODE_F1 + vk - VK_F1);
    if (vk > VK_NUMPAD0 && vk <= VK_NUMPAD9)
        return (SDL_Scancode)(SDL_SCANCODE_KP_1 + vk - VK_NUMPAD0 - 1);
    switch (vk) {
    case '0': return SDL_SCANCODE_0;
    case VK_NUMPAD0: return SDL_SCANCODE_KP_0;
    case VK_LEFT: return SDL_SCANCODE_LEFT;
    case VK_RIGHT: return SDL_SCANCODE_RIGHT;
    case VK_UP: return SDL_SCANCODE_UP;
    case VK_DOWN: return SDL_SCANCODE_DOWN;
    case VK_RETURN: return SDL_SCANCODE_RETURN;
    case VK_ESCAPE: return SDL_SCANCODE_ESCAPE;
    case VK_SPACE: return SDL_SCANCODE_SPACE;
    case VK_TAB: return SDL_SCANCODE_TAB;
    case VK_BACK: return SDL_SCANCODE_BACKSPACE;
    }
    return SDL_SCANCODE_UNKNOWN;
}

static int mods_of(SDL_Keymod m)
{
    return ((m & SDL_KMOD_SHIFT) ? PM_SHIFT : 0) | ((m & SDL_KMOD_CTRL) ? PM_CTRL : 0) |
           ((m & SDL_KMOD_ALT) ? PM_ALT : 0) | ((m & SDL_KMOD_CAPS) ? PM_CAPS : 0) |
           ((m & SDL_KMOD_GUI) ? PM_CMD : 0);
}

int plat_key_down(int vk)
{
    const bool *ks;
    SDL_Scancode sc;

    SDL_PumpEvents();
    switch (vk) {
    case VK_SHIFT:
        return (plat_mods() & PM_SHIFT) != 0;
    case VK_CONTROL:
        return (plat_mods() & PM_CTRL) != 0;
    case VK_MENU:
        return (plat_mods() & PM_ALT) != 0;
    }
    ks = SDL_GetKeyboardState(NULL);
    sc = scancode_of(vk);
    return sc != SDL_SCANCODE_UNKNOWN && ks[sc];
}

int plat_mods(void)
{
    return mods_of(SDL_GetModState());
}

unsigned plat_mouse_buttons(void)
{
    float x, y;
    SDL_MouseButtonFlags b = SDL_GetGlobalMouseState(&x, &y);

    return ((b & SDL_BUTTON_LMASK) ? 1u : 0) | ((b & SDL_BUTTON_RMASK) ? 2u : 0) | ((b & SDL_BUTTON_MMASK) ? 4u : 0);
}

void plat_mouse_pos(int *x, int *y)
{
    float fx, fy;

    SDL_PumpEvents();
    SDL_GetGlobalMouseState(&fx, &fy);
    *x = (int)fx;
    *y = (int)fy;
}

void plat_set_mouse_pos(int x, int y)
{
    SDL_WarpMouseGlobal((float)x, (float)y);
}

void plat_capture_mouse(int on)
{
    SDL_CaptureMouse(on != 0);
}

int plat_key_char(int scancode, int mods)
{
    SDL_Keymod m = ((mods & PM_SHIFT) ? SDL_KMOD_SHIFT : 0) | ((mods & PM_CAPS) ? SDL_KMOD_CAPS : 0);

    return (int)SDL_GetKeyFromScancode((SDL_Scancode)scancode, m, false);
}

void plat_inject_mouse(PlatWindow *p, int x, int y, int button, int down, int clicks, int moveOnly)
{
    SDL_Event e;

    (void)button;
    memset(&e, 0, sizeof e);
    if (moveOnly) {
        e.type = SDL_EVENT_MOUSE_MOTION;
        e.motion.windowID = SDL_GetWindowID(p->win);
        e.motion.which = INJECTED;
        e.motion.x = (float)x;
        e.motion.y = (float)y;
    } else {
        e.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        e.button.windowID = SDL_GetWindowID(p->win);
        e.button.which = INJECTED;
        e.button.button = SDL_BUTTON_LEFT;
        e.button.down = down != 0;
        e.button.clicks = (Uint8)clicks;
        e.button.x = (float)x;
        e.button.y = (float)y;
    }
    SDL_PushEvent(&e);
}

void plat_inject_key(PlatWindow *p, const char *name, int down)
{
    SDL_Event e;
    SDL_Scancode sc = SDL_GetScancodeFromName(name);

    memset(&e, 0, sizeof e);
    e.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    e.key.windowID = SDL_GetWindowID(p->win);
    e.key.which = INJECTED;
    e.key.scancode = sc;
    e.key.key = SDL_GetKeyFromScancode(sc, SDL_KMOD_NONE, true);
    e.key.down = down != 0;
    SDL_PushEvent(&e);
}

/* ---- Events ---- */

static void handle(const SDL_Event *ev)
{
    Wnd *w;

    switch (ev->type) {
    case SDL_EVENT_QUIT:
        input_quit();
        break;
    case SDL_EVENT_MOUSE_MOTION:
        input_mouse_move(wnd_of(ev->motion.windowID), (int)ev->motion.x, (int)ev->motion.y,
                         ev->motion.which == INJECTED);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            int b = ev->button.button == SDL_BUTTON_LEFT ? 0 : ev->button.button == SDL_BUTTON_RIGHT ? 1
                  : ev->button.button == SDL_BUTTON_MIDDLE ? 2 : -1;

            input_mouse(wnd_of(ev->button.windowID), (int)ev->button.x, (int)ev->button.y, b,
                        ev->type == SDL_EVENT_MOUSE_BUTTON_DOWN, ev->button.clicks, ev->button.which == INJECTED);
        }
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        input_wheel((int)(ev->wheel.y * 120), ev->wheel.which == INJECTED);
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        input_key(vk_of(ev->key.key, ev->key.scancode), ev->key.scancode, ev->key.down, ev->key.repeat,
                  mods_of(ev->key.mod), ev->key.which == INJECTED);
        break;
    case SDL_EVENT_WINDOW_MOVED:
        input_window_moved(wnd_of(ev->window.windowID), ev->window.data1, ev->window.data2);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        input_window_resized(wnd_of(ev->window.windowID), ev->window.data1, ev->window.data2);
        break;
    case SDL_EVENT_WINDOW_MINIMIZED:
        input_window_state(wnd_of(ev->window.windowID), SIZE_MINIMIZED);
        break;
    case SDL_EVENT_WINDOW_MAXIMIZED:
        input_window_state(wnd_of(ev->window.windowID), SIZE_MAXIMIZED);
        break;
    case SDL_EVENT_WINDOW_RESTORED:
        input_window_state(wnd_of(ev->window.windowID), SIZE_RESTORED);
        break;
    case SDL_EVENT_WINDOW_EXPOSED:
        input_window_exposed(wnd_of(ev->window.windowID));
        break;
    case SDL_EVENT_WINDOW_SHOWN:
        w = wnd_of(ev->window.windowID);
        if (w && w->plat) {
            int x, y, cw, ch;

            if (SDL_GetWindowPosition(w->plat->win, &x, &y))
                input_window_moved(w, x, y);
            if (SDL_GetWindowSize(w->plat->win, &cw, &ch))
                input_window_resized(w, cw, ch);
        }
        break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        input_window_focus(wnd_of(ev->window.windowID));
        break;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        input_window_close(wnd_of(ev->window.windowID));
        break;
    default:
        break;
    }
}

int plat_pump(int timeout_ms)
{
    SDL_Event ev;
    int n = 0;

    if (timeout_ms > 0 ? SDL_WaitEventTimeout(&ev, timeout_ms) : SDL_PollEvent(&ev)) {
        do {
            handle(&ev);
            n++;
        } while (SDL_PollEvent(&ev));
    }
    if (!SDL_GetKeyboardFocus())
        input_app_deactivated();
    return n;
}

/* ---- Cursors ---- */

void *plat_cursor_system(int idc)
{
    SDL_SystemCursor sc;

    switch (idc) {
    case 32513: sc = SDL_SYSTEM_CURSOR_TEXT; break;
    case 32514: sc = SDL_SYSTEM_CURSOR_WAIT; break;
    case 32515: sc = SDL_SYSTEM_CURSOR_CROSSHAIR; break;
    case 32640: case 32646: sc = SDL_SYSTEM_CURSOR_MOVE; break;
    case 32642: sc = SDL_SYSTEM_CURSOR_NWSE_RESIZE; break;
    case 32643: sc = SDL_SYSTEM_CURSOR_NESW_RESIZE; break;
    case 32644: sc = SDL_SYSTEM_CURSOR_EW_RESIZE; break;
    case 32645: sc = SDL_SYSTEM_CURSOR_NS_RESIZE; break;
    case 32648: sc = SDL_SYSTEM_CURSOR_NOT_ALLOWED; break;
    default: sc = SDL_SYSTEM_CURSOR_DEFAULT; break;
    }
    return SDL_CreateSystemCursor(sc);
}

void *plat_cursor_image(const uint32_t *argb, int w, int h, int hx, int hy)
{
    SDL_Surface *s = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_ARGB8888, (void *)argb, w * 4);
    SDL_Cursor *c = s ? SDL_CreateColorCursor(s, hx, hy) : NULL;

    if (s)
        SDL_DestroySurface(s);
    return c;
}

void plat_cursor_set(void *c)
{
    if (!c) {
        SDL_HideCursor();
        return;
    }
    SDL_SetCursor((SDL_Cursor *)c);
    SDL_ShowCursor();
}

void plat_cursor_show(int show)
{
    if (show)
        SDL_ShowCursor();
    else
        SDL_HideCursor();
}

/* ---- The menu bar: drawn in the window by menu.c ---- */

int plat_menubar_native(void)
{
    return 0;
}

void plat_menubar_set(uint32_t hwnd, uint32_t hmenu)
{
    (void)hwnd;
    (void)hmenu;
}

int plat_menu_choose(const char *path)
{
    (void)path;
    return 0;
}

/* ---- Controls: drawn by the layer ---- */

PlatControl *plat_control_create(PlatWindow *top, uint32_t hwnd, int kind)
{
    (void)top;
    (void)hwnd;
    (void)kind;
    return NULL;
}

void plat_control_destroy(PlatControl *c) { (void)c; }
void plat_control_frame(PlatControl *c, int x, int y, int w, int h, int visible, int enabled)
{
    (void)c; (void)x; (void)y; (void)w; (void)h; (void)visible; (void)enabled;
}
void plat_control_text(PlatControl *c, const char *utf8) { (void)c; (void)utf8; }
void plat_control_check(PlatControl *c, int check) { (void)c; (void)check; }
void plat_control_focus(PlatWindow *top, PlatControl *c) { (void)top; (void)c; }
void plat_list_reload(PlatControl *c, int rows, int rowHeight) { (void)c; (void)rows; (void)rowHeight; }
void plat_list_select(PlatControl *c, int row) { (void)c; (void)row; }
void plat_list_top(PlatControl *c, int row) { (void)c; (void)row; }

/* ---- Sound ---- */

static SDL_AudioDeviceID g_dev;

int plat_audio_open(void)
{
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        return 0;
    g_dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    return g_dev != 0;
}

void plat_audio_close(void)
{
    if (g_dev)
        SDL_CloseAudioDevice(g_dev);
    g_dev = 0;
}

PlatStream *plat_stream_create(int rate, int bits, int channels)
{
    SDL_AudioSpec spec;
    SDL_AudioStream *s;

    spec.format = bits == 8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE;
    spec.channels = channels;
    spec.freq = rate;
    s = SDL_CreateAudioStream(&spec, NULL);
    if (s && !SDL_BindAudioStream(g_dev, s)) {
        SDL_DestroyAudioStream(s);
        s = NULL;
    }
    return (PlatStream *)s;
}

int plat_stream_put(PlatStream *s, const void *data, int len)
{
    if (!SDL_PutAudioStreamData((SDL_AudioStream *)s, data, len))
        return 0;
    /* a buffer is a whole sound: without a flush SDL keeps its last samples
       back for resampling, and the game would wait for its end for ever */
    SDL_FlushAudioStream((SDL_AudioStream *)s);
    return 1;
}

int plat_stream_queued(PlatStream *s)
{
    return SDL_GetAudioStreamQueued((SDL_AudioStream *)s);
}

void plat_stream_clear(PlatStream *s)
{
    SDL_ClearAudioStream((SDL_AudioStream *)s);
}

void plat_stream_gain(PlatStream *s, float gain)
{
    SDL_SetAudioStreamGain((SDL_AudioStream *)s, gain);
}

void plat_stream_destroy(PlatStream *s)
{
    SDL_DestroyAudioStream((SDL_AudioStream *)s);
}
