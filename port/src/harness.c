/*
 * The test harness of the native build, the counterpart of the Wine
 * harness's logging PROCESS.DLL (tools/harness/proxy.c). With
 * LEMMINGS_HARNESS=1 it runs the commands of CMD.TXT (wait MS, waitupd N,
 * click/rclick/dbl/move X Y, key VK, menu NAME, cmd ID, ctl NAME, edit TEXT,
 * shot NAME) from the message loop, logs them to CMDLOG.TXT, and after every
 * ProcessLemmings call writes the lemmings to LEMLOG.TXT in the proxy's
 * format, so tools/cmplog.py compares a native run with the reference logs
 * of the original game. Files are in LEMMINGS_HARNESS_DIR (default: the
 * current directory). The game sees only the harness's pointer and buttons;
 * real input is ignored and message boxes answer themselves.
 */
#include "port_internal.h"

int port_harness;

static char g_dir[1024] = ".";
static FILE *g_log, *g_cmdlog;
static volatile int g_update;
static long g_done;
static POINT g_vcur;
static unsigned char g_vkeys[256];

/* ---- The lemming log ---- */

/* PROCESS.DLL's ProcessLemmings, renamed by the build so this one wraps it */
int WINAPI process_ProcessLemmings(int *active);
extern int g_numLemmings;
extern void *g_lemmings[100];

static FILE *open_file(const char *name, const char *mode)
{
    char p[1200];

    snprintf(p, sizeof p, "%s/%s", g_dir, name);
    return fopen(p, mode);
}

int WINAPI ProcessLemmings(int *active)
{
    int ret = process_ProcessLemmings(active), n, i, k;

    if (!port_harness)
        return ret;
    if (!g_log)
        g_log = open_file("LEMLOG.TXT", "w");
    if (g_log) {
        n = g_numLemmings;
        fprintf(g_log, "%d n %d arg %d ret %d\n", g_update, n, 0, ret);
        for (i = 0; i < n && i < 4; i++) {
            const unsigned char *r = g_lemmings[i];
            unsigned char raw[0x80];

            /* the 32-bit record: the same fields to 0x5c, then the three
               pointers cut to 32 bits; the rest was heap in the original */
            memset(raw, 0, sizeof raw);
            memcpy(raw, r, 0x5c);
            for (k = 0; k < 3; k++) {
                uint64_t p;

                memcpy(&p, r + 0x60 + 8 * k, 8);
                raw[0x5c + 4 * k] = (unsigned char)p;
                raw[0x5d + 4 * k] = (unsigned char)(p >> 8);
                raw[0x5e + 4 * k] = (unsigned char)(p >> 16);
                raw[0x5f + 4 * k] = (unsigned char)(p >> 24);
            }
            fprintf(g_log, "%d %d: %d %d raw ", g_update, i, ((const int *)r)[0], ((const int *)r)[1]);
            for (k = 0; k < 0x80; k++)
                fprintf(g_log, "%02x", raw[k]);
            fputc('\n', g_log);
        }
        fflush(g_log);
    }
    g_update++;
    return ret;
}

void harness_update(void)
{
}

/* ---- Input the game sees ---- */

int harness_cursor(POINT *pt)
{
    *pt = g_vcur;
    return 1;
}

int harness_key(int vk)
{
    return vk >= 0 && vk < 256 && g_vkeys[vk];
}

void port_harness_log(const char *fmt, ...)
{
    va_list ap;

    if (!g_cmdlog)
        return;
    va_start(ap, fmt);
    vfprintf(g_cmdlog, fmt, ap);
    va_end(ap);
    fputc('\n', g_cmdlog);
    fflush(g_cmdlog);
}

/* ---- Windows ---- */

