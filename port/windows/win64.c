/*
 * The 64-bit Windows build (PORTING.md, section 10): the game on the real
 * Win32, one program with the seven DLLs linked in. This file is all it needs
 * besides its own sources: the program's entry (finding the game's data),
 * and the few calls that win64_prefix.h sends here because a 64-bit program
 * on today's Windows cannot make them as the game did in 1995.
 */
#include <windows.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "win64.h"

/* ---- The entry point ---- */

/* the game's WinMain (src/lemmings/winmain.cpp), renamed by the build */
int WINAPI game_WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdLine, int show);

static int has_data(void)
{
    return GetFileAttributesA("GFX\\ICONS.BMP") != INVALID_FILE_ATTRIBUTES;
}

/* A message for the user: a box, or stderr in a scripted run (nobody is
   there to press OK). */
static void complain(const char *text)
{
    if (port_harness || getenv("LEMMINGS_QUIET"))
        fprintf(stderr, "lemmings: %s\n", text);
    else
        MessageBoxA(NULL, text, "Lemmings", MB_ICONEXCLAMATION);
}

/*
 * The game works in the current directory, which has to be the folder of
 * Lemmings for Windows 95 (GFX, LEVEL, SOUND, ...): --data FOLDER, the
 * LEMMINGS_DATA variable, where it was started, or where the program is.
 */
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdLine, int show)
{
    const char *data = getenv("LEMMINGS_DATA");
    const char *h = getenv("LEMMINGS_HARNESS");
    char dir[MAX_PATH], *p;
    int i;

    port_harness = h && *h && *h != '0';
    for (i = 1; i + 1 < __argc; i++)
        if (!strcmp(__argv[i], "--data"))
            data = __argv[i + 1];
    if (data && *data && !SetCurrentDirectoryA(data)) {
        char text[MAX_PATH + 64];

        snprintf(text, sizeof text, "The data folder \"%s\" cannot be opened.", data);
        complain(text);
        return 1;
    }
    if (!has_data() && GetModuleFileNameA(NULL, dir, sizeof dir) && (p = strrchr(dir, '\\')) != NULL) {
        *p = 0;
        SetCurrentDirectoryA(dir);
    }
    if (!has_data()) {
        complain("The game's data was not found.\n\nPut this program into the folder of Lemmings for Windows 95 "
                 "(the one with GFX, LEVEL and SOUND), or start it with --data FOLDER.");
        return 1;
    }
    if (port_harness)
        harness_start();
    return game_WinMain(inst, prev, cmdLine, show);
}

/* ---- The registry ---- */

/*
 * The game keeps everything under HKEY_LOCAL_MACHINE\Software\Visual
 * Sciences, opened for writing. A 64-bit program run by an ordinary user may
 * not write there, and Windows does not quietly redirect it as it does for
 * old 32-bit programs: the game's key is the same path under
 * HKEY_CURRENT_USER. Other machine keys (the game reads ProgramFilesDir) are
 * opened to read only.
 */
static int is_game_key(LPCSTR subkey)
{
    static const char game[] = "Software\\Visual Sciences";

    return subkey && !_strnicmp(subkey, game, sizeof game - 1);
}

LONG WINAPI port_RegOpenKeyExA(HKEY parent, LPCSTR subkey, DWORD options, REGSAM access, PHKEY key)
{
    if (parent == HKEY_LOCAL_MACHINE) {
        if (is_game_key(subkey))
            parent = HKEY_CURRENT_USER;
        else
            access = KEY_READ;
    }
    return RegOpenKeyExA(parent, subkey, options, access, key);
}

LONG WINAPI port_RegCreateKeyExA(HKEY parent, LPCSTR subkey, DWORD reserved, LPSTR cls, DWORD options, REGSAM access,
                                 LPSECURITY_ATTRIBUTES sa, PHKEY key, LPDWORD disposition)
{
    if (parent == HKEY_LOCAL_MACHINE) {
        if (!is_game_key(subkey))
            return ERROR_ACCESS_DENIED;
        parent = HKEY_CURRENT_USER;
    }
    return RegCreateKeyExA(parent, subkey, reserved, cls, options, access, sa, key, disposition);
}

