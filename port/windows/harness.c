/*
 * The test harness of the 64-bit Windows build: what the Wine harness's
 * logging PROCESS.DLL (tools/harness/proxy.c) does for the 32-bit game, built
 * into the program, since PROCESS is linked in here. With LEMMINGS_HARNESS=1
 * a thread runs the commands of CMD.TXT (re-read as it grows) and logs them
 * to CMDLOG.TXT, and every ProcessLemmings call appends the lemmings to
 * LEMLOG.TXT in the proxy's format, so tools/cmplog.py compares a run with
 * the reference logs of the original game. The files are in
 * LEMMINGS_HARNESS_DIR (default: the current directory, the game's data).
 *
 * Commands, as the proxy's: wait MS, waitupd N; click/rclick/dbl X Y (client
 * coordinates of the game's window or open dialog), hold X Y MS, move X Y,
 * key VK, menu NAME, cmd ID, keymenu C, ctl NAME, ctls, edit TEXT, size W H,
 * max, restore, shot NAME, wshot NAME, tree, windows NAME. The game polls
 * the pointer and keys (GetCursorPos, GetAsyncKeyState): in a scripted run
 * it sees only the harness's.
 *
 * Unlike the proxy's thread, this one cannot lose a race with the game: when
 * it waits for an update (waitupd, and the updates a click holds its button
 * for), the game stops at the end of that update until the thread has posted
 * what follows and waits again. The proxy posts while the game still draws
 * the frame, which lands before the next update only as long as drawing
 * takes longer than the thread needs; the reference logs were made that way,
 * so "before the next update" is what they expect.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "win64.h"

int port_harness;

static char g_dir[MAX_PATH] = ".";
static FILE *g_log, *g_cmdlog;
static volatile int g_update;
static POINT g_vcur;
static volatile unsigned char g_vkeys[256];

/* The gate between the harness thread and the game (see the top) */
static HANDLE g_reached, g_resume;      /* events: the game is at the update; go on */
static volatile LONG g_waitFor = -1;    /* the update count the thread waits for */
static int g_holding;                   /* the thread's: the game waits for release_game() */

static FILE *open_file(const char *name, const char *mode)
{
    char p[MAX_PATH + 80];

    snprintf(p, sizeof p, "%s\\%s", g_dir, name);
    return fopen(p, mode);
}

/* ---- The lemming log ---- */

/* PROCESS's ProcessLemmings, renamed by the build so this one wraps it */
int WINAPI process_ProcessLemmings(int *active);
extern int g_numLemmings;
extern void *g_lemmings[100];

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
            for (k = 0; k < 3; k++)
                memcpy(raw + 0x5c + 4 * k, r + 0x60 + 8 * k, 4);
            fprintf(g_log, "%d %d: %d %d raw ", g_update, i, ((const int *)r)[0], ((const int *)r)[1]);
            for (k = 0; k < 0x80; k++)
                fprintf(g_log, "%02x", raw[k]);
            fputc('\n', g_log);
        }
        fflush(g_log);
    }
    g_update++;
    /* the harness thread waits for this update: tell it, and wait until it
       has posted what follows (never long: sent messages still get through,
       and after a quarter of a second the game goes on regardless) */
    {
        LONG target = g_waitFor;

        if (target >= 0 && g_update >= target && InterlockedCompareExchange(&g_waitFor, -1, target) == target) {
            DWORD until = GetTickCount() + 250;
            MSG msg;

            ResetEvent(g_resume);       /* (a release that came after an earlier time-out) */
            SetEvent(g_reached);
            for (;;) {
                DWORD now = GetTickCount();

                if ((LONG)(until - now) <= 0 ||
                    MsgWaitForMultipleObjects(1, &g_resume, FALSE, until - now, QS_SENDMESSAGE) != WAIT_OBJECT_0 + 1)
                    break;
                PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
            }
        }
    }
    return ret;
}

/* The harness thread: let the game go on, if it is held. */
static void release_game(void)
{
    if (g_holding) {
        g_holding = 0;
        SetEvent(g_resume);
    }
}

/* The harness thread: until `target` updates have run, or `ms` have passed
   (INFINITE: never). If it had to wait, the game is then held at the end of
   that update until release_game(). */
