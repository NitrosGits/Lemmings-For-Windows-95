/* GAMEOVER.DLL: the end-of-game text. */
#ifndef GAMEOVER_H
#define GAMEOVER_H

#include <windows.h>

#if defined(GAMEOVER_BUILD) && defined(_MSC_VER)
#define GOAPI __declspec(dllexport)
#else
#define GOAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

GOAPI BOOL WINAPI GO_Strings(char **strings, int *count);


#ifdef __cplusplus
}
#endif

#endif
