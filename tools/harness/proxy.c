/* Logging stand-in for Lemmings for Windows' PROCESS.DLL: every export is
 * forwarded to the original (renamed PROCESS0.DLL, see process.def), except
 * ProcessLemmings, which runs the original's and then appends each lemming
 * record (pointer table at +0x9018, count at +0x112dc) to LEMLOG.TXT. A
 * rebuilt PROCESS0.DLL laid out differently (the MinGW build) exports the
 * table and count as g_lemmings and g_numLemmings instead. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef int (__stdcall *ProcFn)(int);
static ProcFn real_process;
static HMODULE real;
static FILE *logf;
static volatile int update;

/* Every sound the game asks for, with the time, to SNDLOG.TXT. */
static FILE *sndlog;
typedef struct { int x, y; } Pnt;
typedef void (__stdcall *SndFn)(Pnt, int);
__declspec(dllexport) void __stdcall SoundPlayFX(Pnt p, int id)
{
    static SndFn real_snd;
    if (!real_snd) real_snd = (SndFn)GetProcAddress(GetModuleHandleA("PROCESS0.DLL"), "?SoundPlayFX@@YGXUtagPNT@@H@Z");
    if (!sndlog) sndlog = fopen("SNDLOG.TXT", "w");
    if (sndlog) fprintf(sndlog, "%lu sound %d\n", GetTickCount(), id), fflush(sndlog);
    real_snd(p, id);
}

__declspec(dllexport) int __stdcall ProcessLemmings(int a)
{
    if (!real) {
        real = GetModuleHandleA("PROCESS0.DLL");
        real_process = (ProcFn)GetProcAddress(real, "_ProcessLemmings@4");
#ifndef PLAY
        logf = fopen("LEMLOG.TXT", "w");
#endif
    }
    int ret = real_process(a);
    if (!logf) return ret;
    char *base = (char *)real;
    int *count = (int *)GetProcAddress(real, "g_numLemmings");
    unsigned char **tab = (unsigned char **)GetProcAddress(real, "g_lemmings");
    if (!count || !tab) {
        count = (int *)(base + 0x112dc);
        tab = (unsigned char **)(base + 0x9018);
    }
    int n = *count;
    fprintf(logf, "%d n %d arg %d ret %d\n", update, n, a, ret);
    for (int i = 0; i < n && i < 4; i++) {
        unsigned char *r = tab[i];
        fprintf(logf, "%d %d: %d %d raw ", update, i, ((int *)r)[0], ((int *)r)[1]);
        for (int k = 0; k < 0x80; k++) fprintf(logf, "%02x", r[k]);
        fputc('\n', logf);
    }
    fflush(logf);
    update++;
    return ret;
}

/* Test harness: a thread that runs commands from CMD.TXT (re-read as it
 * grows): "shot NAME" saves the game window as NAME.BMP, "click X Y" and
 * "rclick X Y" post a mouse click at client coordinates, "key VK" posts a
 * key press, "wait MS", "waitupd N" (until N updates have run). Progress
 * goes to CMDLOG.TXT, with the update count. */
static BOOL CALLBACK pick_window(HWND w, LPARAM lp)
{
    DWORD p;
    GetWindowThreadProcessId(w, &p);
    if (p != GetCurrentProcessId()) return TRUE;
    RECT rc;
    GetClientRect(w, &rc);
    HWND *best = (HWND *)lp;
    RECT b = { 0 };
    if (*best) GetClientRect(*best, &b);
    if (rc.right * rc.bottom > b.right * b.bottom) *best = w;
    return TRUE;
}

static BOOL CALLBACK pick_dialog(HWND w, LPARAM lp)
{
    DWORD p;
    char cls[16];
    GetWindowThreadProcessId(w, &p);
    if (p == GetCurrentProcessId() && IsWindowVisible(w) && GetClassNameA(w, cls, sizeof cls) && !strcmp(cls, "#32770")) {
        *(HWND *)lp = w;
        return FALSE;
    }
    return TRUE;
}

