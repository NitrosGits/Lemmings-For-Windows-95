/*
 * Files. The game uses Windows paths: backslashes, any case (the original
 * data files are upper case on disk), relative to the game's folder or on
 * drive C:. They map like this (PORTING.md, section 3):
 *   C:\WINLEMM\... and relative paths  ->  the original data folder (read);
 *                                          writes go to <prefs>/drive_c/WINLEMM,
 *                                          which reads also look at first
 *   any other C:\...                   ->  <prefs>/drive_c/...
 * so the original data is never written.
 */
#include "port_internal.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static char g_data[1024];       /* the original data, no trailing slash */
static char g_drive[1024];      /* <prefs>/drive_c */
static char g_pref[1024];

#define GAME_DIR "C:\\WINLEMM"

const char *file_data_dir(void)
{
    return g_data;
}

const char *file_pref_dir(void)
{
    return g_pref;
}

static void make_dirs(const char *path)
{
    char p[1100];
    char *s;

    port_strlcpy(p, path, sizeof p);
    for (s = p + 1; *s; s++) {
        if (*s == '/') {
            *s = 0;
            mkdir(p, 0755);
            *s = '/';
        }
    }
    mkdir(p, 0755);
}

void file_init(const char *dataDir, const char *prefDir)
{
    size_t n;

    port_strlcpy(g_data, dataDir, sizeof g_data);
    n = strlen(g_data);
    while (n > 1 && g_data[n - 1] == '/')
        g_data[--n] = 0;
    port_strlcpy(g_pref, prefDir, sizeof g_pref);
    n = strlen(g_pref);
    while (n > 1 && g_pref[n - 1] == '/')
        g_pref[--n] = 0;
    snprintf(g_drive, sizeof g_drive, "%s/drive_c", g_pref);
    make_dirs(g_drive);
}

static int exists(const char *p)
{
    struct stat st;

    return stat(p, &st) == 0;
}

/* Appends rel (components separated by '/') to base, matching each existing
   component case-insensitively; missing ones are kept as written. */
static void resolve(char *out, size_t size, const char *base, const char *rel)
{
    char comp[256];
    const char *p = rel;

    port_strlcpy(out, base, size);
    while (*p) {
        size_t n = 0, len;

        while (*p == '/')
            p++;
        while (*p && *p != '/' && n + 1 < sizeof comp)
            comp[n++] = *p++;
        comp[n] = 0;
        if (!n)
            break;
        if (!strcmp(comp, "."))
            continue;
        len = strlen(out);
        snprintf(out + len, size - len, "/%s", comp);
        if (!exists(out)) {
            DIR *d;
            struct dirent *e;

            out[len] = 0;
            d = opendir(*out ? out : "/");
            snprintf(out + len, size - len, "/%s", comp);
            if (d) {
                while ((e = readdir(d)) != NULL) {
                    if (!strcasecmp(e->d_name, comp)) {
                        snprintf(out + len, size - len, "/%s", e->d_name);
                        break;
                    }
                }
                closedir(d);
            }
        }
    }
}

/* The host path for a Windows path; 0 if it can't be mapped */
int file_host_path(const char *win, char *out, size_t size, int forWrite)
{
    char rel[1024];
    size_t i, n;
    int game;

    if (!win || !*win)
        return 0;
    /* backslashes, then where it is */
    for (i = n = 0; win[i] && n + 1 < sizeof rel; i++)
        rel[n++] = win[i] == '\\' ? '/' : win[i];
    rel[n] = 0;
    if (rel[0] && rel[1] == ':') {
        const char *p = rel + 2;

        if ((rel[0] | 0x20) != 'c')
            return 0;
        while (*p == '/')
            p++;
        if (!strncasecmp(p, "WINLEMM", 7) && (p[7] == 0 || p[7] == '/')) {
            memmove(rel, p + 7, strlen(p + 7) + 1);
            game = 1;
        } else {
            memmove(rel, p, strlen(p) + 1);
            game = 0;
        }
    } else if (rel[0] == '/') {
        port_strlcpy(out, win, size);    /* a host path (the layer's own) */
        return 1;
    } else
        game = 1;
    if (!game) {
        resolve(out, size, g_drive, rel);
        return 1;
    }
    {
        char overlay[1100];
        char base[1100];

        snprintf(base, sizeof base, "%s/WINLEMM", g_drive);
        resolve(overlay, sizeof overlay, base, rel);
        if (forWrite || exists(overlay)) {
            port_strlcpy(out, overlay, size);
            return 1;
        }
    }
    resolve(out, size, g_data, rel);
    return 1;
}

