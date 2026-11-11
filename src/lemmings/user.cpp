/*
 * Settings and players: each player has a registry key and a copy of the
 * level database (who has done which level) in the user directory.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include "winlemm.h"

int g_unused08c = 0;
char *g_selectedUser = 0;
int g_newUser = 0;
HWND g_hUserList = 0;
HWND g_hUserEdit = 0;
HWND g_hDeleteUser = 0;
char **g_userNames = 0;
int g_numUsers = -1;

char g_userEdit[128];
char g_userDir[256];
#ifdef PORT_SDL                 /* the native build: room for a real path, not just the */
char g_nameDb[MAX_PATH];        /* 36 characters of "C:\Program Files\WinLemm\usr00000.db" (PORTING.md) */
#else
char g_nameDb[32];
#endif

/* 0x410dc0 */
void LoadSettings(void)
{
    OpenRegistry();
    ReadIniSettings(g_szIniFile);
}

/* 0x410de0 */
void SaveSettings(void)
{
    WriteIniSettings(g_szIniFile);
    CloseRegistry();
}

/* 0x410e00: a new player starts with a copy of the empty database */
void CopyDatabase(char *dest)
{
    FILE *out;
    FILE *in;
    long size;
    char *buf;

    out = fopen(dest, "wb");
    if (!out) {
        MessageBox(0, dest, "Unable to open file for writing", 0);
        return;
    }
    in = fopen("users\\lemmings.db", "rb");
    if (!in) {
        MessageBox(0, "users\\lemmings.db", "Unable to open file for writing", 0);
        return;
    }
    size = _filelength((_fileno)(in));
    buf = new char[size];
    if (!buf) {
        MessageBox(0, "Unable to allocate buffer for database copy", "Memory Error", 0);
        fclose(out);
        fclose(in);
        return;
    }
    fread(buf, size, 1, in);
    fwrite(buf, size, 1, out);
    delete buf;
    fclose(out);
    fclose(in);
}

/* 0x410ef0: Program Files\WinLemm, or the Windows directory */
char *GetUserDir(void)
{
    int failed = 0;
    HKEY key;
    char *dir;

    key = OpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion", "Visual Sciences data");
    if (key && (dir = ReadRegString(key, "ProgramFilesDir", 0)) != 0) {
        strcpy(g_userDir, dir);
        strcat(g_userDir, "\\WinLemm");
        if (!CreateDirectory(g_userDir, 0) && GetLastError() != ERROR_ALREADY_EXISTS)
            failed = 1;
    } else
        failed = 1;
    if (failed) {
        if (!GetWindowsDirectory(g_userDir, sizeof(g_userDir))) {
            MessageBox(0, "Unable to get windows directory entry", "FATAL ERROR", 0);
            ExitProcess(0xaaaa);
        }
    }
    return g_userDir;
}

/* 0x410fd0 */
char *CreateUserDatabase(char *user)
{
    char *dir = GetUserDir();
    int n;
    FILE *f;

    for (n = 0;; n++) {
        wsprintf(g_nameDb, "%s\\usr%05d.db", dir, n);
        f = fopen(g_nameDb, "rb");
        if (!f)
            break;
        fclose(f);
    }
    CopyDatabase(g_nameDb);
    WriteRegString(g_hkUser, "DatabaseName", g_nameDb);
    return g_nameDb;
}

/* 0x411050 */
char *GetUserDatabase(void)
{
    char *name = ReadRegString(g_hkUser, "DatabaseName", "Ben Dog");
    FILE *f = fopen(name, "rb");

    if (!f)
        name = CreateUserDatabase(g_userName);
    else
        fclose(f);
    return name;
}