/* The game's open dialog if there is one, else its largest top-level window. */
static HWND game_window(void)
{
    HWND found = NULL;
    EnumWindows(pick_dialog, (LPARAM)&found);
    if (found) return found;
    EnumWindows(pick_window, (LPARAM)&found);
    return found;
}

static int sx0, sy0, sw0, sh0;   /* "shotr": a region of the client only */
static void shot(HWND w, const char *name)
{
    RECT rc;
    GetClientRect(w, &rc);
    int W = rc.right, H = rc.bottom, X = 0, Y = 0;
    if (sw0) X = sx0, Y = sy0, W = sw0, H = sh0;
    if (W <= 0 || H <= 0) return;
    HDC wdc = GetDC(w), mdc = CreateCompatibleDC(wdc);
    HBITMAP bm = CreateCompatibleBitmap(wdc, W, H);
    SelectObject(mdc, bm);
    BitBlt(mdc, 0, 0, W, H, wdc, X, Y, SRCCOPY);
    BITMAPINFOHEADER bi = { sizeof bi, W, H, 1, 24, BI_RGB };
    int stride = (W * 3 + 3) & ~3;
    char *px = malloc(stride * H);
    GetDIBits(mdc, bm, 0, H, px, (BITMAPINFO *)&bi, DIB_RGB_COLORS);
    char fn[MAX_PATH];
    snprintf(fn, sizeof fn, "%s.BMP", name);
    FILE *f = fopen(fn, "wb");
    if (f) {
        BITMAPFILEHEADER fh = { 0x4D42, sizeof fh + sizeof bi + stride * H, 0, 0, sizeof fh + sizeof bi };
        fwrite(&fh, sizeof fh, 1, f); fwrite(&bi, sizeof bi, 1, f); fwrite(px, stride * H, 1, f); fclose(f);
    }
    free(px); DeleteObject(bm); DeleteDC(mdc); ReleaseDC(w, wdc);
}

/* "wshot NAME": the whole window, frame and menu bar included. */
static void wshot(HWND w, const char *name)
{
    RECT rc;
    GetWindowRect(w, &rc);
    int W = rc.right - rc.left, H = rc.bottom - rc.top;
    if (W <= 0 || H <= 0) return;
    HDC wdc = GetWindowDC(w), mdc = CreateCompatibleDC(wdc);
    HBITMAP bm = CreateCompatibleBitmap(wdc, W, H);
    SelectObject(mdc, bm);
    BitBlt(mdc, 0, 0, W, H, wdc, 0, 0, SRCCOPY);
    BITMAPINFOHEADER bi = { sizeof bi, W, H, 1, 24, BI_RGB };
    int stride = (W * 3 + 3) & ~3;
    char *px = malloc(stride * H);
    GetDIBits(mdc, bm, 0, H, px, (BITMAPINFO *)&bi, DIB_RGB_COLORS);
    char fn[MAX_PATH];
    snprintf(fn, sizeof fn, "%s.BMP", name);
    FILE *f = fopen(fn, "wb");
    if (f) {
        BITMAPFILEHEADER fh = { 0x4D42, sizeof fh + sizeof bi + stride * H, 0, 0, sizeof fh + sizeof bi };
        fwrite(&fh, sizeof fh, 1, f); fwrite(&bi, sizeof bi, 1, f); fwrite(px, stride * H, 1, f); fclose(f);
    }
    free(px); DeleteObject(bm); DeleteDC(mdc); ReleaseDC(w, wdc);
}

/* "tree": every window of the process, children indented, with class, id,
 * styles, text, the window rect in its top-level window's client
 * coordinates and the client size; the frame's menu with item states. */