static void parent_dirs(const char *host)
{
    char p[1100];
    char *s;

    port_strlcpy(p, host, sizeof p);
    s = strrchr(p, '/');
    if (s && s != p) {
        *s = 0;
        make_dirs(p);
    }
}

/* ---- C runtime (port_prefix.h) ---- */

FILE *port_fopen(const char *name, const char *mode)
{
    char host[1100], m[8];
    int w = strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+');
    size_t i, n = 0;
    FILE *f;

    for (i = 0; mode[i] && n + 1 < sizeof m; i++)
        if (mode[i] != 't')
            m[n++] = mode[i];
    m[n] = 0;
    if (!file_host_path(name, host, sizeof host, w))
        return NULL;
    if (w)
        parent_dirs(host);
    f = fopen(host, m);
    port_trace("fopen(%s, %s) -> %s %s", name, mode, host, f ? "ok" : "failed");
    return f;
}

int port_fileno(FILE *f)
{
    return fileno(f);
}

long port_filelength(int fd)
{
    struct stat st;

    return fstat(fd, &st) ? -1 : (long)st.st_size;
}

int port_unlink(const char *name)
{
    char host[1100];

    if (!file_host_path(name, host, sizeof host, 1))
        return -1;
    return unlink(host);
}

/* ---- kernel32 ---- */

DWORD WINAPI GetCurrentDirectoryA(DWORD size, LPSTR buf)
{
    port_strlcpy(buf, GAME_DIR, size);
    return (DWORD)strlen(GAME_DIR);
}

UINT WINAPI GetWindowsDirectoryA(LPSTR buf, UINT size)
{
    char host[1100];

    port_strlcpy(buf, "C:\\WINDOWS", size);
    if (file_host_path(buf, host, sizeof host, 1))
        make_dirs(host);
    return (UINT)strlen(buf);
}

BOOL WINAPI CreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sa)
{
    char host[1100];

    (void)sa;
    if (!file_host_path(path, host, sizeof host, 1)) {
        port_last_error = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }
    if (exists(host)) {
        port_last_error = ERROR_ALREADY_EXISTS;
        return FALSE;
    }
    parent_dirs(host);
    if (mkdir(host, 0755)) {
        port_last_error = ERROR_PATH_NOT_FOUND;
        return FALSE;
    }
    return TRUE;
}

BOOL WINAPI DeleteFileA(LPCSTR path)
{
    char host[1100];

    if (!file_host_path(path, host, sizeof host, 1) || unlink(host)) {
        port_last_error = ERROR_FILE_NOT_FOUND;
        return FALSE;
    }
    return TRUE;
}

typedef struct HostFile {
    int fd;
} HostFile;

HANDLE WINAPI CreateFileA(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD creation,
                          DWORD flags, HANDLE templ)
{
    char host[1100];
    int w = (access & GENERIC_WRITE) != 0;
    int oflags = w ? (access & GENERIC_READ ? O_RDWR : O_WRONLY) : O_RDONLY;
    HostFile *f;
    int fd;

    (void)share;
    (void)sa;
    (void)flags;
    (void)templ;
    switch (creation) {
    case CREATE_NEW:
        oflags |= O_CREAT | O_EXCL;
        break;
    case CREATE_ALWAYS:
        oflags |= O_CREAT | O_TRUNC;
        break;
    case OPEN_ALWAYS:
        oflags |= O_CREAT;
        break;
    case TRUNCATE_EXISTING:
        oflags |= O_TRUNC;
        break;
    }
    if (!file_host_path(name, host, sizeof host, w)) {
        port_last_error = ERROR_PATH_NOT_FOUND;
        return INVALID_HANDLE_VALUE;
    }
    if (w)
        parent_dirs(host);
    fd = open(host, oflags, 0644);
    port_trace("CreateFile(%s) -> %s %s", name, host, fd >= 0 ? "ok" : "failed");
    if (fd < 0) {
        port_last_error = ERROR_FILE_NOT_FOUND;
        return INVALID_HANDLE_VALUE;
    }
    f = malloc(sizeof *f);
    f->fd = fd;
    return f;
}

