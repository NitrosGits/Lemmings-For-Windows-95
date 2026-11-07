/*
 * The options dialog: sound, music, effects, window size and zoom.
 */
#include <windows.h>
#include "winlemm.h"

int g_optionsApplied = 0;

static int s_volume;

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_applyoptions0, pad_applyoptions1, pad_applyoptions2, pad_applyoptions3, pad_applyoptions4, pad_applyoptions5, pad_applyoptions6, pad_applyoptions7, pad_applyoptions8, pad_applyoptions9, pad_applyoptions10, pad_applyoptions11, pad_applyoptions12;

/* 0x401770: take the dialog's settings; 0 to stay in the dialog */
int ApplyOptions(HWND hDlg, int volume)
{
    int zoom;
    int size;

    g_volume = volume;
    if (g_waveAvailable)
        g_soundFX = IsDlgButtonChecked(hDlg, 0x2001);
    if (g_musicAvailable)
        g_music = IsDlgButtonChecked(hDlg, 0x2002);
    g_visualFX = IsDlgButtonChecked(hDlg, 0x2003);
    g_pauseFocus = IsDlgButtonChecked(hDlg, 0x2004);
    zoom = g_zoom;
    if (IsDlgButtonChecked(hDlg, 1000))
        zoom = 1;
    if (IsDlgButtonChecked(hDlg, 1001))
        zoom = 2;
    if (IsDlgButtonChecked(hDlg, 1002))
        zoom = 4;
    size = 0;
    if (IsDlgButtonChecked(hDlg, 0x184))
        size = 1;
    if (IsDlgButtonChecked(hDlg, 0x185))
        size = 2;
    if (!WindowFits(size, zoom)) {
        if (MessageBox(0, GetString(0x7d01), "Warning", MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
            return 0;
    }
    SetSFXFlags(g_visualFX, g_soundFX);
    SetMusic(g_music);
    WaveSetVolume(g_volume);
    if (g_resolutionNew != size && InGame()) {
        if (MessageBox(0, GetString(0x7d00), "Warning", MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
            g_resolutionNew = size;
            SetZoom(g_hwnd, zoom == 8 ? 4 : zoom);
            PostMessage(g_hwnd, 0x603, 0, 0);
            return 1;
        }
    } else if (!InGame()) {
        g_resolutionNew = size;
        g_zoom = zoom == 8 ? 4 : zoom;
        return 1;
    } else if (InGame() && g_zoom != zoom)
        SetZoom(g_hwnd, zoom == 8 ? 4 : zoom);
#ifdef _MSC_VER
    switch (size) { case 1: break; }
#endif
    return 1;
}

/* 0x401990 */
void InitOptions(HWND hDlg)
{
    HWND item;

    g_waveAvailable = WaveIsEnabled();
    g_musicAvailable = 1;
    if (!g_waveAvailable)
        {
        item = GetDlgItem(hDlg, 0x2001);
        EnableWindow(item, FALSE);
    }
    else if (g_soundFX)
        CheckDlgButton(hDlg, 0x2001, 1);
    else
        CheckDlgButton(hDlg, 0x2001, 0);
    if (!g_musicAvailable)
        {
        item = GetDlgItem(hDlg, 0x2002);
        EnableWindow(item, FALSE);
    }
    else if (g_music)
        CheckDlgButton(hDlg, 0x2002, 1);
    else
        CheckDlgButton(hDlg, 0x2002, 0);
    if (g_visualFX)
        CheckDlgButton(hDlg, 0x2003, 1);
    else
        CheckDlgButton(hDlg, 0x2003, 0);
    if (g_pauseFocus)
        CheckDlgButton(hDlg, 0x2004, 1);
    else
        CheckDlgButton(hDlg, 0x2004, 0);
    if (g_memStatus.dwTotalPhys <= 0x400000)
        {
        item = GetDlgItem(hDlg, 0x185);
        EnableWindow(item, FALSE);
    }
    if (g_resolutionNew == 1)
        CheckDlgButton(hDlg, 0x184, 1);
    else
        CheckDlgButton(hDlg, 0x185, 1);
    switch (g_zoom) {
    case 1:
        CheckDlgButton(hDlg, 1000, 1);
        break;
    case 2:
        CheckDlgButton(hDlg, 1001, 1);
        break;
    case 4:
        CheckDlgButton(hDlg, 1002, 1);
        break;
    }
}

/* 0x401ae0 */
BOOL CALLBACK OptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if (!ApplyOptions(hDlg, s_volume))
                return 1;
            g_optionsApplied++;
        case IDCANCEL:
            EndDialog(hDlg, 1);
            return 1;
        case 0x183:
            WaveTestVolume(s_volume);
            return 1;
        }
        return 0;
    case WM_CTLCOLORDLG:
        GetSysColor(COLOR_BTNFACE);
    case WM_INITDIALOG:
        s_volume = g_volume;
        g_optionsApplied = 0;
        InitOptions(hDlg);
        return 1;
    }
    return 0;
}