static Wnd *game_window(void)
{
    Wnd *w, *best = NULL;
    int bestArea = -1;

    for (w = port_top; w; w = w->next)
        if (!w->destroyed && wnd_visible(w) && w->dlg)
            return w;
    for (w = port_top; w; w = w->next) {
        RECT r;

        if (w->destroyed || !wnd_visible(w) || w == port_desktop)
            continue;
        GetClientRect(w->h, &r);
        if (r.right * r.bottom > bestArea) {
            bestArea = r.right * r.bottom;
            best = w;
        }
    }
    return best;
}

/* the innermost visible child at a client point, and the point in it */
static HWND child_at(Wnd *w, POINT *pt)
{
    HWND t = w->h;

    for (;;) {
        Wnd *tw = wnd_get(t), *c, *hit = NULL;

        for (c = tw->child; c; c = c->next)
            if ((c->style & WS_VISIBLE) && pt->x >= c->rc.left && pt->x < c->rc.right && pt->y >= c->rc.top &&
                pt->y < c->rc.bottom) {
                hit = c;
                break;
            }
        if (!hit)
            return t;
        MapWindowPoints(t, hit->h, pt, 1);
        t = hit->h;
    }
}

static void shot(Wnd *w, const char *name)
{
    Wnd *top = wnd_toplevel(w);
    RECT r;
    char fn[256];
    FILE *f;
    int x, y, W, H, stride;
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER bi;

    wnd_client_fb(w, &r);
    W = r.right - r.left;
    H = r.bottom - r.top;
    if (W <= 0 || H <= 0)
        return;
    stride = (W * 3 + 3) & ~3;
    snprintf(fn, sizeof fn, "%s.BMP", name);
    f = open_file(fn, "wb");
    if (!f)
        return;
    memset(&fh, 0, sizeof fh);
    memset(&bi, 0, sizeof bi);
    fh.bfType = 0x4d42;
    fh.bfOffBits = sizeof fh + sizeof bi;
    fh.bfSize = fh.bfOffBits + (DWORD)(stride * H);
    bi.biSize = sizeof bi;
    bi.biWidth = W;
    bi.biHeight = H;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    fwrite(&fh, sizeof fh, 1, f);
    fwrite(&bi, sizeof bi, 1, f);
    for (y = H - 1; y >= 0; y--) {
        unsigned char row[8192 * 3];
        const uint32_t *p = (const uint32_t *)(top->fb.bits + (size_t)(r.top + y) * top->fb.pitch) + r.left;

        memset(row, 0, (size_t)stride);
        for (x = 0; x < W && x < 8192; x++) {
            row[x * 3] = (unsigned char)p[x];
            row[x * 3 + 1] = (unsigned char)(p[x] >> 8);
            row[x * 3 + 2] = (unsigned char)(p[x] >> 16);
        }
        fwrite(row, 1, (size_t)stride, f);
    }
    fclose(f);
}

static void plain(char *t)
{
    char *o = t, *i;

    for (i = t; *i; i++)
        if (*i != '&')
            *o++ = *i;
    *o = 0;
}

static int find_menu(HMENU m, const char *name)
{
    int i, n = GetMenuItemCount(m);

    for (i = 0; i < n; i++) {
        char t[128] = "";
        HMENU sub;

        GetMenuStringA(m, (UINT)i, t, sizeof t, MF_BYPOSITION);
        plain(t);
        sub = GetSubMenu(m, i);
        if (sub) {
            int id = find_menu(sub, name);

            if (id)
                return id;
        } else if (!strncmp(t, name, strlen(name)))
            return (int)GetMenuItemID(m, i);
    }
    return 0;
}

static Wnd *largest(void)
{
    Wnd *w, *best = NULL;
    int bestArea = -1;

    for (w = port_top; w; w = w->next) {
        RECT r;

        if (w->destroyed || w == port_desktop || w->dlg)
            continue;
        GetClientRect(w->h, &r);
        if (r.right * r.bottom > bestArea) {
            bestArea = r.right * r.bottom;
            best = w;
        }
    }
    return best;
}

/* ---- Commands ---- */

