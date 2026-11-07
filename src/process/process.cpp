/* The Visual Sciences libraries' assertion handler (not called here). */
#include <windows.h>

/* 0x10005c70 */
void VS_AssertFailed(const char *expr, const char *file, unsigned line)
{
    char msg[256];

    wsprintf(msg, "%s\nin file \"%s\" at line %u\n", expr, file, line);
    MessageBox(NULL, msg, "Visual Sciences Assertion Failure", MB_ICONEXCLAMATION);
    ExitProcess(0xaaaa);
}