static FILE *tree_log;
static HWND tree_top;
static void tree_one(HWND w, int depth)
{
    char title[128] = "", cls[64] = "";
    GetWindowTextA(w, title, sizeof title);
    GetClassNameA(w, cls, sizeof cls);
    RECT r, c;
    GetWindowRect(w, &r);
    GetClientRect(w, &c);
    POINT a = { r.left, r.top }, b = { r.right, r.bottom };
    if (depth) { ScreenToClient(tree_top, &a); ScreenToClient(tree_top, &b); }
    fprintf(tree_log, "  %*s%p vis %d id %ld class '%s' style %#lx ex %#lx text '%s' rect %ld %ld %ld %ld client %ld %ld\n",
            depth * 2, "", (void *)w, IsWindowVisible(w), (long)GetWindowLongA(w, GWL_ID), cls,
            (long)GetWindowLongA(w, GWL_STYLE), (long)GetWindowLongA(w, GWL_EXSTYLE), title,
            a.x, a.y, b.x, b.y, c.right, c.bottom);
    for (HWND ch = GetWindow(w, GW_CHILD); ch; ch = GetWindow(ch, GW_HWNDNEXT)) tree_one(ch, depth + 1);
}
static void tree_menu(HMENU m, int depth)
{
    for (int i = 0, n = GetMenuItemCount(m); i < n; i++) {
        char t[128] = "";
        GetMenuStringA(m, i, t, sizeof t, MF_BYPOSITION);
        UINT st = GetMenuState(m, i, MF_BYPOSITION);
        fprintf(tree_log, "  %*smenu '%s' id %d state %#x\n", depth * 2, "", t, GetMenuItemID(m, i), st);
        HMENU sub = GetSubMenu(m, i);
        if (sub) tree_menu(sub, depth + 1);
    }
}
static BOOL CALLBACK tree_top_window(HWND w, LPARAM lp)
{
    (void)lp;
    DWORD p;
    GetWindowThreadProcessId(w, &p);
    if (p != GetCurrentProcessId()) return TRUE;
    tree_top = w;
    tree_one(w, 0);
    if (GetMenu(w)) tree_menu(GetMenu(w), 2);
    return TRUE;
}

/* The game polls the mouse (GetCursorPos, GetAsyncKeyState): it sees only
 * the harness's virtual pointer and buttons/keys, never the real ones. */
static POINT vcur;
static volatile unsigned char vkeys[256];
static BOOL WINAPI v_GetCursorPos(LPPOINT p) { *p = vcur; return TRUE; }
static SHORT WINAPI v_GetAsyncKeyState(int vk) { return vk >= 0 && vk < 256 && vkeys[vk] ? (SHORT)0x8001 : 0; }

static void hold_updates(int n)
{
    int until = update + n;
    for (int k = 0; k < 400 && update < until; k++) Sleep(5);   /* (menus: no updates, ~2s at most) */
}

/* "windows NAME": lists the process's top-level windows into CMDLOG and
 * saves each visible one as NAME_k.BMP. */
static FILE *wl_log;
static const char *wl_name;
static int wl_n;
static BOOL CALLBACK list_window(HWND w, LPARAM lp)
{
    (void)lp;
    DWORD p;
    GetWindowThreadProcessId(w, &p);
    if (p != GetCurrentProcessId()) return TRUE;
    char title[128] = "", cls[64] = "";
    GetWindowTextA(w, title, sizeof title);
    GetClassNameA(w, cls, sizeof cls);
    RECT r;
    GetWindowRect(w, &r);
    int k = wl_n++;
    if (wl_log) fprintf(wl_log, "  window %d %p vis %d class '%s' title '%s' rect %ld %ld %ld %ld\n", k, (void *)w,
                        IsWindowVisible(w), cls, title, r.left, r.top, r.right, r.bottom);
    if (IsWindowVisible(w)) {
        char nm[160];
        snprintf(nm, sizeof nm, "%s_%d", wl_name, k);
        shot(w, nm);
    }
    return TRUE;
}

/* Text without '&' (menu and button accelerators). */
static void plain(char *t)
{
    char *o = t;
    for (char *i = t; *i; i++) if (*i != '&') *o++ = *i;
    *o = 0;
}

/* "menu NAME": the frame's menu item whose text starts with NAME, as if chosen. */
static int find_menu(HMENU m, const char *name)
{
    for (int i = 0, n = GetMenuItemCount(m); i < n; i++) {
        char t[128] = "";
        GetMenuStringA(m, i, t, sizeof t, MF_BYPOSITION);
        plain(t);
        HMENU sub = GetSubMenu(m, i);
        if (sub) {
            int id = find_menu(sub, name);
            if (id) return id;
        } else if (!strncmp(t, name, strlen(name))) {
            return GetMenuItemID(m, i);
        }
    }
    return 0;
}