static void wait_update(int target, DWORD ms)
{
    release_game();
    if (g_update >= target)
        return;
    InterlockedExchange(&g_waitFor, target);
    if (WaitForSingleObject(g_reached, ms) != WAIT_OBJECT_0) {
        if (InterlockedCompareExchange(&g_waitFor, -1, target) == target)
            return;                     /* timed out: the game never got there */
        WaitForSingleObject(g_reached, INFINITE);       /* it just did: its signal is coming */
    }
    g_holding = 1;
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

void harness_log(const char *fmt, ...)
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

/* ---- The game's windows ---- */

static BOOL CALLBACK pick_window(HWND w, LPARAM lp)
{
    DWORD p;
    RECT rc, b = { 0, 0, 0, 0 };
    HWND *best = (HWND *)lp;

    GetWindowThreadProcessId(w, &p);
    if (p != GetCurrentProcessId())
        return TRUE;
    GetClientRect(w, &rc);
    if (*best)
        GetClientRect(*best, &b);
    if (rc.right * rc.bottom > b.right * b.bottom)
        *best = w;
    return TRUE;
}

static BOOL CALLBACK pick_dialog(HWND w, LPARAM lp)
{
    DWORD p;
    char cls[16];

    GetWindowThreadProcessId(w, &p);
    if (p == GetCurrentProcessId() && IsWindowVisible(w) && GetClassNameA(w, cls, sizeof cls) &&
        !strcmp(cls, "#32770")) {
        *(HWND *)lp = w;
        return FALSE;
    }
    return TRUE;
}

/* The game's largest top-level window (the frame). */
static HWND frame_window(void)
{
    HWND found = NULL;

    EnumWindows(pick_window, (LPARAM)&found);
    return found;
}

/* The game's open dialog if there is one, else the frame. */
static HWND game_window(void)
{
    HWND found = NULL;

    EnumWindows(pick_dialog, (LPARAM)&found);
    return found ? found : frame_window();
}

/* The innermost child window at a client point of w; the point in it. */
static HWND child_at(HWND w, POINT *pt)
{
    for (;;) {
        HWND ch = ChildWindowFromPointEx(w, *pt, CWP_SKIPINVISIBLE);

        if (!ch || ch == w)
            return w;
        MapWindowPoints(w, ch, pt, 1);
        w = ch;
    }
}

/* ---- Pictures ---- */

static void save_dc(HDC src, int x, int y, int w, int h, const char *name)
{
    HDC mdc = CreateCompatibleDC(src);
    HBITMAP bm = CreateCompatibleBitmap(src, w, h);
    BITMAPINFOHEADER bi = { sizeof bi, w, h, 1, 24, BI_RGB, 0, 0, 0, 0, 0 };
    int stride = (w * 3 + 3) & ~3;
    char *px = malloc((size_t)stride * h);
    char fn[80];
    FILE *f;

    SelectObject(mdc, bm);
    BitBlt(mdc, 0, 0, w, h, src, x, y, SRCCOPY);
    GetDIBits(mdc, bm, 0, h, px, (BITMAPINFO *)&bi, DIB_RGB_COLORS);
    snprintf(fn, sizeof fn, "%s.BMP", name);
    f = open_file(fn, "wb");
    if (f) {
        BITMAPFILEHEADER fh = { 0x4D42, (DWORD)(sizeof fh + sizeof bi + stride * h), 0, 0, sizeof fh + sizeof bi };

        fwrite(&fh, sizeof fh, 1, f);
        fwrite(&bi, sizeof bi, 1, f);
        fwrite(px, (size_t)stride * h, 1, f);
        fclose(f);
    }
    free(px);
    DeleteObject(bm);
    DeleteDC(mdc);
}

/* "shot NAME": the window's client area as NAME.BMP */
static void shot(HWND w, const char *name)
{
    RECT rc;
    HDC dc;

    GetClientRect(w, &rc);
    if (rc.right <= 0 || rc.bottom <= 0)
        return;
    dc = GetDC(w);
    save_dc(dc, 0, 0, rc.right, rc.bottom, name);
    ReleaseDC(w, dc);
}

/* "wshot NAME": the whole window, frame and menu bar included */
static void wshot(HWND w, const char *name)
{
    RECT rc;
    HDC dc;

    GetWindowRect(w, &rc);
    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return;
    dc = GetWindowDC(w);
    save_dc(dc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, name);
    ReleaseDC(w, dc);
}

/* ---- Lists for the command log ---- */

/* "tree": every window of the process, children indented, and the frame's
   menu with its items' states */
static HWND g_treeTop;

static void tree_one(HWND w, int depth)
{
    char title[128] = "", cls[64] = "";
    RECT r, c;
    POINT a, b;
    HWND ch;

    GetWindowTextA(w, title, sizeof title);
    GetClassNameA(w, cls, sizeof cls);
    GetWindowRect(w, &r);
    GetClientRect(w, &c);
    a.x = r.left, a.y = r.top, b.x = r.right, b.y = r.bottom;
    if (depth) {
        ScreenToClient(g_treeTop, &a);
        ScreenToClient(g_treeTop, &b);
    }
    harness_log("  %*svis %d id %ld class '%s' style %#lx ex %#lx text '%s' rect %ld %ld %ld %ld client %ld %ld",
                depth * 2, "", IsWindowVisible(w), (long)GetWindowLongA(w, GWL_ID), cls,
                (long)GetWindowLongA(w, GWL_STYLE), (long)GetWindowLongA(w, GWL_EXSTYLE), title,
                a.x, a.y, b.x, b.y, c.right, c.bottom);
    for (ch = GetWindow(w, GW_CHILD); ch; ch = GetWindow(ch, GW_HWNDNEXT))
        tree_one(ch, depth + 1);
}

static void tree_menu(HMENU m, int depth)
{
    int i, n = GetMenuItemCount(m);

    for (i = 0; i < n; i++) {
        char t[128] = "";
        HMENU sub = GetSubMenu(m, i);

        GetMenuStringA(m, i, t, sizeof t, MF_BYPOSITION);
        harness_log("  %*smenu '%s' id %d state %#x", depth * 2, "", t, (int)GetMenuItemID(m, i),
                    GetMenuState(m, i, MF_BYPOSITION));
        if (sub)
            tree_menu(sub, depth + 1);
    }
}

static BOOL CALLBACK tree_top_window(HWND w, LPARAM lp)
{
    DWORD p;

    (void)lp;
    GetWindowThreadProcessId(w, &p);
    if (p != GetCurrentProcessId())
        return TRUE;
    g_treeTop = w;
    tree_one(w, 0);
    if (GetMenu(w))
        tree_menu(GetMenu(w), 2);
    return TRUE;
}

/* "windows NAME": the process's top-level windows, each visible one saved
   as NAME_k.BMP */
static const char *g_listName;
static int g_listCount;

static BOOL CALLBACK list_window(HWND w, LPARAM lp)
{
    DWORD p;
    char title[128] = "", cls[64] = "";
    RECT r;
    int k;

    (void)lp;
    GetWindowThreadProcessId(w, &p);
    if (p != GetCurrentProcessId())
        return TRUE;
    GetWindowTextA(w, title, sizeof title);
    GetClassNameA(w, cls, sizeof cls);
    GetWindowRect(w, &r);
    k = g_listCount++;
    harness_log("  window %d vis %d class '%s' title '%s' rect %ld %ld %ld %ld", k, IsWindowVisible(w), cls, title,
                r.left, r.top, r.right, r.bottom);
    if (IsWindowVisible(w)) {
        char nm[160];

        snprintf(nm, sizeof nm, "%s_%d", g_listName, k);
        shot(w, nm);
    }
    return TRUE;
}

/* Text without '&' (menu and button accelerators). */
static void plain(char *t)
{
    char *o = t, *i;

    for (i = t; *i; i++)
        if (*i != '&')
            *o++ = *i;
    *o = 0;
}

/* "menu NAME": the frame's menu item whose text starts with NAME */
static int find_menu(HMENU m, const char *name)
{
    int i, n = GetMenuItemCount(m);

    for (i = 0; i < n; i++) {
        char t[128] = "";
        HMENU sub = GetSubMenu(m, i);

        GetMenuStringA(m, i, t, sizeof t, MF_BYPOSITION);
        plain(t);
        if (sub) {
            int id = find_menu(sub, name);

            if (id)
                return id;
        } else if (!strncmp(t, name, strlen(name)))
            return (int)GetMenuItemID(m, i);
    }
    return 0;
}

/* "ctl NAME": the dialog control whose text starts with NAME, clicked;
   "ctls": the dialog's controls (class, id, text, checked) */
static const char *g_ctlName;
static HWND g_ctlFound;

static BOOL CALLBACK each_ctl(HWND c, LPARAM lp)
{
    char t[128] = "", cls[64] = "";

    (void)lp;
    GetWindowTextA(c, t, sizeof t);
    GetClassNameA(c, cls, sizeof cls);
    plain(t);
    if (g_ctlName) {
        if (!g_ctlFound && t[0] && !strncmp(t, g_ctlName, strlen(g_ctlName)))
            g_ctlFound = c;
    } else
        harness_log("  ctl class '%s' id %ld text '%s' checked %ld", cls, (long)GetWindowLongA(c, GWL_ID), t,
                    (long)SendMessageA(c, BM_GETCHECK, 0, 0));
    return TRUE;
}

/* ---- The commands ---- */

/* until n more updates have run (menus: no updates, about 2 s at most) */
static void hold_updates(int n)
{
    wait_update(g_update + n, 2000);
}

/* Commands that only post messages keep the game held meanwhile; the rest
   (they wait, or call into the game's windows) let it go on first. */
static int posts_only(const char *line)
{
    static const char *const names[] = { "click ", "rclick ", "dbl ", "hold ", "move ", "key ", "keymenu ",
                                         "menu ", "cmd ", "ctl " };
    size_t i;

    for (i = 0; i < sizeof names / sizeof names[0]; i++)
        if (!strncmp(line, names[i], strlen(names[i])))
            return 1;
    return 0;
}

static void run_command(const char *line)
{
    HWND w;
    char a[64];
    int x = 0, y = 0, ms = 0;

    if (!posts_only(line))
        release_game();
    w = game_window();
    if (sscanf(line, "shot %63s", a) == 1 && w)
        shot(w, a);
    else if (sscanf(line, "wshot %63s", a) == 1 && w)
        wshot(w, a);
    else if (sscanf(line, "edit %63[^\r\n]", a) == 1 && w) {
        HWND ed = FindWindowExA(w, NULL, "Edit", NULL);

        if (ed)
            SetWindowTextA(ed, a);
        harness_log("  edit %s '%s'", ed ? "set" : "not found", a);
    } else if (sscanf(line, "size %d %d", &x, &y) == 2 && w)
        SetWindowPos(w, NULL, 0, 0, x, y, SWP_NOMOVE | SWP_NOZORDER);
    else if (sscanf(line, "cmd %d", &x) == 1) {
        HWND f = frame_window();

        if (f)
            PostMessageA(f, WM_COMMAND, x, 0);
    } else if (sscanf(line, "keymenu %63s", a) == 1 && w)
        PostMessageA(w, WM_SYSCOMMAND, SC_KEYMENU, a[0]);
    else if (!strncmp(line, "max", 3) && w)
        ShowWindow(w, SW_MAXIMIZE);
    else if (!strncmp(line, "restore", 7) && w)
        ShowWindow(w, SW_RESTORE);
    else if (!strncmp(line, "tree", 4))
        EnumWindows(tree_top_window, 0);
    else if (sscanf(line, "dbl %d %d", &x, &y) == 2 && w) {
        POINT pt = { x, y };
        HWND t = child_at(w, &pt);
        LPARAM lp = MAKELPARAM(pt.x, pt.y);

        PostMessageA(t, WM_LBUTTONDOWN, MK_LBUTTON, lp);
        PostMessageA(t, WM_LBUTTONUP, 0, lp);
        PostMessageA(t, WM_LBUTTONDBLCLK, MK_LBUTTON, lp);
        PostMessageA(t, WM_LBUTTONUP, 0, lp);
        harness_log("  -> dbl at %ld %ld", pt.x, pt.y);
    } else if ((sscanf(line, "click %d %d", &x, &y) == 2 || sscanf(line, "rclick %d %d", &x, &y) == 2) && w) {
        /* the pointer there (screen), the button held for two updates, and
           the messages to the innermost child window */
        int right = line[0] == 'r';
        POINT sp = { x, y }, pt = { x, y };
        HWND t;
        LPARAM lp;

        ClientToScreen(w, &sp);
        g_vcur = sp;
        t = child_at(w, &pt);
        lp = MAKELPARAM(pt.x, pt.y);
        PostMessageA(t, WM_MOUSEMOVE, 0, lp);
        hold_updates(1);
        g_vkeys[right ? VK_RBUTTON : VK_LBUTTON] = 1;
        PostMessageA(t, right ? WM_RBUTTONDOWN : WM_LBUTTONDOWN, right ? MK_RBUTTON : MK_LBUTTON, lp);
        hold_updates(2);
        g_vkeys[right ? VK_RBUTTON : VK_LBUTTON] = 0;
        PostMessageA(t, right ? WM_RBUTTONUP : WM_LBUTTONUP, 0, lp);
        harness_log("  -> at %ld %ld (screen %ld %ld)", pt.x, pt.y, sp.x, sp.y);
    } else if (sscanf(line, "hold %d %d %d", &x, &y, &ms) == 3 && w) {
        POINT sp = { x, y }, pt = { x, y };
        HWND t;
        LPARAM lp;

        ClientToScreen(w, &sp);
        g_vcur = sp;
        t = child_at(w, &pt);
        lp = MAKELPARAM(pt.x, pt.y);
        g_vkeys[VK_LBUTTON] = 1;
        PostMessageA(t, WM_LBUTTONDOWN, MK_LBUTTON, lp);
        release_game();
        Sleep(ms);
        g_vkeys[VK_LBUTTON] = 0;
        PostMessageA(t, WM_LBUTTONUP, 0, lp);
    } else if (sscanf(line, "move %d %d", &x, &y) == 2 && w) {
        POINT sp = { x, y };

        ClientToScreen(w, &sp);
        g_vcur = sp;
    } else if (sscanf(line, "key %d", &x) == 1 && w && x > 0 && x < 256) {
        g_vkeys[x] = 1;
        PostMessageA(w, WM_KEYDOWN, x, 1);
        hold_updates(2);
        g_vkeys[x] = 0;
        PostMessageA(w, WM_KEYUP, x, 0xC0000001);
    } else if (sscanf(line, "wait %d", &x) == 1)
        Sleep(x);
    else if (sscanf(line, "menu %63[^\r\n]", a) == 1) {
        HWND f = frame_window();
        int id = f && GetMenu(f) ? find_menu(GetMenu(f), a) : 0;

        if (id)
            PostMessageA(f, WM_COMMAND, id, 0);
        harness_log("  menu '%s' id %d", a, id);
    } else if (!strncmp(line, "ctls", 4) && w) {
        g_ctlName = NULL;
        EnumChildWindows(w, each_ctl, 0);
    } else if (sscanf(line, "ctl %63[^\r\n]", a) == 1 && w) {
        g_ctlName = a;
        g_ctlFound = NULL;
        EnumChildWindows(w, each_ctl, 0);
        if (g_ctlFound)
            PostMessageA(g_ctlFound, BM_CLICK, 0, 0);
        harness_log("  ctl '%s' %s", a, g_ctlFound ? "clicked" : "not found");
        g_ctlName = NULL;
    } else if (sscanf(line, "windows %63s", a) == 1) {
        g_listName = a;
        g_listCount = 0;
        EnumWindows(list_window, 0);
    } else if (sscanf(line, "waitupd %d", &x) == 1)
        wait_update(x, INFINITE);
    if (g_cmdlog) {
        fprintf(g_cmdlog, "%s update %d: %s", w ? "ok" : "no window", g_update, line);
        if (!strchr(line, '\n'))
            fputc('\n', g_cmdlog);
        fflush(g_cmdlog);
    }
}

static DWORD WINAPI harness_thread(LPVOID p)
{
    long done = 0;

    (void)p;
    for (;;) {
        FILE *f;
        char line[256];
        long n = 0;

        release_game();                 /* no more commands for now */
        Sleep(200);
        f = open_file("CMD.TXT", "r");
        if (!f)
            continue;
        while (fgets(line, sizeof line, f)) {
            if (n++ < done)
                continue;
            done = n;
            run_command(line);
        }
        fclose(f);
    }
    return 0;
}

void harness_start(void)
{
    const char *dir = getenv("LEMMINGS_HARNESS_DIR");

    if (dir && *dir && strlen(dir) < sizeof g_dir)
        strcpy(g_dir, dir);
    g_cmdlog = open_file("CMDLOG.TXT", "w");
    g_reached = CreateEventA(NULL, FALSE, FALSE, NULL);
    g_resume = CreateEventA(NULL, FALSE, FALSE, NULL);
    CreateThread(NULL, 0, harness_thread, NULL, 0, NULL);
}
