/*
 * The about box: the version resource's strings and a page of system
 * information (Windows version, memory, MIDI device).
 */
#include <windows.h>
#include <string.h>
#include "winlemm.h"

#ifdef _MSC_VER
#pragma warning(disable: 4173)   /* GetSystemInfoText returns a local buffer */
#endif

static HFONT s_hFont;

/* 0x401bb0: centre a window over another, keeping it on the screen */
void CentreWindow(HWND hwnd, HWND hwndParent)
{
    RECT rect;
    RECT parent;
    int width, height;
    int parentWidth, parentHeight;
    int screenWidth, screenHeight;
    int x, y;
    HDC hdc;

    GetWindowRect(hwnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
    GetWindowRect(hwndParent, &parent);
    parentWidth = parent.right - parent.left;
    parentHeight = parent.bottom - parent.top;
    hdc = GetDC(hwnd);
    screenWidth = GetDeviceCaps(hdc, HORZRES);
    screenHeight = GetDeviceCaps(hdc, VERTRES);
    ReleaseDC(hwnd, hdc);
    x = parent.left + (parentWidth - width) / 2;
    if (x < 0)
        x = 0;
    else if (x + width > screenWidth)
        x = screenWidth - width;
    y = parent.top + (parentHeight - height) / 2;
    if (y < 0)
        y = 0;
    else if (y + height > screenHeight)
        y = screenHeight - height;
    SetWindowPos(hwnd, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/* 0x401ca0: returns a local buffer, as the original does */
char *GetSystemInfoText(void)
{
    MIDIOUTCAPS caps;
    char type[256];
    char os[256];
    char midi[256];
#ifdef _MSC_VER
    char text[1024];            /* returned: the caller uses it at once (sic) */
#else
    static char text[1024];     /* not on a stack frame that is gone by the time it is read */
#endif
    DWORD version = GetVersion();
    WORD ver = LOWORD(version);

    if (version < 0x80000000)
        wsprintf(os, "WindowsNT %u.%u (Build: %u)", LOBYTE(ver), HIBYTE(ver), HIWORD(version));
    else if (LOBYTE(ver) < 4)
        wsprintf(os, "Win32s %u.%u (Build: %u)", LOBYTE(ver), HIBYTE(ver), HIWORD(version) & ~0x8000);
    else
        wsprintf(os, "Windows95 %u.%u", LOBYTE(ver), HIBYTE(ver));
    if (g_midiDevice != -1) {
        midiOutGetDevCaps(g_midiDevice, &caps, sizeof(caps));
        type[0] = 0;
        if (caps.wTechnology & MOD_MIDIPORT)
            strcat(type, "MIDI Hardware Port");
        else if (caps.wTechnology & MOD_SYNTH)
            strcat(type, "Generic Internal Synthesiser");
        else if ((caps.wTechnology & MOD_SQSYNTH) == MOD_SQSYNTH)
            strcat(type, "Square Wave Synthesiser");
        else if (caps.wTechnology & MOD_FMSYNTH)
            strcat(type, "FM Synthesiser");
        else if ((caps.wTechnology & MOD_MAPPER) == MOD_MAPPER)
            strcat(type, "Microsoft Midi Mapper");
        wsprintf(midi, "MIDI driver version : %d.%d\nMIDI product name : %s\nMIDI Device Type : %s\n",
                 HIBYTE(caps.vDriverVersion), LOBYTE(caps.vDriverVersion), caps.szPname, type);
    } else
        wsprintf(midi, "No MIDI device activated\n");
#ifdef PORT_SDL                 /* the native build: the system it really runs on (PORTING.md) */
    {
        char memory[256];

        if (port_native_system(os, sizeof(os), memory, sizeof(memory))) {
            wsprintf(text, "Operating System : %s\nLemmings Build Number : %d\n%s", os, 31, memory);
            strcat(text, midi);
            return text;
        }
    }
#endif
    wsprintf(text, "Operating System : %s\nLemmings Build Number : %d\nPhysical Memory Size : %ld bytes\n"
             "Percent Used : %d%%\nAvail Memory Free : %ld bytes\n",
             os, 31, g_memStatus.dwTotalPhys, g_memStatus.dwMemoryLoad, g_memStatus.dwAvailPhys);
    strcat(text, midi);
    return text;
}

/* 0x401f10 */
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char *value;
    UINT len;
    DWORD handle;
    char buf[256];
    char path[256];
    char key[1024];
    DWORD size;
    void *data;
    int id;
    char *end;

    switch (msg) {
    case WM_INITDIALOG:
        s_hFont = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, VARIABLE_PITCH | FF_SWISS, "");
        CentreWindow(hDlg, GetWindow(hDlg, GW_OWNER));
        SetDlgItemText(hDlg, 0x11b, GetSystemInfoText());
        SendMessage(GetDlgItem(hDlg, 0x11b), WM_SETFONT, (WPARAM)s_hFont, 1);
        GetModuleFileName(g_hInst, path, sizeof(path));
        size = GetFileVersionInfoSize(path, &handle);
        if (size) {
            data = VSMEM_Allocate(size, "AboutBox");
            GetFileVersionInfo(path, handle, size, data);
            lstrcpy(key, "\\StringFileInfo\\080904e4\\");
            WORD length = (WORD)lstrlen(key);
            id = 0x400;
            end = key + length;
            for (; id <= 0x404; id++) {
                GetDlgItemText(hDlg, id, buf, sizeof(buf));
                *end = 0;
                lstrcat(key, buf);
                len = 0;
                value = 0;
                if (VerQueryValue(data, key, (void **)&value, &len) && len && value) {
                    lstrcpy(buf, value);
                    SetDlgItemText(hDlg, id, buf);
                    SendMessage(GetDlgItem(hDlg, id), WM_SETFONT, (WPARAM)s_hFont, 1);
                }
            }
            VSMEM_Free(data);
        }
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, 1);
            DeleteObject(s_hFont);
            return 1;
        }
        break;
    }
    return 0;
}
