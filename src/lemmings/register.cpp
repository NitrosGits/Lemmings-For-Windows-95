/*
 * Registration: the player's name, saved with a scrambled copy.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5;

#include <windows.h>
#include <string.h>
#include "winlemm.h"

int g_unused45c = 0;
HWND g_hRegEdit = 0;
char *g_key = "Visual";

char g_regName[0x200];
char g_encodedInfo[0x200];

/* 0x411a30: XOR with the key, written as letters A-P */
char *EncodeName(char *name)
{
    char *k = g_key;
    int len = strlen(name);
    int i;

    for (i = 0; i < len; i++) {
        BYTE c = name[i] ^ *k++;

        if (!*k)
            k = g_key;
        g_encodedInfo[i * 2 + 1] = (c & 0xf) + 'A';
        g_encodedInfo[i * 2] = (c >> 4) + 'A';
        g_encodedInfo[i * 2 + 2] = 0;
    }
    return g_encodedInfo;
}

/* 0x411aa0 */
int SaveRegistration(HWND hEdit)
{
    char name[256];
    int ok;

    GetWindowText(hEdit, name, sizeof(name));
    if (strlen(name))
        ok = 1;
    else
        ok = 0;
    if (ok) {
        strcpy(g_regName, name);
        WriteRegString(g_hkGame, g_szRegistered, name);
        WriteRegString(g_hkGame, g_szParams, EncodeName(name));
    }
    return ok;
}

/* 0x411b50 */
BOOL CALLBACK RegisterDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char text[1024];
    WORD id;

    switch (msg) {
    case WM_INITDIALOG:
        CentreWindow(hDlg, GetDesktopWindow());
        g_hRegEdit = GetDlgItem(hDlg, 0x3ed);
        break;
    case WM_COMMAND:
        id = LOWORD(wParam);
        if (id == IDCANCEL)
            EndDialog(hDlg, 0);
        if (id == IDOK) {
            if (SaveRegistration(g_hRegEdit)) {
                wsprintf(text, "Name : %s\n", g_regName);
                if (MessageBox(0, text, "Are You Sure?", MB_YESNO) == IDYES) {
                    EndDialog(hDlg, 1);
                    return 1;
                }
            } else
                MessageBox(0, "A Registration Name is required", "Registration Name", 0);
            return 1;
        }
        break;
    }
    return 0;
}