enum { IDLE, WAIT_MS, WAIT_UPD, HOLD };

static int g_state = IDLE;
static DWORD g_untilMs;
static int g_untilUpd;
static void (*g_then)(void);
static char g_line[256];
static HWND g_target;
static LPARAM g_lp;
static int g_button, g_key;

static void log_line(Wnd *w)
{
    if (g_cmdlog) {
        fprintf(g_cmdlog, "%s window %p update %d: %s", w ? "ok" : "no", w ? (void *)w->h : NULL, g_update, g_line);
        if (!strchr(g_line, '\n'))
            fputc('\n', g_cmdlog);
        fflush(g_cmdlog);
    }
}

static void hold(int updates, void (*then)(void))
{
    g_state = HOLD;
    g_untilUpd = g_update + updates;
    g_untilMs = port_ticks() + 2000;    /* menus and dialogs: no updates */
    g_then = then;
}

static void click_up(void)
{
    g_vkeys[g_button == 2 ? VK_RBUTTON : VK_LBUTTON] = 0;
    PostMessageA(g_target, g_button == 2 ? WM_RBUTTONUP : WM_LBUTTONUP, 0, g_lp);
    log_line(game_window());
}

static void click_down(void)
{
    g_vkeys[g_button == 2 ? VK_RBUTTON : VK_LBUTTON] = 1;
    PostMessageA(g_target, g_button == 2 ? WM_RBUTTONDOWN : WM_LBUTTONDOWN, g_button == 2 ? MK_RBUTTON : MK_LBUTTON,
                 g_lp);
    hold(2, click_up);
}

static void key_up(void)
{
    g_vkeys[g_key] = 0;
    PostMessageA(g_target, WM_KEYUP, (WPARAM)g_key, (LPARAM)0xC0000001u);
    log_line(game_window());
}

