/*
 * The game's registry keys: HKLM\Software\Visual Sciences\Windows Lemmings,
 * with a key per player under Users.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11;

#include <windows.h>
#include <string.h>
#include "winlemm.h"

HKEY g_hkGame = 0;
HKEY g_hkUsers = 0;
HKEY g_hkUser = 0;
char *g_userName = 0;

char g_userNameBuf[0x800];
char *g_hereUserList[256];
char g_stringReg[0x400];

/* 0x411560 */
void OpenRegistry(void)
{
    char *last;

    g_hkGame = OpenKey(HKEY_LOCAL_MACHINE, "Software\\Visual Sciences\\Windows Lemmings", "Visual Sciences data");
    if (!g_hkGame)
        MessageBox(0, "Unable to open registry for game, user information and options cannot be loaded",
                   "Warning", MB_ICONEXCLAMATION);
    if ((g_hkGame = g_hkGame) != 0) {
        g_hkUsers = OpenKey(g_hkGame, "Users", "Visual Sciences data");
        last = ReadRegString(g_hkUsers, "LastUser", 0);
        if (g_hkUsers) {
            if (last)
                OpenUser(last);
        } else
            MessageBox(0, "Unable to open registry for all users, user information and options cannot be loaded",
                       "Warning", MB_ICONEXCLAMATION);
    }
}

/* 0x411600 */
void CloseRegistry(void)
{
    if (g_hkUsers) {
        CloseKey(g_hkUsers);
        g_hkUsers = 0;
    }
    if (g_hkUser) {
        CloseKey(g_hkUser);
        g_hkUser = 0;
    }
    if (g_hkGame) {
        CloseKey(g_hkGame);
        g_hkGame = 0;
    }
    if (g_userName) {
        delete g_userName;
        g_userName = 0;
    }
}

/* 0x411680 */
HKEY OpenUser(char *name)
{
    char text[256];

    if (!name)
        return 0;
    if (g_hkUser) {
        CloseKey(g_hkUser);
        g_hkUser = 0;
    }
    g_hkUser = OpenKey(g_hkUsers, name, "Visual Sciences data");
    if (!g_hkUser) {
        wsprintf(text, "Unable to open registry for user %s, user information and options cannot be accessed", name);
        MessageBox(0, text, "Warning", MB_ICONEXCLAMATION);
    }
    if (g_userName) {
        delete g_userName;
        g_userName = 0;
    }
    g_userName = new char[strlen(name) + 1];
    strcpy(g_userName, name);
    WriteRegString(g_hkUsers, "LastUser", g_userName);
    return g_hkUser;
}

/* 0x411780: the players' names */
int EnumUsers(char ***names)
{
    DWORD len;
    FILETIME time;
    int done;
    int n;
    char *p;
    int left;

    p = g_userNameBuf;
    left = sizeof(g_userNameBuf);
    done = 0;
    n = 0;
    memset(g_userNameBuf, 0, sizeof(g_userNameBuf));
    memset(g_hereUserList, 0, sizeof(g_hereUserList));
    do {
        len = left;
        if (RegEnumKeyEx(g_hkUsers, n, p, &len, 0, 0, 0, &time))
            done = 1;
        else {
            g_hereUserList[n] = p;
            p[len] = 0;
            p += len + 1;
            left -= len + 1;
            n++;
        }
    } while (!done);
    *names = g_hereUserList;
    return n;
}

/* 0x411820 */
void DeleteUser(char *name)
{
    RegDeleteKey(g_hkUsers, name);
    RegFlushKey(g_hkUsers);
}

/* 0x411840: open a key, creating it if need be */
HKEY OpenKey(HKEY parent, char *subkey, char *cls)
{
    HKEY key;
    DWORD disposition;

    if (!parent)
        return 0;
    key = 0;
    if (RegOpenKeyEx(parent, subkey, 0, KEY_ALL_ACCESS, &key)) {
        disposition = 0;
        if (RegCreateKeyEx(parent, subkey, 0, cls, 0, KEY_ALL_ACCESS, 0, &key, &disposition))
            return 0;
        if (disposition != REG_CREATED_NEW_KEY && disposition != REG_OPENED_EXISTING_KEY)
            return 0;
    }
    return key;
}

/* 0x4118e0 */
void CloseKey(HKEY key)
{
    RegFlushKey(key);
    RegCloseKey(key);
}

/* 0x411900 */
void WriteRegDword(HKEY key, char *name, DWORD value)
{
    if (RegSetValueEx(key, name, 0, REG_DWORD, (BYTE *)&value, sizeof(DWORD)))
        MessageBox(0, name, "Unable to save DWORD in registry key", 0);
}

/* 0x411940 */
void WriteRegString(HKEY key, char *name, char *value)
{
    if (RegSetValueEx(key, name, 0, REG_SZ, (BYTE *)value, strlen(value) + 1))
        MessageBox(0, name, "Unable to save STRING in registry key", 0);
}

/* 0x411980 */
DWORD ReadRegDword(HKEY key, char *name, DWORD def)
{
    DWORD value;
    DWORD type;
    DWORD size;

    type = 0xffffffff;
    value = 0xffffffff;
    size = sizeof(DWORD);
    if (!RegQueryValueEx(key, name, 0, &type, (BYTE *)&value, &size) && type == REG_DWORD)
        return value;
    return def;
}

/* 0x4119e0 */
char *ReadRegString(HKEY key, char *name, char *def)
{
    DWORD type = 0xffffffff;
    DWORD size = sizeof(g_stringReg);

    if (!RegQueryValueEx(key, name, 0, &type, (BYTE *)g_stringReg, &size) && type == REG_SZ)
        return g_stringReg;
    return def;
}
