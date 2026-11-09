/*
 * The layer's core: the handle table, start-up (main), the kernel functions
 * the game uses, wsprintf, message boxes and the C runtime extras.
 */
#include "port_internal.h"
#include "port_res.h"

#include <ctype.h>
#include <sys/stat.h>

DWORD port_last_error;
static int g_trace = -1;

/* ---- Strings ---- */

size_t port_strlcpy(char *dst, const char *src, size_t size)
{
    size_t n = strlen(src);

    if (size) {
        size_t k = n < size - 1 ? n : size - 1;

        memcpy(dst, src, k);
        dst[k] = 0;
    }
    return n;
}

size_t port_strlcat(char *dst, const char *src, size_t size)
{
    size_t d = strnlen(dst, size);

    if (d == size)
        return size + strlen(src);
    return d + port_strlcpy(dst + d, src, size - d);
}

/* ---- Handles ---- */

typedef struct HandleSlot {
    int type;
    uint16_t gen;
    void *obj;
    int nextFree;
} HandleSlot;

static HandleSlot *g_slots;
static int g_nslots;
static int g_freeSlot = -1;

uintptr_t port_handle_new(int type, void *obj)
{
    int i;

    if (g_freeSlot >= 0) {
        i = g_freeSlot;
        g_freeSlot = g_slots[i].nextFree;
    } else {
        if (g_nslots == 0) {
            g_slots = calloc(1024, sizeof(HandleSlot));
            /* indexes below 64 stay unused: no handle is 0, and 1-31 are the
               system colour brushes, (HBRUSH)(COLOR_x + 1) */
            g_nslots = 64;
        } else if ((g_nslots & (g_nslots - 1)) == 0 && g_nslots >= 1024) {
            g_slots = realloc(g_slots, (size_t)g_nslots * 2 * sizeof(HandleSlot));
            memset(g_slots + g_nslots, 0, (size_t)g_nslots * sizeof(HandleSlot));
        }
        if (g_nslots >= 0xffff)
            port_fatal("out of handles");
        i = g_nslots++;
    }
    g_slots[i].type = type;
    g_slots[i].obj = obj;
    if (++g_slots[i].gen == 0)
        g_slots[i].gen = 1;
    return ((uintptr_t)g_slots[i].gen << 16) | (uintptr_t)i;
}

static HandleSlot *slot_of(const void *h)
{
    uintptr_t v = (uintptr_t)h;
    unsigned i = (unsigned)(v & 0xffff);
    unsigned gen = (unsigned)((v >> 16) & 0xffff);

    if (!v || (v >> 32) || i == 0 || (int)i >= g_nslots)
        return NULL;
    if (g_slots[i].type == H_FREE)
        return NULL;
    if (gen && gen != g_slots[i].gen)
        return NULL;
    return &g_slots[i];
}

void *port_handle_get(const void *h, int type)
{
    HandleSlot *s = slot_of(h);

    return s && s->type == type ? s->obj : NULL;
}

int port_handle_type(const void *h)
{
    HandleSlot *s = slot_of(h);

    return s ? s->type : H_FREE;
}

void port_handle_free(const void *h)
{
    HandleSlot *s = slot_of(h);

    if (s) {
        s->type = H_FREE;
        s->obj = NULL;
        s->nextFree = g_freeSlot;
        g_freeSlot = (int)(s - g_slots);
    }
}

/* ---- Tracing and errors ---- */

