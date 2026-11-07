/*
 * Settings: the player's options in the registry, the registration check,
 * and installing the uninstaller.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14, pad_top15, pad_top16, pad_top17, pad_top18, pad_top19;

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include "winlemm.h"

int g_zoom = -1;
int g_currentLevel = -1;
int g_musicAvailable = 0;
int g_waveAvailable = 0;
int g_music = 0;
int g_soundFX = 0;
int g_visualFX = 0;
int g_pauseFocus = 0;
int g_unused58 = 0;
int g_volume = 0;
char g_iniWinSection[] = "WinSection";
char g_iniGameSection[] = "GameSection";
char g_iniOptionsSection[] = "OptionsSection";
char g_iniInstall[] = "install";
char g_iniGraphics[] = "Graphics";
char g_iniWidth[] = "Width";
char g_iniHeight[] = "Height";
char g_iniMaximised[] = "Maximised";
char g_iniZoom[] = "Zoom";
char g_iniLevel[] = "Level";
char g_iniResolution[] = "Resolution";
char g_iniCheat[] = "Cheat";
char g_iniMusic[] = "Music";
char g_iniSoundFX[] = "SoundFX";
char g_iniVisualFX[] = "VisualFX";
char g_iniPauseFocus[] = "PauseFocus";
char g_iniIconDocked[] = "IconDocked";
char g_iniIconX[] = "IconX";
char g_iniIconY[] = "IconY";
char g_iniMapDocked[] = "MapDocked";
char g_iniMapX[] = "MapX";
char g_iniMapY[] = "MapY";
char g_szRegistered[] = "Registered";
char g_szSerialNum[] = "SerialNum";
char g_szParams[] = "Params";
int g_taster = -1;

char g_regUserUsed[84];
int g_cheat;
int g_maximised;
char g_dbHeader[128];
RECT g_rectStatus;
RECT g_viewRectSpare;
int g_mapDock, g_savedIconDock;
int g_mapXUsed, g_usedMapY;            /* not POINTs: the original keeps them 4-aligned in .bss */
int g_dataUnused8bc, g_unused8c0Cur, g_unused8c4TempGame, g_unused8c8OwnTemp, g_unused8ccTemp;
int g_iconXInfo, g_mainIconY;
int g_resolutionNew;
int g_mainWidthNext, g_mainHeightOldTemp;

/* 0x407ce0: the free taster edition's database lets everyone in */
int IsTaster(void)
{
    HFILE f;

    if (g_taster != -1)
        return g_taster;
    {
        g_taster = 0;
        f = _lopen("users\\lemmings.db", OF_READ);
        if (f != HFILE_ERROR) {
            _lread(f, g_dbHeader, sizeof(g_dbHeader));
            _lclose(f);
            if (!strcmp(g_dbHeader + 0x12, "Taster Levels...."))
                g_taster = 1;
        }
    }
    return g_taster;
}

/* 0x407d70: does the scrambled copy decode to the name? */
int CheckSerial(char *params, char *name)
{
    static char *key = "Visual";        /* a static local: .data puts it with this function's literals */
    char decoded[164];
    char *k = key;
    int len = strlen(name);
    int i;

    for (i = 0; i < len; i++) {
        decoded[i] = (((params[0] - 'A') << 4) | (params[1] - 'A')) ^ *k++;
        if (!*k)
            k = key;
        params += 2;
        decoded[i + 1] = 0;
    }
    return strcmp(decoded, name) == 0;
}

/* 0x407e10 */
int IsRegistered(void)
{
    char params[164];

    if (IsTaster())
        return 1;
    strcpy(g_regUserUsed, ReadRegString(g_hkGame, g_szRegistered, "the same"));
    strcpy(params, ReadRegString(g_hkGame, g_szParams, "the same"));
    if (!strcmp(g_regUserUsed, "Ben Dog"))
        return 1;
    return CheckSerial(params, g_regUserUsed);
}

/* 0x407f10 */
int CopyFileTo(char *dest, char *src)
{
    char buf[0x800];
    FILE *out;
    FILE *in;
    long left;
    int n;
    int got;
    int written;

    out = fopen(dest, "wb");
    if (!out)
        return 0;
    in = fopen(src, "rb");
    if (!in) {
        fclose(out);
        return 0;
    }
    left = _filelength((_fileno)(in));
    while (left > 0) {
        n = left < 0x800 ? left : 0x800;
        got = fread(buf, 1, n, in);
        written = fwrite(buf, 1, n, out);
        if (got != n || written != n) {
            fclose(in);
            fclose(out);
            DeleteFile(dest);
            return 0;
        }
        left -= n;
    }
    fclose(in);
    fclose(out);
    return 1;
}