/*
 * The players' level databases go into "ProgramFilesDir\WinLemm" (GetUserDir
 * in user.cpp), which only an administrator may write today. The game is
 * told a folder of the user's instead: "Visual Sciences" in the roaming
 * application data, or LEMMINGS_USERDIR. NULL if there is none.
 */
static const char *user_root(void)
{
    static char dir[MAX_PATH];
    const char *env = getenv("LEMMINGS_USERDIR");

    if (dir[0])
        return dir;
    if (env && *env && strlen(env) < 200)
        strcpy(dir, env);
    else if (SHGetFolderPathA(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, dir) == S_OK &&
             strlen(dir) < 200)
        strcat(dir, "\\Visual Sciences");
    else {
        dir[0] = 0;
        return NULL;
    }
    CreateDirectoryA(dir, NULL);
    return dir;
}

LONG WINAPI port_RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size)
{
    const char *root;

    if (name && !lstrcmpiA(name, "ProgramFilesDir") && (root = user_root()) != NULL) {
        DWORD need = (DWORD)strlen(root) + 1;

        if (type)
            *type = REG_SZ;
        if (!size)
            return data ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
        if (data && *size < need) {
            *size = need;
            return ERROR_MORE_DATA;
        }
        if (data)
            memcpy(data, root, need);
        *size = need;
        return ERROR_SUCCESS;
    }
    return RegQueryValueExA(key, name, reserved, type, data, size);
}

/* ---- The display ---- */

/*
 * The game warns at every start when the display is not in 256 colours, and
 * no display is any more. It draws through 8-bit DIB sections, which Windows
 * shows on a true-colour display as they are, and it copes when the system
 * has no palette to give (Bitmap::MakePalette), so it is simply told what it
 * was written for.
 */
int WINAPI port_GetDeviceCaps(HDC hdc, int index)
{
    int v = GetDeviceCaps(hdc, index);

    switch (index) {
    case BITSPIXEL:
        return 8;
    case PLANES:
        return 1;
    case NUMCOLORS:
    case SIZEPALETTE:
        return 256;
    case RASTERCAPS:
        return v | RC_PALETTE;
    }
    return v;
}

/* ---- Help ---- */

/* Windows has had no viewer for .HLP files since Vista: the manual that
   came with the game opens instead. */
BOOL WINAPI port_WinHelpA(HWND hwnd, LPCSTR file, UINT command, ULONG_PTR data)
{
    static const char manual[] = "MANUALS\\ENGLISH.PDF";

    (void)file;
    (void)data;
    if (command == HELP_QUIT)
        return TRUE;
    if (port_harness) {
        harness_log("  help: %s", manual);
        return TRUE;
    }
    return (INT_PTR)ShellExecuteA(hwnd, "open", manual, NULL, NULL, SW_SHOWNORMAL) > 32;
}

/* ---- Dialogs ---- */

/*
 * The game's dialog procedures return BOOL, 32 bits, as in the 1995 SDK;
 * Win64's return INT_PTR. Calling one as the other leaves the upper half of
 * the result to chance, so every dialog runs through this procedure, which
 * calls the game's and widens its answer: a handle (the dialog's brush) by
 * its sign, as Windows defines for handles, anything else as it is.
 */
typedef BOOL (CALLBACK *GAMEDLGPROC)(HWND, UINT, WPARAM, LPARAM);

static const char g_dlgProp[] = "LemmingsDlgProc";
static GAMEDLGPROC g_newDlgProc;        /* of the dialog being created */

static INT_PTR CALLBACK DlgThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GAMEDLGPROC proc = (GAMEDLGPROC)GetPropA(hwnd, g_dlgProp);
    BOOL r;

    if (!proc) {
        if (!g_newDlgProc)
            return FALSE;
        proc = g_newDlgProc;
        g_newDlgProc = NULL;
        SetPropA(hwnd, g_dlgProp, (HANDLE)proc);
    }
    r = proc(hwnd, msg, wParam, lParam);
    if (msg == WM_NCDESTROY)
        RemovePropA(hwnd, g_dlgProp);
    return (INT_PTR)r;                  /* int to INT_PTR: sign-extended */
}

