/*
 * The registry: a tree of keys with REG_SZ / REG_DWORD / REG_BINARY values,
 * kept in memory and saved to registry.txt in the preferences folder in the
 * .reg file style:
 *   [HKEY_LOCAL_MACHINE\Software\Visual Sciences\Windows Lemmings\Users\Tester]
 *   "Width"=dword:00000280
 *   "DatabaseName"="C:\\Program Files\\WinLemm\\usr00000.db"
 * Names are matched without regard to case, as Windows does.
 */
#include "port_internal.h"

typedef struct RegValue {
    char *name;
    DWORD type;
    BYTE *data;
    DWORD size;
    struct RegValue *next;
} RegValue;

typedef struct RegKey {
    char *name;
    struct RegKey *parent, *child, *next;
    RegValue *values;
    int deleted;
} RegKey;

static RegKey g_roots[4];
static const char *g_rootNames[4] = {"HKEY_CLASSES_ROOT", "HKEY_CURRENT_USER", "HKEY_LOCAL_MACHINE", "HKEY_USERS"};
static int g_dirty;

/* open keys: HKEY values 1..n index this table (the predefined keys are 0x8000000x) */
static RegKey **g_open;
static int g_nopen;

static RegKey *key_of(HKEY h)
{
    uintptr_t v = (uintptr_t)h;

    if (v >= 0x80000000u && v <= 0x80000003u)
        return &g_roots[v - 0x80000000u];
    if (v >= 1 && (int)v <= g_nopen && g_open[v - 1] && !g_open[v - 1]->deleted)
        return g_open[v - 1];
    return NULL;
}

static HKEY open_handle(RegKey *k)
{
    int i;

    for (i = 0; i < g_nopen; i++)
        if (!g_open[i])
            break;
    if (i == g_nopen) {
        g_open = realloc(g_open, (size_t)(g_nopen + 16) * sizeof *g_open);
        memset(g_open + g_nopen, 0, 16 * sizeof *g_open);
        g_nopen += 16;
    }
    g_open[i] = k;
    return (HKEY)(uintptr_t)(i + 1);
}

static RegKey *find_child(RegKey *k, const char *name, size_t n, int create)
{
    RegKey *c, **last = &k->child;

    for (c = k->child; c; last = &c->next, c = c->next)
        if (!c->deleted && strlen(c->name) == n && !SDL_strncasecmp(c->name, name, n))
            return c;
    if (!create)
        return NULL;
    c = calloc(1, sizeof *c);
    c->name = malloc(n + 1);
    memcpy(c->name, name, n);
    c->name[n] = 0;
    c->parent = k;
    *last = c;
    g_dirty = 1;
    return c;
}

/* a\b\c below k; NULL if a part is missing and create is 0 */
static RegKey *find_path(RegKey *k, const char *path, int create, int *created)
{
    const char *p = path;

    if (created)
        *created = 0;
    while (k && p && *p) {
        const char *e = strchr(p, '\\');
        size_t n = e ? (size_t)(e - p) : strlen(p);

        if (n) {
            RegKey *c = find_child(k, p, n, 0);

            if (!c && create) {
                c = find_child(k, p, n, 1);
                if (created)
                    *created = 1;
            }
            k = c;
        }
        p = e ? e + 1 : NULL;
    }
    return k;
}

static RegValue *find_value(RegKey *k, const char *name, int create)
{
    RegValue *v, **last = &k->values;

    if (!name)
        name = "";
    for (v = k->values; v; last = &v->next, v = v->next)
        if (!SDL_strcasecmp(v->name, name))
            return v;
    if (!create)
        return NULL;
    v = calloc(1, sizeof *v);
    v->name = SDL_strdup(name);
    *last = v;
    return v;
}

static void set_value(RegKey *k, const char *name, DWORD type, const BYTE *data, DWORD size)
{
    RegValue *v = find_value(k, name, 1);

    free(v->data);
    v->data = malloc(size ? size : 1);
    if (size)
        memcpy(v->data, data, size);
    v->size = size;
    v->type = type;
    g_dirty = 1;
}

/* ---- the file ---- */

static void path_of(RegKey *k, char *out, size_t size)
{
    if (!k->parent) {
        SDL_strlcpy(out, g_rootNames[k - g_roots], size);
        return;
    }
    path_of(k->parent, out, size);
    SDL_strlcat(out, "\\", size);
    SDL_strlcat(out, k->name, size);
}

