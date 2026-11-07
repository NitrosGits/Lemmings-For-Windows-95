/*
 * The few C runtime functions the game uses, in place of the Visual C++ 2.x
 * runtime the originals linked statically. Keeping our own (instead of
 * msvcrt.dll) leaves the rebuilt modules depending only on Windows itself, as
 * the originals did, so they still run on Windows 95.
 *
 * Built with -ffreestanding so the compiler doesn't turn these loops back into
 * calls to themselves.
 */
#include <stddef.h>

void *memset(void *dst, int c, size_t n)
{
    unsigned char *d = dst;

    while (n--)
        *d++ = (unsigned char)c;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    while (n--)
        *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else if (d > s) {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p = a, *q = b;

    for (; n; n--, p++, q++)
        if (*p != *q)
            return *p - *q;
    return 0;
}

size_t strlen(const char *s)
{
    const char *p = s;

    while (*p)
        p++;
    return (size_t)(p - s);
}

char *strcpy(char *dst, const char *src)
{
    char *d = dst;

    while ((*d++ = *src++) != 0)
        ;
    return dst;
}

/* Copies at most n characters and pads the rest of dst with zeros. */
char *strncpy(char *dst, const char *src, size_t n)
{
    char *d = dst;

    while (n && *src) {
        *d++ = *src++;
        n--;
    }
    while (n--)
        *d++ = 0;
    return dst;
}

char *strcat(char *dst, const char *src)
{
    strcpy(dst + strlen(dst), src);
    return dst;
}

int strcmp(const char *a, const char *b)
{
    const unsigned char *p = (const unsigned char *)a, *q = (const unsigned char *)b;

    while (*p && *p == *q) {
        p++;
        q++;
    }
    return *p - *q;
}

/* The Visual C++ runtime's generator, so the sequence is the original's. The
 * originals each had their own copy of the runtime, so each DLL has its own
 * seed. */
static unsigned long holdrand = 1;

void srand(unsigned seed)
{
    holdrand = seed;
}

int rand(void)
{
    holdrand = holdrand * 214013L + 2531011L;
    return (int)((holdrand >> 16) & 0x7fff);
}