BOOL WINAPI ReadFile(HANDLE file, LPVOID buf, DWORD size, LPDWORD got, LPVOID overlapped)
{
    HostFile *f = (HostFile *)file;
    ssize_t n;

    (void)overlapped;
    if (!f || file == INVALID_HANDLE_VALUE)
        return FALSE;
    n = read(f->fd, buf, size);
    if (got)
        *got = n > 0 ? (DWORD)n : 0;
    return n >= 0;
}

BOOL WINAPI WriteFile(HANDLE file, LPCVOID buf, DWORD size, LPDWORD done, LPVOID overlapped)
{
    HostFile *f = (HostFile *)file;
    ssize_t n;

    (void)overlapped;
    if (!f || file == INVALID_HANDLE_VALUE)
        return FALSE;
    n = write(f->fd, buf, size);
    if (done)
        *done = n > 0 ? (DWORD)n : 0;
    return n >= 0;
}

DWORD WINAPI GetFileSize(HANDLE file, LPDWORD high)
{
    HostFile *f = (HostFile *)file;
    struct stat st;

    if (high)
        *high = 0;
    if (!f || file == INVALID_HANDLE_VALUE || fstat(f->fd, &st))
        return INVALID_FILE_SIZE;
    return (DWORD)st.st_size;
}

DWORD WINAPI SetFilePointer(HANDLE file, LONG dist, LONG *high, DWORD method)
{
    HostFile *f = (HostFile *)file;
    off_t r;

    if (high)
        *high = 0;
    if (!f || file == INVALID_HANDLE_VALUE)
        return 0xffffffff;
    r = lseek(f->fd, dist, method == FILE_END ? SEEK_END : method == FILE_CURRENT ? SEEK_CUR : SEEK_SET);
    return r < 0 ? 0xffffffff : (DWORD)r;
}

BOOL WINAPI CloseHandle(HANDLE h)
{
    HostFile *f = (HostFile *)h;

    if (!f || h == INVALID_HANDLE_VALUE)
        return FALSE;
    close(f->fd);
    free(f);
    return TRUE;
}

/* The 16-bit file functions: an HFILE is the host's descriptor */

HFILE WINAPI _lopen(LPCSTR path, int mode)
{
    char host[1100];
    int w = (mode & 3) != OF_READ;
    int fd;

    if (!file_host_path(path, host, sizeof host, w))
        return HFILE_ERROR;
    fd = open(host, (mode & 3) == OF_WRITE ? O_WRONLY : (mode & 3) == OF_READWRITE ? O_RDWR : O_RDONLY);
    /* (the game writes only files it created: no copying of original files to the overlay) */
    port_trace("_lopen(%s, %d) -> %s %s", path, mode, host, fd >= 0 ? "ok" : "failed");
    return fd < 0 ? HFILE_ERROR : fd;
}

HFILE WINAPI _lcreat(LPCSTR path, int attr)
{
    char host[1100];

    (void)attr;
    if (!file_host_path(path, host, sizeof host, 1))
        return HFILE_ERROR;
    parent_dirs(host);
    return open(host, O_RDWR | O_CREAT | O_TRUNC, 0644);
}

UINT WINAPI _lread(HFILE file, LPVOID buf, UINT size)
{
    ssize_t n = read(file, buf, size);

    return n < 0 ? (UINT)HFILE_ERROR : (UINT)n;
}

UINT WINAPI _lwrite(HFILE file, LPCSTR buf, UINT size)
{
    ssize_t n = write(file, buf, size);

    return n < 0 ? (UINT)HFILE_ERROR : (UINT)n;
}

LONG WINAPI _llseek(HFILE file, LONG offset, int origin)
{
    off_t r = lseek(file, offset, origin == 2 ? SEEK_END : origin == 1 ? SEEK_CUR : SEEK_SET);

    return r < 0 ? -1 : (LONG)r;
}

HFILE WINAPI _lclose(HFILE file)
{
    return close(file) ? HFILE_ERROR : 0;
}