/* "ctl NAME": clicks the dialog control whose text starts with NAME;
 * "ctls": lists the dialog's controls (class, text, checked). */
static FILE *ctl_log;
static const char *ctl_name;
static HWND ctl_found;
static BOOL CALLBACK each_ctl(HWND c, LPARAM lp)
{
    (void)lp;
    char t[128] = "", cls[64] = "";
    GetWindowTextA(c, t, sizeof t);
    GetClassNameA(c, cls, sizeof cls);
    plain(t);
    if (ctl_name && !ctl_found && t[0] && !strncmp(t, ctl_name, strlen(ctl_name))) ctl_found = c;
    if (!ctl_name && ctl_log)
        fprintf(ctl_log, "  ctl %p class '%s' text '%s' checked %ld\n", (void *)c, cls, t, (long)SendMessageA(c, BM_GETCHECK, 0, 0));
    return TRUE;
}

static DWORD WINAPI harness(LPVOID p)
{
    (void)p;
    long done = 0;
    FILE *cl = fopen("CMDLOG.TXT", "w");
    for (;;) {
        Sleep(200);
        FILE *f = fopen("CMD.TXT", "r");
        if (!f) continue;
        char line[256];
        long n = 0;
        while (fgets(line, sizeof line, f)) {
            if (n++ < done) continue;
            done = n;
            HWND w = game_window();
            char a[64]; int x = 0, y = 0;
            if (sscanf(line, "shot %63s", a) == 1 && w) shot(w, a);
            else if (sscanf(line, "shotr %63s %d %d %d %d", a, &sx0, &sy0, &sw0, &sh0) == 5 && w) { shot(w, a); sw0 = 0; }
            else if (sscanf(line, "seq %63s %d %d %d %d %d %d", a, &sx0, &sy0, &sw0, &sh0, &x, &y) == 7 && w) {
                /* "seq NAME X Y W H N MS": N region shots MS apart, NAME_k */
                for (int k = 0; k < x; k++) {
                    char nm[96];
                    snprintf(nm, sizeof nm, "%s_%03d", a, k);
                    DWORD t0 = GetTickCount();
                    shot(w, nm);
                    if (cl) { fprintf(cl, "  seq %d at %lu\n", k, (unsigned long)t0); }
                    DWORD el = GetTickCount() - t0;
                    if ((int)el < y) Sleep(y - el);
                }
                sw0 = 0;
            }
            else if (sscanf(line, "wshot %63s", a) == 1 && w) wshot(w, a);
            else if (sscanf(line, "edit %63[^\n]", a) == 1 && w) {
                HWND ed = FindWindowExA(w, NULL, "Edit", NULL);
                if (ed) SetWindowTextA(ed, a);
                if (cl) fprintf(cl, "  edit %p '%s'\n", (void *)ed, a);
            }
            else if (sscanf(line, "size %d %d", &x, &y) == 2 && w) SetWindowPos(w, NULL, 0, 0, x, y, SWP_NOMOVE | SWP_NOZORDER);
            else if (sscanf(line, "cmd %d", &x) == 1) {
                HWND f = NULL;
                EnumWindows(pick_window, (LPARAM)&f);
                if (f) PostMessageA(f, WM_COMMAND, x, 0);
            }
            else if (sscanf(line, "keymenu %63s", a) == 1 && w) PostMessageA(w, WM_SYSCOMMAND, SC_KEYMENU, a[0]);
            else if (!strncmp(line, "max", 3) && w) ShowWindow(w, SW_MAXIMIZE);
            else if (!strncmp(line, "restore", 7) && w) ShowWindow(w, SW_RESTORE);
            else if (!strncmp(line, "tree", 4)) { tree_log = cl; EnumWindows(tree_top_window, 0); }
            else if (sscanf(line, "dbl %d %d", &x, &y) == 2 && w) {
                POINT pt = { x, y };
                HWND t = w;
                for (;;) {
                    HWND ch = ChildWindowFromPointEx(t, pt, CWP_SKIPINVISIBLE);
                    if (!ch || ch == t) break;
                    MapWindowPoints(t, ch, &pt, 1);
                    t = ch;
                }
                LPARAM lp = MAKELPARAM(pt.x, pt.y);
                PostMessageA(t, WM_LBUTTONDOWN, MK_LBUTTON, lp); PostMessageA(t, WM_LBUTTONUP, 0, lp);
                PostMessageA(t, WM_LBUTTONDBLCLK, MK_LBUTTON, lp); PostMessageA(t, WM_LBUTTONUP, 0, lp);
                if (cl) fprintf(cl, "  -> dbl window %p at %ld %ld\n", (void *)t, pt.x, pt.y);
            }
            else if ((sscanf(line, "click %d %d", &x, &y) == 2 || sscanf(line, "rclick %d %d", &x, &y) == 2) && w) {
                /* the virtual pointer there (screen), the button held for two
                 * updates, and the messages to the innermost child window */
                bool right = line[0] == 'r';
                POINT sp = { x, y };
                ClientToScreen(w, &sp);
                vcur = sp;
                POINT pt = { x, y };
                HWND t = w;
                for (;;) {
                    HWND ch = ChildWindowFromPointEx(t, pt, CWP_SKIPINVISIBLE);
                    if (!ch || ch == t) break;
                    MapWindowPoints(t, ch, &pt, 1);
                    t = ch;
                }
                LPARAM lp = MAKELPARAM(pt.x, pt.y);
                PostMessageA(t, WM_MOUSEMOVE, 0, lp);
                hold_updates(1);
                vkeys[right ? VK_RBUTTON : VK_LBUTTON] = 1;
                PostMessageA(t, right ? WM_RBUTTONDOWN : WM_LBUTTONDOWN, right ? MK_RBUTTON : MK_LBUTTON, lp);
                hold_updates(2);
                vkeys[right ? VK_RBUTTON : VK_LBUTTON] = 0;
                PostMessageA(t, right ? WM_RBUTTONUP : WM_LBUTTONUP, 0, lp);
                if (cl) fprintf(cl, "  -> window %p at %ld %ld (screen %ld %ld)\n", (void *)t, pt.x, pt.y, sp.x, sp.y);
            } else if (sscanf(line, "hold %d %d %d", &x, &y, &sw0) == 3 && w) {
                /* the left button held there sw0 ms */
                POINT sp = { x, y };
                ClientToScreen(w, &sp);
                vcur = sp;
                POINT pt = { x, y };
                HWND t = w;
                for (;;) {
                    HWND ch = ChildWindowFromPointEx(t, pt, CWP_SKIPINVISIBLE);
                    if (!ch || ch == t) break;
                    MapWindowPoints(t, ch, &pt, 1);
                    t = ch;
                }
                LPARAM lp = MAKELPARAM(pt.x, pt.y);
                if (sndlog) fprintf(sndlog, "%lu hold down\n", GetTickCount());
                vkeys[VK_LBUTTON] = 1;
                PostMessageA(t, WM_LBUTTONDOWN, MK_LBUTTON, lp);
                Sleep(sw0);
                vkeys[VK_LBUTTON] = 0;
                PostMessageA(t, WM_LBUTTONUP, 0, lp);
                if (sndlog) fprintf(sndlog, "%lu hold up\n", GetTickCount()), fflush(sndlog);
                sw0 = 0;
            } else if (sscanf(line, "move %d %d", &x, &y) == 2 && w) {
                POINT sp = { x, y };
                ClientToScreen(w, &sp);
                vcur = sp;
            } else if (sscanf(line, "key %d", &x) == 1 && w && x > 0 && x < 256) {
                vkeys[x] = 1;
                PostMessageA(w, WM_KEYDOWN, x, 1);
                hold_updates(2);
                vkeys[x] = 0;
                PostMessageA(w, WM_KEYUP, x, 0xC0000001);
            } else if (sscanf(line, "wait %d", &x) == 1) Sleep(x);
            else if (sscanf(line, "menu %63[^\n]", a) == 1) {
                HWND f = NULL;
                EnumWindows(pick_window, (LPARAM)&f);
                int id = f ? find_menu(GetMenu(f), a) : 0;
                if (id) PostMessageA(f, WM_COMMAND, id, 0);
                if (cl) fprintf(cl, "  menu '%s' id %d\n", a, id);
            } else if (sscanf(line, "ctl %63[^\n]", a) == 1 && strncmp(line, "ctls", 4) && w) {
                ctl_name = a; ctl_found = NULL;
                EnumChildWindows(w, each_ctl, 0);
                if (ctl_found) PostMessageA(ctl_found, BM_CLICK, 0, 0);
                if (cl) fprintf(cl, "  ctl '%s' %p\n", a, (void *)ctl_found);
                ctl_name = NULL;
            } else if (!strncmp(line, "ctls", 4) && w) {
                ctl_log = cl; ctl_name = NULL;
                EnumChildWindows(w, each_ctl, 0);
            }
            else if (sscanf(line, "windows %63s", a) == 1) { wl_log = cl; wl_name = a; wl_n = 0; EnumWindows(list_window, 0); }
            else if (sscanf(line, "waitupd %d", &x) == 1) while (update < x) Sleep(2);
            if (cl) { fprintf(cl, "%s window %p update %d: %s", w ? "ok" : "no", (void *)w, update, line); fflush(cl); }
        }
        fclose(f);
    }
    return 0;
}

