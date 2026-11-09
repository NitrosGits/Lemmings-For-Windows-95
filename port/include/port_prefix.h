/*
 * Included first into every source file of the game in the native build
 * (-include port_prefix.h). It brings in the host's C library and replaces the
 * pieces of the Visual C++ runtime the game uses that the host lacks or that
 * must see Windows paths:
 *   - fopen goes through the layer's path translation (backslashes, upper-case
 *     files on disk, drive letters);
 *   - _filelength, _fileno, _unlink and itoa;
 *   - rand/srand: Visual C++'s generator, one per module, as each DLL had its
 *     own copy of the runtime (the build defines PORT_MODULE_RAND to a name per
 *     module).
 */
#ifndef PORT_PREFIX_H
#define PORT_PREFIX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PORT_SDL
#define PORT_SDL 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

FILE *port_fopen(const char *name, const char *mode);
long port_filelength(int fd);
int port_fileno(FILE *f);
int port_unlink(const char *name);
char *port_itoa(int value, char *buf, int radix);
int port_rand(unsigned int *seed);

#ifdef __cplusplus
}
#endif

#define fopen port_fopen
#define _filelength port_filelength
#define _fileno port_fileno
#define _unlink port_unlink
#define itoa port_itoa

#ifdef PORT_MODULE_RAND
/* One seed per module, like the per-DLL C runtimes of the original. Each
   module's rand() users are in one file (render.c, midi.cpp), so a seed per
   file is a seed per module. */
static unsigned int PORT_MODULE_RAND __attribute__((unused)) = 1;
#define rand() port_rand(&PORT_MODULE_RAND)
#define srand(s) ((void)(PORT_MODULE_RAND = (unsigned int)(s)))
#endif

#endif
