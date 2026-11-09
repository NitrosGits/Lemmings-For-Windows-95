/*
 * Pointers that the original keeps in or passes as 32-bit integers (window
 * longs, MCI and WinHelp parameters, a handle in an int). In the native 64-bit
 * build (PORT_SDL, PORTING.md) they get pointer-sized types; every other build
 * gets exactly the original's types, so Visual C++ 2.2 and MinGW compile the
 * same code as before. Only macros: no symbols for the compiler.
 */
#ifndef PORT64_H
#define PORT64_H

#ifdef PORT_SDL
#define PORT_GetWindowLong GetWindowLongPtr
#define PORT_SetWindowLong SetWindowLongPtr
#define PORT_LONG LONG_PTR      /* a pointer in a window long */
#define PORT_DWORD DWORD_PTR    /* a pointer passed as a DWORD */
#define PORT_INT INT_PTR        /* a pointer or handle passed as an int */
#else
#define PORT_GetWindowLong GetWindowLong
#define PORT_SetWindowLong SetWindowLong
#define PORT_LONG LONG
#define PORT_DWORD DWORD
#define PORT_INT int
#endif

#endif