void port_trace(const char *fmt, ...)
{
    va_list ap;

    if (g_trace < 0)
        g_trace = getenv("LEMMINGS_TRACE") != NULL;
    if (!g_trace)
        return;
    va_start(ap, fmt);
    fprintf(stderr, "[port %6u] ", (unsigned)plat_ticks());
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void port_fatal(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    fprintf(stderr, "Lemmings: %s\n", buf);
    if (!port_harness)
        plat_message_box(NULL, buf, "Lemmings", MB_ICONHAND);
    exit(1);
}

/* Windows-1252 to UTF-8, for SDL's window titles and message boxes */
void port_utf8(const char *s, char *out, size_t size)
{
    static const unsigned short hi[32] = {
        0x20ac, 0x81, 0x201a, 0x192, 0x201e, 0x2026, 0x2020, 0x2021, 0x2c6, 0x2030, 0x160, 0x2039, 0x152, 0x8d,
        0x17d, 0x8f, 0x90, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x2dc, 0x2122, 0x161,
        0x203a, 0x153, 0x9d, 0x17e, 0x178,
    };
    size_t n = 0;

    for (; s && *s && n + 4 < size; s++) {
        unsigned c = (unsigned char)*s;

        if (c >= 0x80 && c < 0xa0)
            c = hi[c - 0x80];
        if (c < 0x80)
            out[n++] = (char)c;
        else if (c < 0x800) {
            out[n++] = (char)(0xc0 | (c >> 6));
            out[n++] = (char)(0x80 | (c & 0x3f));
        } else {
            out[n++] = (char)(0xe0 | (c >> 12));
            out[n++] = (char)(0x80 | ((c >> 6) & 0x3f));
            out[n++] = (char)(0x80 | (c & 0x3f));
        }
    }
    out[n] = 0;
}

/* ---- kernel32 ---- */

DWORD WINAPI GetVersion(void)
{
    return 0xC0000004;          /* Windows 95 4.0, what the game was made for */
}

DWORD WINAPI GetLastError(void)
{
    return port_last_error;
}

void WINAPI SetLastError(DWORD err)
{
    port_last_error = err;
}

static void shutdown_layer(void)
{
    static int done;

    if (done)
        return;
    done = 1;
    reg_save();
    mci_quit();
    wave_quit();
    plat_quit();
}

void WINAPI ExitProcess(UINT code)
{
    port_trace("ExitProcess(%u)", code);
    shutdown_layer();
    exit((int)code);
}

DWORD port_ticks(void)
{
    return (DWORD)plat_ticks();
}

DWORD WINAPI timeGetTime(void)
{
    return port_ticks();
}

DWORD WINAPI GetTickCount(void)
{
    return port_ticks();
}

void WINAPI Sleep(DWORD ms)
{
    plat_sleep(ms);
}

void WINAPI OutputDebugStringA(LPCSTR text)
{
    port_trace("debug: %s", text ? text : "");
}

void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS st)
{
    uint64_t total = plat_ram_bytes();

    if (total > 0x7fff0000)
        total = 0x7fff0000;     /* the game prints them as signed 32-bit numbers */
    st->dwLength = sizeof *st;
    st->dwMemoryLoad = 50;
    st->dwTotalPhys = (DWORD)total;
    st->dwAvailPhys = (DWORD)(total / 2);
    st->dwTotalPageFile = (DWORD)total;
    st->dwAvailPageFile = (DWORD)(total / 2);
    st->dwTotalVirtual = 0x7ffe0000;
    st->dwAvailVirtual = 0x7ff00000;
}

/* Global memory: a block with its size in front; the handle is the block */
typedef struct GBlock {
    SIZE_T size;
    int locks;
    long long align;
} GBlock;

HGLOBAL WINAPI GlobalAlloc(UINT flags, SIZE_T size)
{
    GBlock *b = (GBlock *)(flags & GMEM_ZEROINIT ? calloc(1, sizeof(GBlock) + size) : malloc(sizeof(GBlock) + size));

    if (!b)
        return NULL;
    b->size = size;
    b->locks = 0;
    return b;
}

LPVOID WINAPI GlobalLock(HGLOBAL mem)
{
    GBlock *b = (GBlock *)mem;

    if (!b)
        return NULL;
    b->locks++;
    return b + 1;
}

BOOL WINAPI GlobalUnlock(HGLOBAL mem)
{
    GBlock *b = (GBlock *)mem;

    if (b && b->locks > 0)
        b->locks--;
    return b && b->locks > 0;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL mem)
{
    free(mem);
    return NULL;
}

SIZE_T WINAPI GlobalSize(HGLOBAL mem)
{
    return mem ? ((GBlock *)mem)->size : 0;
}

int WINAPI MulDiv(int number, int numerator, int denominator)
{
    long long v;

    if (!denominator)
        return -1;
    v = (long long)number * numerator;
    /* rounded half away from zero, as Windows does */
    if ((v < 0) != (denominator < 0))
        v -= llabs((long long)denominator) / 2;
    else
        v += llabs((long long)denominator) / 2;
    return (int)(v / denominator);
}