INT_PTR WINAPI port_DialogBoxParamA(HINSTANCE inst, LPCSTR name, HWND parent, DLGPROC proc, LPARAM param)
{
    g_newDlgProc = (GAMEDLGPROC)(void (*)(void))proc;   /* the game casts its procedure to DLGPROC: back again */
    return DialogBoxParamA(inst, name, parent, DlgThunk, param);
}

/* ---- The pointer and keys the game polls ---- */

BOOL WINAPI port_GetCursorPos(LPPOINT pt)
{
    if (port_harness && harness_cursor(pt))
        return TRUE;
    return GetCursorPos(pt);
}

SHORT WINAPI port_GetAsyncKeyState(int vk)
{
    if (port_harness)
        return harness_key(vk) ? (SHORT)0x8001 : 0;
    return GetAsyncKeyState(vk);
}

/* ---- The C runtime ---- */

/* Visual C++'s rand(), with the caller's seed (win64_prefix.h) */
int port_rand(unsigned int *seed)
{
    *seed = *seed * 214013u + 2531011u;
    return (int)((*seed >> 16) & 0x7fff);
}

/* The DLLs' DllEntryPoint functions call it; they are never run (they were
   not the DLLs' entry points in the original either, see vsmem.c). */
BOOL WINAPI _CRT_INIT(HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    (void)inst;
    (void)reason;
    (void)reserved;
    return TRUE;
}

/* ---- What the game tells a port with native views ---- */

/* The game draws its own skill panel and status bar here. */
int port_native_icon(HWND hwnd, const BYTE *sprite, int frame, const RGBQUAD *colours, int count, int pressed)
{
    (void)hwnd;
    (void)sprite;
    (void)frame;
    (void)colours;
    (void)count;
    (void)pressed;
    return 0;
}

int port_native_status(HWND hwnd, const char *const *texts, int n)
{
    (void)hwnd;
    (void)texts;
    (void)n;
    return 0;
}

/*
 * The about box: the system it really runs on. GetVersion tells a program
 * without a manifest "Windows 8", and the game prints memory sizes with
 * "%ld", 32 bits: the true version (ntdll's RtlGetVersion) and the memory in
 * GB, in the lines of the game's own text.
 */
int port_native_system(char *os, int osSize, char *memory, int memorySize)
{
    typedef LONG (WINAPI *RTLGETVERSION)(OSVERSIONINFOW *);
    const double gb = 1024.0 * 1024.0 * 1024.0;
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    RTLGETVERSION getVersion = ntdll ? (RTLGETVERSION)(void *)GetProcAddress(ntdll, "RtlGetVersion") : NULL;
    OSVERSIONINFOW v;
    MEMORYSTATUSEX m;
    char size[32];
#if defined(__aarch64__) || defined(_M_ARM64)
    static const char arch[] = "ARM64";
#else
    static const char arch[] = "x64";
#endif

    memset(&v, 0, sizeof v);
    v.dwOSVersionInfoSize = sizeof v;
    m.dwLength = sizeof m;
    if (!getVersion || getVersion(&v) != 0 || !GlobalMemoryStatusEx(&m) || !m.ullTotalPhys)
        return 0;
    if (v.dwMajorVersion == 10 && v.dwMinorVersion == 0)
        snprintf(os, (size_t)osSize, "Windows %d (build %lu, %s)", v.dwBuildNumber >= 22000 ? 11 : 10,
                 (unsigned long)v.dwBuildNumber, arch);
    else
        snprintf(os, (size_t)osSize, "Windows %lu.%lu (build %lu, %s)", (unsigned long)v.dwMajorVersion,
                 (unsigned long)v.dwMinorVersion, (unsigned long)v.dwBuildNumber, arch);
    if (m.ullTotalPhys % (1u << 30))
        snprintf(size, sizeof size, "%.1f GB", m.ullTotalPhys / gb);
    else
        snprintf(size, sizeof size, "%llu GB", (unsigned long long)(m.ullTotalPhys >> 30));
    snprintf(memory, (size_t)memorySize, "Physical Memory Size : %s\nPercent Used : %d%%\n"
             "Avail Memory Free : %.1f GB\n", size, (int)m.dwMemoryLoad, m.ullAvailPhys / gb);
    return 1;
}
