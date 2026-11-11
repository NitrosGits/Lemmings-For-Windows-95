/* What win64.c and harness.c share (PORTING.md, section 10). */
#ifndef WIN64_H
#define WIN64_H

#include <windows.h>

extern int port_harness;                /* a scripted run (LEMMINGS_HARNESS=1) */

void harness_start(void);
int harness_cursor(POINT *pt);          /* the harness's pointer, in screen coordinates */
int harness_key(int vk);                /* is its key or mouse button held? */
void harness_log(const char *fmt, ...);

#endif