LPSTR WINAPI lstrcpyA(LPSTR dst, LPCSTR src)
{
    return strcpy(dst, src);
}

LPSTR WINAPI lstrcatA(LPSTR dst, LPCSTR src)
{
    return strcat(dst, src);
}

int WINAPI lstrlenA(LPCSTR s)
{
    return s ? (int)strlen(s) : 0;
}

int WINAPI lstrcmpA(LPCSTR a, LPCSTR b)
{
    return strcmp(a, b);
}

int WINAPI lstrcmpiA(LPCSTR a, LPCSTR b)
{
    return strcasecmp(a, b);
}

static HINSTANCE g_hinst = (HINSTANCE)(uintptr_t)0x400000;

HMODULE WINAPI GetModuleHandleA(LPCSTR name)
{
    (void)name;
    return g_hinst;
}

DWORD WINAPI GetModuleFileNameA(HMODULE module, LPSTR name, DWORD size)
{
    (void)module;
    port_strlcpy(name, "C:\\WINLEMM\\LEMMINGS.EXE", size);
    return (DWORD)strlen(name);
}

/* ---- version.dll: the resource script's StringFileInfo ---- */

DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR file, LPDWORD handle)
{
    (void)file;
    if (handle)
        *handle = 0;
    return 16;
}

BOOL WINAPI GetFileVersionInfoA(LPCSTR file, DWORD handle, DWORD size, LPVOID data)
{
    (void)file;
    (void)handle;
    if (size >= 4)
        memcpy(data, "VSV1", 4);
    return TRUE;
}

BOOL WINAPI VerQueryValueA(LPCVOID block, LPCSTR sub, LPVOID *buf, PUINT len)
{
    const PortResVersion *v;
    char lang[16], key[64];

    (void)block;
    if (sscanf(sub, "\\StringFileInfo\\%15[^\\]\\%63s", lang, key) != 2)
        return FALSE;
    for (v = port_res_version; v->key; v++) {
        if (!strcasecmp(v->lang, lang) && !strcmp(v->key, key)) {
            *buf = (LPVOID)v->value;
            *len = (UINT)strlen(v->value) + 1;
            return TRUE;
        }
    }
    return FALSE;
}

/* ---- wsprintf: %d %i %u %x %X %c %s with flags and widths; 'l' and 'h'
   mean 32 and 16 bits, as in Windows (long is 64-bit on LP64) ---- */

int WINAPI wvsprintfA(LPSTR buf, LPCSTR fmt, va_list ap)
{
    char *o = buf, *end = buf + 1023;
    const char *f;

    for (f = fmt; *f && o < end; f++) {
        char spec[32], piece[600];
        int k = 0, isShort = 0;
        char conv;

        if (*f != '%') {
            *o++ = *f;
            continue;
        }
        spec[k++] = '%';
        f++;
        while (*f && strchr("-#0 +", *f) && k < 8)
            spec[k++] = *f++;
        while (isdigit((unsigned char)*f) && k < 16)
            spec[k++] = *f++;
        if (*f == '.') {
            spec[k++] = *f++;
            while (isdigit((unsigned char)*f) && k < 24)
                spec[k++] = *f++;
        }
        while (*f == 'l' || *f == 'h' || *f == 'L') {
            if (*f == 'h')
                isShort = 1;
            f++;
        }
        conv = *f;
        if (!conv)
            break;
        spec[k++] = conv == 'i' ? 'd' : conv;
        spec[k] = 0;
        switch (conv) {
        case 'd':
        case 'i':
            {
                int v = va_arg(ap, int);
                snprintf(piece, sizeof piece, spec, isShort ? (int)(short)v : v);
            }
            break;
        case 'u':
        case 'x':
        case 'X':
            {
                unsigned v = va_arg(ap, unsigned);
                snprintf(piece, sizeof piece, spec, isShort ? (unsigned)(unsigned short)v : v);
            }
            break;
        case 'c':
            snprintf(piece, sizeof piece, spec, va_arg(ap, int));
            break;
        case 's':
            {
                const char *s = va_arg(ap, const char *);
                snprintf(piece, sizeof piece, spec, s ? s : "(null)");
            }
            break;
        case 'p':
            snprintf(piece, sizeof piece, "%08X", (unsigned)(uintptr_t)va_arg(ap, void *));
            break;
        case '%':
            strcpy(piece, "%");
            break;
        default:
            piece[0] = conv;
            piece[1] = 0;
            break;
        }
        for (k = 0; piece[k] && o < end; k++)
            *o++ = piece[k];
    }
    *o = 0;
    return (int)(o - buf);
}