/* Wine's display is true colour, which the game takes for "1 colour" and
 * refuses: its GetDeviceCaps calls are answered as on a 256-colour one. */
static FILE *capslog;
static int WINAPI caps_256(HDC dc, int idx)
{
    int v = GetDeviceCaps(dc, idx);
    switch (idx) {
    case BITSPIXEL: v = 8; break;
    case PLANES: v = 1; break;
    case NUMCOLORS: v = 256; break;
    case SIZEPALETTE: v = 256; break;
    case RASTERCAPS: v |= RC_PALETTE; break;
    }
    if (capslog) { fprintf(capslog, "GetDeviceCaps %d -> %d\n", idx, v); fflush(capslog); }
    return v;
}

static HFONT WINAPI log_CreateFontA(int h, int w, int esc, int ori, int wt, DWORD it, DWORD ul, DWORD so, DWORD cs,
                                    DWORD op, DWORD cp, DWORD q, DWORD pf, LPCSTR face)
{
    if (capslog) { fprintf(capslog, "CreateFontA h %d w %d weight %d italic %lu cs %lu q %lu pf %lu face '%s'\n", h, w, wt, it, cs, q, pf, face ? face : "(null)"); fflush(capslog); }
    return CreateFontA(h, w, esc, ori, wt, it, ul, so, cs, op, cp, q, pf, face);
}
static HGDIOBJ WINAPI log_GetStockObject(int i)
{
    if (capslog && i >= 10) { fprintf(capslog, "GetStockObject %d\n", i); fflush(capslog); }
    return GetStockObject(i);
}