static void put_string(FILE *f, const char *s, size_t n)
{
    size_t i;

    fputc('"', f);
    for (i = 0; i < n && s[i]; i++) {
        if (s[i] == '\\' || s[i] == '"')
            fputc('\\', f);
        fputc(s[i], f);
    }
    fputc('"', f);
}

static void save_key(FILE *f, RegKey *k)
{
    RegKey *c;
    RegValue *v;
    char path[2048];
    DWORD i;

    if (k->deleted)
        return;
    if (k->parent || k->values) {
        path_of(k, path, sizeof path);
        fprintf(f, "\n[%s]\n", path);
        for (v = k->values; v; v = v->next) {
            put_string(f, v->name, strlen(v->name));
            fputc('=', f);
            if (v->type == REG_DWORD && v->size == 4) {
                DWORD d;

                memcpy(&d, v->data, 4);
                fprintf(f, "dword:%08x\n", d);
            } else if (v->type == REG_SZ) {
                put_string(f, (const char *)v->data, v->size);
                fputc('\n', f);
            } else {
                fprintf(f, "hex(%x):", v->type);
                for (i = 0; i < v->size; i++)
                    fprintf(f, "%s%02x", i ? "," : "", v->data[i]);
                fputc('\n', f);
            }
        }
    }
    for (c = k->child; c; c = c->next)
        save_key(f, c);
}

static void reg_file(char *out, size_t size)
{
    snprintf(out, size, "%s/registry.txt", file_pref_dir());
}

void reg_save(void)
{
    char name[1100], tmp[1110];
    FILE *f;
    int i;

    if (!g_dirty)
        return;
    reg_file(name, sizeof name);
    snprintf(tmp, sizeof tmp, "%s.new", name);
    f = fopen(tmp, "w");
    if (!f)
        return;
    fprintf(f, "; Lemmings for Windows 95 (native build): the game's registry\n");
    for (i = 0; i < 4; i++)
        save_key(f, &g_roots[i]);
    if (fclose(f) == 0 && rename(tmp, name) == 0)
        g_dirty = 0;
}

static char *get_string(char *p, char *out, size_t size)
{
    size_t n = 0;

    if (*p != '"')
        return NULL;
    for (p++; *p && *p != '"'; p++) {
        if (*p == '\\' && p[1])
            p++;
        if (n + 1 < size)
            out[n++] = *p;
    }
    out[n] = 0;
    return *p == '"' ? p + 1 : NULL;
}

static void load(void)
{
    char name[1100], line[4096];
    RegKey *k = NULL;
    FILE *f;

    reg_file(name, sizeof name);
    f = fopen(name, "r");
    if (!f)
        return;
    while (fgets(line, sizeof line, f)) {
        char *p = line, vname[512], sval[2048];

        line[strcspn(line, "\r\n")] = 0;
        if (*p == '[') {
            char *e = strrchr(p, ']');
            char *rest;
            int i;

            k = NULL;
            if (!e)
                continue;
            *e = 0;
            p++;
            rest = strchr(p, '\\');
            if (rest)
                *rest++ = 0;
            for (i = 0; i < 4; i++)
                if (!strcmp(p, g_rootNames[i]))
                    k = rest ? find_path(&g_roots[i], rest, 1, NULL) : &g_roots[i];
            continue;
        }
        if (!k || *p != '"' || !(p = get_string(p, vname, sizeof vname)) || *p++ != '=')
            continue;
        if (!strncmp(p, "dword:", 6)) {
            DWORD d = (DWORD)strtoul(p + 6, NULL, 16);
            set_value(k, vname, REG_DWORD, (BYTE *)&d, 4);
        } else if (*p == '"') {
            if (get_string(p, sval, sizeof sval))
                set_value(k, vname, REG_SZ, (BYTE *)sval, (DWORD)strlen(sval) + 1);
        } else if (!strncmp(p, "hex(", 4)) {
            BYTE data[1024];
            DWORD type = (DWORD)strtoul(p + 4, &p, 16), n = 0;

            if (*p == ')')
                p++;
            if (*p == ':')
                p++;
            while (*p && n < sizeof data) {
                data[n++] = (BYTE)strtoul(p, &p, 16);
                if (*p == ',')
                    p++;
            }
            set_value(k, vname, type, data, n);
        }
    }
    fclose(f);
}

