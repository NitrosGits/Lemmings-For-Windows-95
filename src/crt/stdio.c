/*
 * MinGW builds only: the little of stdio the game uses - PROCESS.DLL's
 * scripts, and the EXE copying its database and installer files ("rb" and
 * "wb", whole files) - through Windows. The streams are our own; the game
 * only passes them back to these functions. A stream's descriptor (_fileno)
 * is its index here.
 */
#include <windows.h>
#include <stdio.h>

typedef struct VSFILE {
    HANDLE handle;
    int eof;
    DWORD count, pos;
    unsigned char buf[512];
} VSFILE;

#define NFILES 8
static VSFILE g_files[NFILES];

FILE *fopen(const char *name, const char *mode)
{
    int i;

    for (i = 0; i < NFILES; i++) {
        if (!g_files[i].handle) {
            HANDLE h = *mode == 'w'
                ? CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)
                : CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

            if (h == INVALID_HANDLE_VALUE)
                return NULL;
            g_files[i].handle = h;
            g_files[i].eof = 0;
            g_files[i].count = g_files[i].pos = 0;
            return (FILE *)&g_files[i];
        }
    }
    return NULL;
}

int fgetc(FILE *stream)
{
    VSFILE *f = (VSFILE *)stream;

    if (f->pos == f->count) {
        f->pos = 0;
        if (!ReadFile(f->handle, f->buf, sizeof(f->buf), &f->count, NULL) || !f->count) {
            f->count = 0;
            f->eof = 1;
            return EOF;
        }
    }
    return f->buf[f->pos++];
}

size_t fread(void *buf, size_t size, size_t count, FILE *stream)
{
    VSFILE *f = (VSFILE *)stream;
    unsigned char *p = (unsigned char *)buf;
    DWORD want = size * count, got = 0, n;

    if (!size)
        return 0;
    while (got < want && f->pos < f->count)
        p[got++] = f->buf[f->pos++];
    if (got < want) {
        if (!ReadFile(f->handle, p + got, want - got, &n, NULL))
            n = 0;
        got += n;
        if (got < want)
            f->eof = 1;
    }
    return got / size;
}

size_t fwrite(const void *buf, size_t size, size_t count, FILE *stream)
{
    DWORD n;

    if (!size || !WriteFile(((VSFILE *)stream)->handle, buf, size * count, &n, NULL))
        return 0;
    return n / size;
}

int _fileno(FILE *stream)
{
    return (VSFILE *)stream - g_files;
}

long _filelength(int fd)
{
    DWORD size = GetFileSize(g_files[fd].handle, NULL);

    return size == INVALID_FILE_SIZE ? -1 : (long)size;
}

int _unlink(const char *name)
{
    return DeleteFileA(name) ? 0 : -1;
}

/* MinGW's headers declare these three dllimport */
void *_imp___fileno = (void *)_fileno;
void *_imp___filelength = (void *)_filelength;
void *_imp___unlink = (void *)_unlink;

char *itoa(int value, char *str, int radix)
{
    char tmp[34];
    char *p = str;
    unsigned v = value < 0 && radix == 10 ? -(unsigned)value : (unsigned)value;
    int i = 0;

    if (value < 0 && radix == 10)
        *p++ = '-';
    do {
        int d = v % radix;
        tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= radix;
    } while (v);
    while (i)
        *p++ = tmp[--i];
    *p = 0;
    return str;
}

int feof(FILE *stream)
{
    return ((VSFILE *)stream)->eof;
}

int fclose(FILE *stream)
{
    VSFILE *f = (VSFILE *)stream;

    CloseHandle(f->handle);
    f->handle = NULL;
    return 0;
}

int atoi(const char *s)
{
    int n = 0, neg = 0;

    while (*s == ' ' || *s == '\t')
        s++;
    if (*s == '-' || *s == '+')
        neg = *s++ == '-';
    while (*s >= '0' && *s <= '9')
        n = n * 10 + *s++ - '0';
    return neg ? -n : n;
}