static void run(char *line)
{
    Wnd *w = game_window();
    char a[200];
    int x = 0, y = 0;

    SDL_strlcpy(g_line, line, sizeof g_line);
    if (sscanf(line, "wait %d", &x) == 1 && strncmp(line, "waitupd", 7)) {
        g_state = WAIT_MS;
        g_untilMs = port_ticks() + (DWORD)x;
        return;
    }
    if (sscanf(line, "waitupd %d", &x) == 1) {
        g_state = WAIT_UPD;
        g_untilUpd = x;
        return;
    }
    if (w && ((sscanf(line, "click %d %d", &x, &y) == 2) || (sscanf(line, "rclick %d %d", &x, &y) == 2))) {
        POINT sp = {x, y}, pt = {x, y};

        ClientToScreen(w->h, &sp);
        g_vcur = sp;
        g_target = child_at(w, &pt);
        g_lp = MAKELPARAM(pt.x, pt.y);
        g_button = line[0] == 'r' ? 2 : 1;
        PostMessageA(g_target, WM_MOUSEMOVE, 0, g_lp);
        port_harness_log("  -> window %p at %ld %ld (screen %ld %ld)", (void *)g_target, (long)pt.x, (long)pt.y,
                         (long)sp.x, (long)sp.y);
        hold(1, click_down);
        return;
    }
    if (w && sscanf(line, "dbl %d %d", &x, &y) == 2) {
        POINT pt = {x, y};
        HWND t = child_at(w, &pt);
        LPARAM lp = MAKELPARAM(pt.x, pt.y);

        PostMessageA(t, WM_LBUTTONDOWN, MK_LBUTTON, lp);
        PostMessageA(t, WM_LBUTTONUP, 0, lp);
        PostMessageA(t, WM_LBUTTONDBLCLK, MK_LBUTTON, lp);
        PostMessageA(t, WM_LBUTTONUP, 0, lp);
        port_harness_log("  -> dbl window %p at %ld %ld", (void *)t, (long)pt.x, (long)pt.y);
    } else if (w && sscanf(line, "move %d %d", &x, &y) == 2) {
        POINT sp = {x, y};

        ClientToScreen(w->h, &sp);
        g_vcur = sp;
    } else if (w && sscanf(line, "key %d", &x) == 1 && x > 0 && x < 256) {
        g_key = x;
        g_target = w->h;
        g_vkeys[x] = 1;
        PostMessageA(w->h, WM_KEYDOWN, (WPARAM)x, 1);
        hold(2, key_up);
        return;
    } else if (w && sscanf(line, "shot %199s", a) == 1)
        shot(w, a);
    else if (sscanf(line, "cmd %d", &x) == 1) {
        Wnd *f = largest();

        if (f)
            PostMessageA(f->h, WM_COMMAND, (WPARAM)x, 0);
    } else if (sscanf(line, "menu %199[^\n]", a) == 1) {
        Wnd *f = largest();
        int id = f && f->menu ? find_menu(f->menu, a) : 0;

        if (id)
            PostMessageA(f->h, WM_COMMAND, (WPARAM)id, 0);
        port_harness_log("  menu '%s' id %d", a, id);
    } else if (w && sscanf(line, "edit %199[^\n]", a) == 1) {
        Wnd *c;

        for (c = w->child; c; c = c->next)
            if (!SDL_strcasecmp(c->cls->name, "Edit")) {
                SetWindowTextA(c->h, a);
                break;
            }
        port_harness_log("  edit %p '%s'", c ? (void *)c->h : NULL, a);
    } else if (w && !strncmp(line, "ctls", 4)) {
        Wnd *c;

        for (c = w->child; c; c = c->next) {
            char t[128];

            SDL_strlcpy(t, c->text, sizeof t);
            plain(t);
            port_harness_log("  ctl %p class '%s' text '%s' checked %ld", (void *)c->h, c->cls->name, t,
                             (long)SendMessageA(c->h, BM_GETCHECK, 0, 0));
        }
    } else if (w && sscanf(line, "ctl %199[^\n]", a) == 1) {
        Wnd *c, *found = NULL;

        for (c = w->child; c && !found; c = c->next) {
            char t[128];

            SDL_strlcpy(t, c->text, sizeof t);
            plain(t);
            if (t[0] && !strncmp(t, a, strlen(a)))
                found = c;
        }
        if (found)
            PostMessageA(found->h, BM_CLICK, 0, 0);
        port_harness_log("  ctl '%s' %p", a, found ? (void *)found->h : NULL);
    }
    log_line(w);
}

void harness_poll(void)
{
    static int busy;
    char line[256];
    FILE *f;
    long n;

    if (busy)
        return;
    busy = 1;
    switch (g_state) {
    case WAIT_MS:
        if ((int)(port_ticks() - g_untilMs) < 0)
            goto out;
        log_line(game_window());
        break;
    case WAIT_UPD:
        if (g_update < g_untilUpd)
            goto out;
        log_line(game_window());
        break;
    case HOLD:
        if (g_update < g_untilUpd && (int)(port_ticks() - g_untilMs) < 0)
            goto out;
        g_state = IDLE;
        g_then();
        if (g_state != IDLE)
            goto out;
        break;
    }
    g_state = IDLE;
    f = open_file("CMD.TXT", "r");
    if (!f)
        goto out;
    for (n = 0; fgets(line, sizeof line, f);) {
        if (n++ < g_done)
            continue;
        g_done = n;
        run(line);
        if (g_state != IDLE)
            break;
    }
    fclose(f);
out:
    busy = 0;
}

void harness_init(void)
{
    const char *e = SDL_getenv("LEMMINGS_HARNESS");

    port_harness = e && SDL_atoi(e);
    if (!port_harness)
        return;
    if (SDL_getenv("LEMMINGS_HARNESS_DIR"))
        SDL_strlcpy(g_dir, SDL_getenv("LEMMINGS_HARNESS_DIR"), sizeof g_dir);
    g_cmdlog = open_file("CMDLOG.TXT", "w");
}