void reg_init(void)
{
    RegKey *k;
    int i;

    for (i = 0; i < 4; i++)
        g_roots[i].name = (char *)g_rootNames[i];
    load();
    g_dirty = 0;
    /* where the game puts the players' databases (GetUserDir) */
    k = find_path(&g_roots[2], "Software\\Microsoft\\Windows\\CurrentVersion", 1, NULL);
    if (!find_value(k, "ProgramFilesDir", 0))
        set_value(k, "ProgramFilesDir", REG_SZ, (const BYTE *)"C:\\Program Files", 17);
}

/* ---- advapi32 ---- */

LONG WINAPI RegOpenKeyExA(HKEY key, LPCSTR sub, DWORD options, REGSAM sam, PHKEY result)
{
    RegKey *k = key_of(key);

    (void)options;
    (void)sam;
    *result = NULL;
    if (!k)
        return ERROR_INVALID_HANDLE;
    k = find_path(k, sub, 0, NULL);
    if (!k)
        return ERROR_FILE_NOT_FOUND;
    *result = open_handle(k);
    return ERROR_SUCCESS;
}

LONG WINAPI RegCreateKeyExA(HKEY key, LPCSTR sub, DWORD reserved, LPSTR cls, DWORD options, REGSAM sam,
                            LPSECURITY_ATTRIBUTES sa, PHKEY result, LPDWORD disposition)
{
    RegKey *k = key_of(key);
    int created;

    (void)reserved;
    (void)cls;
    (void)options;
    (void)sam;
    (void)sa;
    *result = NULL;
    if (!k)
        return ERROR_INVALID_HANDLE;
    k = find_path(k, sub, 1, &created);
    *result = open_handle(k);
    if (disposition)
        *disposition = created ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
    return ERROR_SUCCESS;
}

LONG WINAPI RegCloseKey(HKEY key)
{
    uintptr_t v = (uintptr_t)key;

    if (v >= 1 && (int)v <= g_nopen && g_open[v - 1]) {
        g_open[v - 1] = NULL;
        return ERROR_SUCCESS;
    }
    return v >= 0x80000000u && v <= 0x80000003u ? ERROR_SUCCESS : ERROR_INVALID_HANDLE;
}

LONG WINAPI RegFlushKey(HKEY key)
{
    if (!key_of(key))
        return ERROR_INVALID_HANDLE;
    reg_save();
    return ERROR_SUCCESS;
}

LONG WINAPI RegDeleteKeyA(HKEY key, LPCSTR sub)
{
    RegKey *k = key_of(key);

    if (!k)
        return ERROR_INVALID_HANDLE;
    k = find_path(k, sub, 0, NULL);
    if (!k || !k->parent)
        return ERROR_FILE_NOT_FOUND;
    k->deleted = 1;             /* with its subkeys, as Windows 95 does */
    g_dirty = 1;
    return ERROR_SUCCESS;
}

LONG WINAPI RegEnumKeyExA(HKEY key, DWORD index, LPSTR name, LPDWORD len, LPDWORD reserved, LPSTR cls,
                          LPDWORD clslen, PFILETIME time)
{
    RegKey *k = key_of(key), *c;
    DWORD i = 0;

    (void)reserved;
    if (!k)
        return ERROR_INVALID_HANDLE;
    for (c = k->child; c; c = c->next) {
        if (c->deleted)
            continue;
        if (i++ == index) {
            DWORD n = (DWORD)strlen(c->name);

            if (n + 1 > *len)
                return ERROR_MORE_DATA;
            memcpy(name, c->name, n + 1);
            *len = n;
            if (cls && clslen && *clslen)
                *cls = 0;
            if (clslen)
                *clslen = 0;
            if (time)
                memset(time, 0, sizeof *time);
            return ERROR_SUCCESS;
        }
    }
    return ERROR_NO_MORE_ITEMS;
}

LONG WINAPI RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size)
{
    RegKey *k = key_of(key);
    RegValue *v;

    (void)reserved;
    if (!k)
        return ERROR_INVALID_HANDLE;
    v = find_value(k, name, 0);
    if (!v)
        return ERROR_FILE_NOT_FOUND;
    if (type)
        *type = v->type;
    if (data) {
        if (!size || *size < v->size) {
            if (size)
                *size = v->size;
            return ERROR_MORE_DATA;
        }
        memcpy(data, v->data, v->size);
    }
    if (size)
        *size = v->size;
    return ERROR_SUCCESS;
}

LONG WINAPI RegSetValueExA(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD size)
{
    RegKey *k = key_of(key);

    (void)reserved;
    if (!k)
        return ERROR_INVALID_HANDLE;
    set_value(k, name, type, data, size);
    return ERROR_SUCCESS;
}