int WINAPIV wsprintfA(LPSTR buf, LPCSTR fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = wvsprintfA(buf, fmt, ap);
    va_end(ap);
    return n;
}

/* ---- Message boxes ---- */

void port_harness_log(const char *fmt, ...);

int WINAPI MessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type)
{
    char t[2048], c[256];
    Wnd *w = wnd_get(hwnd);
    int pressed;

    port_trace("MessageBox '%s': %s", caption ? caption : "", text ? text : "");
    if (port_harness) {
        port_harness_log("  messagebox '%s': %s", caption ? caption : "", text ? text : "");
        return (type & 0xf) == MB_YESNO || (type & 0xf) == MB_YESNOCANCEL ? IDYES : IDOK;
    }
    msg_present();
    port_utf8(text, t, sizeof t);
    port_utf8(caption ? caption : "Error", c, sizeof c);
    if (!w)
        w = port_active;
    w = w ? wnd_toplevel(w) : NULL;
    port_modal_depth++;
    pressed = plat_message_box(w ? w->plat : NULL, t, c, type);
    port_modal_depth--;
    return pressed;
}

/* ---- C runtime extras (port_prefix.h) ---- */

int port_rand(unsigned int *seed)
{
    *seed = *seed * 214013u + 2531011u;
    return (int)((*seed >> 16) & 0x7fff);
}

char *port_itoa(int value, char *buf, int radix)
{
    char tmp[34];
    char *p = buf;
    unsigned v = value < 0 && radix == 10 ? 0u - (unsigned)value : (unsigned)value;
    int i = 0;

    if (value < 0 && radix == 10)
        *p++ = '-';
    do {
        int d = (int)(v % (unsigned)radix);
        tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (unsigned)radix;
    } while (v);
    while (i)
        *p++ = tmp[--i];
    *p = 0;
    return buf;
}

/* ---- Start-up ---- */

static int is_data_dir(const char *dir)
{
    char p[1100];
    struct stat st;
    static const char *names[] = {"GFX", "gfx", "Gfx"};
    int i;

    for (i = 0; i < 3; i++) {
        snprintf(p, sizeof p, "%s/%s", dir, names[i]);
        if (!stat(p, &st) && S_ISDIR(st.st_mode))
            return 1;
    }
    return 0;
}

static char *find_data_dir(int argc, char **argv)
{
    const char *cands[8];
    char buf[1100], *res = NULL;
    const char *base = plat_base_dir();
    int n = 0, i;

    for (i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], "--data"))
            cands[n++] = argv[i + 1];
    if (getenv("LEMMINGS_DATA"))
        cands[n++] = getenv("LEMMINGS_DATA");
    for (i = 0; i < n; i++)
        if (is_data_dir(cands[i]))
            return strdup(cands[i]);
    if (n)
        port_fatal("The game data folder %s has no GFX folder.", cands[0]);
    /* next to the program (an app bundle's Resources), or orig/WINLEMM of a checkout */
    if (base) {
        static const char *rel[] = {"WINLEMM", "../Resources/WINLEMM", "orig/WINLEMM", "../orig/WINLEMM",
                                    "../../orig/WINLEMM", "."};
        for (i = 0; i < 6 && !res; i++) {
            snprintf(buf, sizeof buf, "%s%s", base, rel[i]);
            if (is_data_dir(buf))
                res = strdup(buf);
        }
    }
    if (!res && is_data_dir("orig/WINLEMM"))
        res = strdup("orig/WINLEMM");
    if (!res)
        port_fatal("Lemmings needs the original game's files.\n\nStart it with --data <folder> (the "
                   "WINLEMM folder of Lemmings for Windows 95), or set LEMMINGS_DATA.");
    return res;
}