static HPALETTE WINAPI log_CreatePalette(const LOGPALETTE *lp)
{
    if (capslog) {
        fprintf(capslog, "CreatePalette %d:", lp->palNumEntries);
        for (int i = 0; i < lp->palNumEntries; i++)
            fprintf(capslog, " %02x%02x%02x%s", lp->palPalEntry[i].peRed, lp->palPalEntry[i].peGreen, lp->palPalEntry[i].peBlue,
                    lp->palPalEntry[i].peFlags ? "*" : "");
        fprintf(capslog, "\n");
        fflush(capslog);
    }
    return CreatePalette(lp);
}

static void hook_import(HMODULE m, const char *dll, const char *fn, void *to)
{
    char *b = (char *)m;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(b + ((IMAGE_DOS_HEADER *)b)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(b + nt->OptionalHeader.DataDirectory[1].VirtualAddress);
    for (; d->Name; d++) {
        if (lstrcmpiA(b + d->Name, dll)) continue;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(b + d->OriginalFirstThunk), *iat = (IMAGE_THUNK_DATA *)(b + d->FirstThunk);
        for (; names->u1.AddressOfData; names++, iat++) {
            if (names->u1.Ordinal & IMAGE_ORDINAL_FLAG) continue;
            IMAGE_IMPORT_BY_NAME *ibn = (IMAGE_IMPORT_BY_NAME *)(b + names->u1.AddressOfData);
            if (strcmp((char *)ibn->Name, fn)) continue;
            DWORD old;
            VirtualProtect(&iat->u1.Function, 4, PAGE_READWRITE, &old);
            iat->u1.Function = (DWORD)to;
            VirtualProtect(&iat->u1.Function, 4, old, &old);
        }
    }
}

/* PROCESS.DLL's sound output: each buffer it hands Windows, appended to
 * WAVOUT.RAW, with its time and size in WAVOUT.TXT (and the format). */
#include <mmsystem.h>
static FILE *wav_raw, *wav_idx;
static MMRESULT WINAPI log_waveOutOpen(LPHWAVEOUT h, UINT dev, LPCWAVEFORMATEX f, DWORD_PTR cb, DWORD_PTR inst, DWORD fl)
{
    if (!wav_idx) wav_idx = fopen("WAVOUT.TXT", "w");
    if (wav_idx && f) fprintf(wav_idx, "format tag %d ch %d rate %lu bits %d\n", f->wFormatTag, f->nChannels, f->nSamplesPerSec, f->wBitsPerSample), fflush(wav_idx);
    return waveOutOpen(h, dev, f, cb, inst, fl);
}
static MMRESULT WINAPI log_waveOutWrite(HWAVEOUT h, LPWAVEHDR w, UINT n)
{
    if (!wav_raw) wav_raw = fopen("WAVOUT.RAW", "wb");
    if (!wav_idx) wav_idx = fopen("WAVOUT.TXT", "w");
    if (wav_raw && w) {
        fprintf(wav_idx, "%lu %lu %ld\n", GetTickCount(), w->dwBufferLength, ftell(wav_raw));
        fwrite(w->lpData, 1, w->dwBufferLength, wav_raw);
        fflush(wav_raw), fflush(wav_idx);
    }
    return waveOutWrite(h, w, n);
}

BOOL WINAPI DllMain(HINSTANCE h, DWORD why, LPVOID r)
{
    (void)h; (void)r;
    if (why == DLL_PROCESS_ATTACH) {
        if (getenv("WAVLOG") && GetModuleHandleA("PROCESS0.DLL")) {
            hook_import(GetModuleHandleA("PROCESS0.DLL"), "WINMM.dll", "waveOutOpen", log_waveOutOpen);
            hook_import(GetModuleHandleA("PROCESS0.DLL"), "WINMM.dll", "waveOutWrite", log_waveOutWrite);
        }
#ifdef PLAY
        /* (a copy to play: only the 256 colours Wine's display can't say it has) */
        hook_import(GetModuleHandleA(NULL), "GDI32.dll", "GetDeviceCaps", caps_256);
        return TRUE;
#endif
        capslog = fopen("CAPS.TXT", "w");
        hook_import(GetModuleHandleA(NULL), "GDI32.dll", "GetDeviceCaps", caps_256);
        hook_import(GetModuleHandleA(NULL), "USER32.dll", "GetCursorPos", v_GetCursorPos);
        hook_import(GetModuleHandleA(NULL), "GDI32.dll", "CreateFontA", log_CreateFontA);
        hook_import(GetModuleHandleA(NULL), "GDI32.dll", "CreatePalette", log_CreatePalette);
        hook_import(GetModuleHandleA(NULL), "GDI32.dll", "GetStockObject", log_GetStockObject);
        hook_import(GetModuleHandleA(NULL), "USER32.dll", "GetAsyncKeyState", v_GetAsyncKeyState);
        CreateThread(NULL, 0, harness, NULL, 0, NULL);
    }
    return TRUE;
}