/* 0x408020 */
void InstallUninstaller(void)
{
    char path[0x400];
    char *dir = GetUserDir();
    HKEY key;

    wsprintf(path, "%s\\wlvsun10.exe", dir);
    if (!CopyFileTo(path, "autorun.exe")) {
        MessageBox(0, "Unable to install the uninstall program\nThis feature will be unavailable from the Control Panel",
                   "Uninstall Installation", 0);
        return;
    }
    key = OpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Windows Lemmings",
                  "Visual Sciences data");
    WriteRegString(key, "DisplayName", "Lemmings for Windows 95");
    wsprintf(path, "%s\\wlvsun10.exe uninstall", dir);
    WriteRegString(key, "UninstallString", path);
}

/* 0x4080e0 */
void ReadIniSettings(char *file)
{
    RECT r;
    POINT pt;

    if (!IsRegistered()) {
        if (!DialogBoxParam(g_hInst, MAKEINTRESOURCE(0x68), GetDesktopWindow(), (DLGPROC)RegisterDlgProc, 0))
            ExitProcess(0xaaaa);
        IsRegistered();
        OpenUser(g_regUserUsed);
        CreateUserDatabase(g_regUserUsed);
        InstallUninstaller();
    }
    if (!strcmp(g_userName, "Ben Dog"))
        OpenUser(g_regUserUsed);
    g_mainWidthNext = ReadRegDword(g_hkUser, g_iniWidth, 100);
    g_mainHeightOldTemp = ReadRegDword(g_hkUser, g_iniHeight, 295);
    g_maximised = ReadRegDword(g_hkUser, g_iniMaximised, 0);
    g_savedIconDock = ReadRegDword(g_hkUser, g_iniIconDocked, 1);
    g_mapDock = ReadRegDword(g_hkUser, g_iniMapDocked, 1);
    g_iconXInfo = ReadRegDword(g_hkUser, g_iniIconX, 0);
    g_mainIconY = ReadRegDword(g_hkUser, g_iniIconY, 0);
    g_mapXUsed = ReadRegDword(g_hkUser, g_iniMapX, 0);
    g_usedMapY = ReadRegDword(g_hkUser, g_iniMapY, 0);
    g_zoom = ReadRegDword(g_hkUser, g_iniZoom, 1);
    g_currentLevel = ReadRegDword(g_hkUser, g_iniLevel, 0);
    g_resolutionNew = ReadRegDword(g_hkUser, g_iniResolution, 1);
    g_cheat = ReadRegDword(g_hkUser, g_iniCheat, 0);
    g_music = ReadRegDword(g_hkUser, g_iniMusic, 0);
    g_soundFX = ReadRegDword(g_hkUser, g_iniSoundFX, 0);
    g_visualFX = ReadRegDword(g_hkUser, g_iniVisualFX, 0);
    g_pauseFocus = ReadRegDword(g_hkUser, g_iniPauseFocus, 0);
    SetSFXFlags(g_visualFX, g_soundFX);
    SetMusic(g_music);
    if (g_memStatus.dwTotalPhys <= 0x400000)
        g_resolutionNew = 1;
    {
        HWND desktop = GetDesktopWindow();
        GetClientRect(desktop, &r);
    }
    r.right -= 20;
    r.bottom -= 20;
    pt.x = g_mainIconY;    /* sic: x and y swapped */
    pt.y = g_iconXInfo;
    if (g_savedIconDock == -1 && !PtInRect(&r, pt))
        g_savedIconDock = 1;
    pt.x = g_usedMapY;
    pt.y = g_mapXUsed;
#ifdef _MSC_VER
    switch (pt.x) { case 1: break; }
#endif
    if (g_mapDock == -1 && !PtInRect(&r, pt))
        g_mapDock = 1;
}

/* 0x408400 */
void WriteIniSettings(char *file)
{
    WriteRegDword(g_hkUser, g_iniWidth, g_mainWidthNext);
    WriteRegDword(g_hkUser, g_iniHeight, g_mainHeightOldTemp);
    WriteRegDword(g_hkUser, g_iniMaximised, g_maximised);
    WriteRegDword(g_hkUser, g_iniIconDocked, g_savedIconDock);
    WriteRegDword(g_hkUser, g_iniMapDocked, g_mapDock);
    WriteRegDword(g_hkUser, g_iniIconX, g_iconXInfo);
    WriteRegDword(g_hkUser, g_iniIconY, g_mainIconY);
    WriteRegDword(g_hkUser, g_iniMapX, g_mapXUsed);
    WriteRegDword(g_hkUser, g_iniMapY, g_usedMapY);
    WriteRegDword(g_hkUser, g_iniZoom, g_zoom);
    WriteRegDword(g_hkUser, g_iniLevel, g_currentLevel);
    WriteRegDword(g_hkUser, g_iniResolution, g_resolutionNew);
    WriteRegDword(g_hkUser, g_iniMusic, g_music);
    WriteRegDword(g_hkUser, g_iniSoundFX, g_soundFX);
    WriteRegDword(g_hkUser, g_iniVisualFX, g_visualFX);
    WriteRegDword(g_hkUser, g_iniPauseFocus, g_pauseFocus);
}