/* 0x4110a0 */
void NewUser(char *name)
{
    WriteIniSettings(g_szIniFile);
    OpenUser(name);
    WriteIniSettings(g_szIniFile);
    CreateUserDatabase(name);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_selectuserdlgproc0, pad_selectuserdlgproc1, pad_selectuserdlgproc2, pad_selectuserdlgproc3, pad_selectuserdlgproc4, pad_selectuserdlgproc5, pad_selectuserdlgproc6, pad_selectuserdlgproc7, pad_selectuserdlgproc8, pad_selectuserdlgproc9, pad_selectuserdlgproc10, pad_selectuserdlgproc11, pad_selectuserdlgproc12, pad_selectuserdlgproc13, pad_selectuserdlgproc14, pad_selectuserdlgproc15, pad_selectuserdlgproc16;

/* 0x4110e0 */
BOOL CALLBACK SelectUserDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char text[256];
    int sel;
    int n;
    int i;

    switch (msg) {
    case WM_INITDIALOG:
        g_hUserList = GetDlgItem(hDlg, 0x3e9);
        g_hUserEdit = GetDlgItem(hDlg, 0x3ea);
        g_hDeleteUser = GetDlgItem(hDlg, 0x3eb);
        g_numUsers = EnumUsers(&g_userNames);
        for (i = 0; i < g_numUsers; i++)
            SendMessage(g_hUserList, LB_ADDSTRING, 0, (LPARAM)g_userNames[i]);
        SetWindowText(g_hUserEdit, g_userName);
        SendMessage(g_hUserEdit, EM_SETSEL, 0, 0xffff0000);
        g_selectedUser = g_userName;
        g_newUser = 0;
        EnableWindow(g_hDeleteUser, FALSE);
        return 1;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            GetWindowText(g_hUserEdit, g_userEdit, sizeof(g_userEdit));
            g_newUser = 1;
            g_selectedUser = g_userEdit;
            n = SendMessage(g_hUserList, LB_GETCOUNT, 0, 0);
            for (i = 0; i < n; i++) {
                SendMessage(g_hUserList, LB_GETTEXT, i, (LPARAM)text);
                if (!strcmp(g_userEdit, text))
                    g_newUser = 0;
            }
            EndDialog(hDlg, msg);
            return 1;
        case IDCANCEL:
            if (IsWindowEnabled(GetDlgItem(hDlg, IDCANCEL)))
                EndDialog(hDlg, msg);
            return 1;
        case 0x3e9:
            switch (HIWORD(wParam)) {
            case LBN_SELCHANGE:
                sel = SendMessage(g_hUserList, LB_GETCURSEL, 0, 0);
                SendMessage(g_hUserList, LB_GETTEXT, sel, (LPARAM)g_userEdit);
                SetWindowText(g_hUserEdit, g_userEdit);
                if (SendMessage(g_hUserList, LB_GETCOUNT, 0, 0) > 1)
                    EnableWindow(g_hDeleteUser, TRUE);
                break;
            case LBN_DBLCLK:
                sel = SendMessage(g_hUserList, LB_GETCURSEL, 0, 0);
                SendMessage(g_hUserList, LB_GETTEXT, sel, (LPARAM)g_userEdit);
                SetWindowText(g_hUserEdit, g_userEdit);
                SendMessage(hDlg, WM_COMMAND, IDOK, 0);
                break;
            }
            return 1;
        case 0x3ea:
            switch (HIWORD(wParam)) {
            case EN_UPDATE:
                EnableWindow(g_hDeleteUser, FALSE);
            }
            return 1;
        case 0x3eb:
            sel = SendMessage(g_hUserList, LB_GETCURSEL, 0, 0);
            SendMessage(g_hUserList, LB_GETTEXT, sel, (LPARAM)g_userEdit);
            _unlink(ReadRegString(g_hkUser, "DatabaseName", "Ben Dog"));
            DeleteUser(g_userEdit);
            SendMessage(g_hUserList, LB_DELETESTRING, sel, 0);
            n = SendMessage(g_hUserList, LB_GETCOUNT, 0, 0);
            if (sel >= n)
                sel = n - 1;
            /* sic: LB_SETSEL (fSelect = sel, item 0), which a single-selection
               list box refuses: the selection is left as it was */
            SendMessage(g_hUserList, LB_SETSEL, sel, 0);
            SendMessage(g_hUserList, LB_GETTEXT, sel, (LPARAM)g_userEdit);
            SetWindowText(g_hUserEdit, g_userEdit);
            SetFocus(g_hUserList);
            EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);
            break;
        }
        break;
    }
    return 0;
}

/* 0x411500 */
int SelectUser(void)
{
    if (DialogBoxParam(g_hInst, "SELECTUSER", 0, (DLGPROC)SelectUserDlgProc, 0) == IDCANCEL)
        return 0;
    if (g_newUser) {
        NewUser(g_selectedUser);
        return 1;
    }
    OpenUser(g_selectedUser);
    ReadIniSettings(g_szIniFile);
    return 1;
}