int main(int argc, char **argv)
{
    char cmdline[1024] = "";
    char *data, *pref;
    int i, ret;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--data") && i + 1 < argc) {
            i++;
            continue;
        }
        if (strlen(cmdline) + strlen(argv[i]) + 2 < sizeof cmdline) {
            if (*cmdline)
                strcat(cmdline, " ");
            strcat(cmdline, argv[i]);
        }
    }
    plat_init();
    harness_init();
    data = find_data_dir(argc, argv);
    pref = getenv("LEMMINGS_PREFS") ? strdup(getenv("LEMMINGS_PREFS")) : plat_pref_dir();
    if (!pref)
        port_fatal("No folder for the settings");
    port_trace("data %s, settings %s", data, pref);
    file_init(data, pref);
    reg_init();
    gdi_init();
    font_init();
    msg_init();
    menu_init();
    controls_init();
    wave_init();
    ret = WinMain(g_hinst, NULL, cmdline, SW_SHOWDEFAULT);
    shutdown_layer();
    return ret;
}

/* ---- user32: rectangles ---- */

BOOL WINAPI SetRect(LPRECT r, int left, int top, int right, int bottom)
{
    *r = (RECT){left, top, right, bottom};
    return TRUE;
}

BOOL WINAPI SetRectEmpty(LPRECT r)
{
    memset(r, 0, sizeof *r);
    return TRUE;
}

BOOL WINAPI CopyRect(LPRECT dst, const RECT *src)
{
    if (!dst || !src)
        return FALSE;
    *dst = *src;
    return TRUE;
}

BOOL WINAPI IsRectEmpty(const RECT *r)
{
    return r->left >= r->right || r->top >= r->bottom;
}

BOOL WINAPI PtInRect(const RECT *r, POINT pt)
{
    return pt.x >= r->left && pt.x < r->right && pt.y >= r->top && pt.y < r->bottom;
}

BOOL WINAPI IntersectRect(LPRECT dst, const RECT *a, const RECT *b)
{
    return rect_intersect(dst, a, b);
}

BOOL WINAPI UnionRect(LPRECT dst, const RECT *a, const RECT *b)
{
    if (IsRectEmpty(a)) {
        *dst = *b;
        return !IsRectEmpty(b);
    }
    if (IsRectEmpty(b)) {
        *dst = *a;
        return TRUE;
    }
    dst->left = a->left < b->left ? a->left : b->left;
    dst->top = a->top < b->top ? a->top : b->top;
    dst->right = a->right > b->right ? a->right : b->right;
    dst->bottom = a->bottom > b->bottom ? a->bottom : b->bottom;
    return TRUE;
}

BOOL WINAPI OffsetRect(LPRECT r, int dx, int dy)
{
    r->left += dx;
    r->right += dx;
    r->top += dy;
    r->bottom += dy;
    return TRUE;
}

BOOL WINAPI InflateRect(LPRECT r, int dx, int dy)
{
    r->left -= dx;
    r->right += dx;
    r->top -= dy;
    r->bottom += dy;
    return TRUE;
}

BOOL WINAPI EqualRect(const RECT *a, const RECT *b)
{
    return !memcmp(a, b, sizeof *a);
}

/* ---- Help: Windows Help is gone; the original's English PDF manual instead ---- */

BOOL WINAPI WinHelpA(HWND hwnd, LPCSTR file, UINT cmd, ULONG_PTR data)
{
    char host[1100], url[1200];
    FILE *f;

    (void)hwnd;
    (void)file;
    (void)data;
    if (cmd == HELP_QUIT)
        return TRUE;
    if (!file_host_path("C:\\WINLEMM\\MANUALS\\ENGLISH.PDF", host, sizeof host, 0) || !(f = fopen(host, "rb")))
        return FALSE;
    fclose(f);
    snprintf(url, sizeof url, "file://%s", host);
    return plat_open_url(url);
}

/* ---- The C runtime's DLL hook (vscrt.h): nothing to set up natively ---- */

BOOL WINAPI _CRT_INIT(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    (void)hinst;
    (void)reason;
    (void)reserved;
    return TRUE;
}
