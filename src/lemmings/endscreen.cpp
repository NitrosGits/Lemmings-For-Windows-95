/*
 * The end-of-level screen: the percentage rescued, a comment on it, and
 * three buttons (next level or retry, replay, choose a level).
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13;

#include <windows.h>
#include <string.h>
#include "winlemm.h"

char *g_esText[10][2] = {
    {"ROCK BOTTOM! I hope for your sake", "that you nuked that level."},
    {"Better rethink your strategy before", "you try this level again!"},
    {"A little more practice on this level", "is definitely recommended."},
    {"You got pretty close that time.", "Now try again for that few % extra."},
    {"OH NO, So near and yet so far (teehee)", "Maybe this time....."},
    {"RIGHT ON. You can't get much closer", "than that. Let's try the next...."},
    {"That level seemed no problem to you on", "that attempt. Onto the next...."},
    {"You totally stormed that level!", "Let's see if you can storm the next..."},
    {"Superb! You rescued every lemming on", "that level. Can you do it again....?"},
    {"You Cheated! You need to come back and", "try the level again ...."},
};
int g_esSaved = 0;
BYTE *g_esSprites = 0;
BYTE *g_esFont = 0;
int g_esCentre = 0;
int g_esButton1 = 0;
int g_esButton2 = 0;
int g_esButton3 = 0;
int g_esCaptured = 0;
char g_esMenu[] = "EndScreen";
int g_unused4e4 = 1;
int g_unused4e8 = 1;
int g_unused4ec = 1;
int g_unused4f0 = 1;
int g_unused4f4 = 0;
HPALETTE g_esPalette = 0;
int g_unused4fc = 0;
int g_unused500[6] = {0};
PANE g_esPanes[2] = {
    {-1, 4, g_statusText},
    {0},
};
int g_esPassed = 0;
int g_esFinished = 0;           /* that was the last level */

STATBAR g_esBarNew;
int g_valEsResult;
char g_line1Es[48];
char g_esLine2MainOwn[48];
char g_esLine3OwnNext[40];
WORD g_usedEsSlot;
WORD g_esRendererSavedVal;

/* 0x409200 */
void ShowEndScreen(void)
{
    char title[256];

    LoadGameMenu(g_esMenu);
    SetMainProc(EndScreenProc);
    ShowStatBar(&g_esBarNew, g_esPanes);
    ShowWindow(g_hwnd, SW_SHOW);
    wsprintf(title, GetString(0x115), g_userName);
    SetWindowText(g_hwnd, title);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_endhitbutton0, pad_endhitbutton1, pad_endhitbutton2, pad_endhitbutton3, pad_endhitbutton4, pad_endhitbutton5, pad_endhitbutton6, pad_endhitbutton7, pad_endhitbutton8, pad_endhitbutton9, pad_endhitbutton10;

/* 0x409280: 1-3 if a point is on a button */
int EndHitButton(int x, int y)
{
    static RECT s_buttons[3] = {
        {g_esCentre - 239, 306, 102, 95},
        {g_esCentre - 63, 306, 102, 95},
        {g_esCentre + 112, 306, 102, 95},
    };
    int i;

    for (i = 0; i < 3; i++) {
        RECT *r = &s_buttons[i];

        if (x >= r->left && x < r->left + r->right && y >= r->top && y < r->top + r->bottom)
            return i + 1;
    }
    return 0;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_endscreenproc0, pad_endscreenproc1, pad_endscreenproc2, pad_endscreenproc3, pad_endscreenproc4, pad_endscreenproc5, pad_endscreenproc6, pad_endscreenproc7, pad_endscreenproc8, pad_endscreenproc9, pad_endscreenproc10, pad_endscreenproc11, pad_endscreenproc12, pad_endscreenproc13, pad_endscreenproc14;

/* 0x409350 */
LRESULT CALLBACK EndScreenProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    int x;

    switch (msg) {
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        DrawEndScreen(hdc, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    case WM_COMMAND:
        EndScreenCommand(hwnd, msg, wParam, lParam);
        return 0;
    case WM_MOUSEMOVE:
        hdc = GetDC((HWND)g_global28);
        x = LOWORD(lParam) - g_clientWidth / 2 + g_esCentre;
        switch (EndHitButton(x, HIWORD(lParam))) {
        case 0:
            strcpy(g_statusText, "");
            break;
        case 1:
            strcpy(g_statusText, "");
            break;
        case 2:
            strcpy(g_statusText, "");
            break;
        case 3:
            strcpy(g_statusText, "");
            break;
        }
        DrawPaneText(hdc, &g_esBarNew, g_esPanes);
        ReleaseDC((HWND)g_global28, hdc);
        return 0;
    case WM_LBUTTONDOWN:
        x = LOWORD(lParam) - g_clientWidth / 2 + g_esCentre;
        switch (EndHitButton(x, HIWORD(lParam))) {
        case 1:
            g_esButton1 = 1;
            SetCapture(hwnd);
            g_esCaptured = 1;
            return 0;
        case 2:
            g_esButton2 = 1;
            SetCapture(hwnd);
            g_esCaptured = 1;
            return 0;
        case 3:
            g_esButton3 = 1;
            SetCapture(hwnd);
            g_esCaptured = 1;
            return 0;
        }
        return 0;
    case WM_LBUTTONUP:
        x = LOWORD(lParam) - g_clientWidth / 2 + g_esCentre;
        switch (EndHitButton(x, HIWORD(lParam))) {
        case 1:
            if (g_levelVal.toRescue <= g_esSaved)
                PostMessage(hwnd, WM_COMMAND, 0x117, 0);
            else
                PostMessage(hwnd, WM_COMMAND, 0x10f, 0);
            break;
        case 2:
            PostMessage(hwnd, WM_COMMAND, 0x187, 0);
            break;
        case 3:
            PostMessage(hwnd, WM_COMMAND, 0x113, 0);
            break;
        }
        g_esButton1 = g_esButton2 = g_esButton3 = 0;
        if (g_esCaptured) {
            ReleaseCapture();
            g_esCaptured = 0;
        }
        break;
    case 0x500:
        hdc = GetDC(hwnd);
        DrawEndScreen(hdc, 0);
        ReleaseDC(hwnd, hdc);
        return 0;
    case 0x603:
        PostMessage(hwnd, WM_COMMAND, 0x10f, 0);
        return 0;
    case 0x700:
        if (g_esCaptured) {
            ReleaseCapture();
            g_esCaptured = 0;
        }
        QuitEndScreen(hwnd);
        return 0;
    case 0x701:
        StartEndScreen(hwnd);
        g_esButton1 = g_esButton2 = g_esButton3 = 0;
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* 0x409700: rescued enough */
int LevelPassed(int saved, int toRescue)
{
    if (g_quitToMenu)
        return 1;
    return toRescue * 100 / g_levelVal.numLemmings <= saved * 100 / g_levelVal.numLemmings;
}

/* 0x409750: the five lines of text */
void MakeResultText(char **lines, int saved, int toRescue)
{
    int got = saved * 100 / g_levelVal.numLemmings;
    int need = toRescue * 100 / g_levelVal.numLemmings;

    if (g_esFinished) {
        strcpy(lines[0], GetString(0x7f0e));
        strcpy(lines[1], GetString(0x7f0f));
        strcpy(lines[2], GetString(0x7f10));
        strcpy(lines[3], GetString(0x7f11));
        strcpy(lines[4], GetString(0x7f12));
        return;
    }
    strcpy(lines[0], g_line1Es);
    wsprintf(lines[0], GetString(0x2005), need);
    strcpy(lines[1], g_esLine3OwnNext);
    wsprintf(lines[1], GetString(0x2006), need);
    strcpy(lines[2], g_esLine2MainOwn);
    wsprintf(lines[2], GetString(0x2007), got);
    if (g_quitToMenu)
        g_valEsResult = 9;
    else if (got == 100)
        g_valEsResult = 8;
    else if (got == 0)
        g_valEsResult = 0;
    else if (need / 2 > got)
        g_valEsResult = 1;
    else if (need - 5 > got)
        g_valEsResult = 2;
    else if (need - 1 > got)
        g_valEsResult = 3;
    else if (need - got == 1)
        g_valEsResult = 4;
    else if (need == got)
        g_valEsResult = 5;
    else {
        g_valEsResult = 6;
        if (need + 20 <= got)
            g_valEsResult = 7;
    }
    g_esPassed = LevelPassed(saved, toRescue);
    strcpy(lines[3], g_esText[g_valEsResult][0]);
    strcpy(lines[4], g_esText[g_valEsResult][1]);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawendscreen0, pad_drawendscreen1, pad_drawendscreen2, pad_drawendscreen3, pad_drawendscreen4, pad_drawendscreen5;

/* 0x409aa0 */
void DrawEndScreen(HDC hdc, PAINTSTRUCT *ps)
{
    HPALETTE old = SelectPalette(hdc, g_hPalette, FALSE);
    DISPLAYLIST *t;
    int x;

    RealizePalette(hdc);
    if (g_esButton1)
        AddSprite(g_esCentre - 239, 306, g_esSprites, 2, 1);
    if (g_esButton2)
        AddSprite(g_esCentre - 63, 306, g_esSprites, 2, 1);
    if (g_esButton3)
        AddSprite(g_esCentre + 112, 306, g_esSprites, 2, 1);
    if (g_levelVal.toRescue <= g_esSaved)
        DrawCentredText(g_esCentre - 189, 311, GetString(0x146), 1);
    else
        DrawCentredText(g_esCentre - 189, 311, GetString(0x145), 1);
    DrawCentredText(g_esCentre - 189, 339, GetString(0x147), 1);
    DrawCentredText(g_esCentre - 13, 311, GetString(0x144), 1);
    DrawCentredText(g_esCentre - 13, 339, GetString(0x145), 1);
    DrawCentredText(g_esCentre + 162, 311, GetString(0x148), 1);
    DrawCentredText(g_esCentre + 162, 339, GetString(0x147), 1);
    Renderer_RenderList(g_esRendererSavedVal, g_feListAllState);
    g_feListAllState->count = 0;
    t = g_feListAllState;
    g_feListAllState = g_feList2Extra;
    g_feList2Extra = t;
    x = g_esCentre - g_clientWidth / 2;
    if (x < 0)
        x = 0;
    if (ps) {
        if (ps->rcPaint.bottom > g_valClientHeight)
            ps->rcPaint.bottom = g_valClientHeight;
        Renderer_Revealed(g_esRendererSavedVal, g_usedEsSlot, ps->rcPaint.left + x, ps->rcPaint.top, ps->rcPaint.right + x,
                          ps->rcPaint.bottom);
    }
    Renderer_Render(g_esRendererSavedVal, hdc, g_usedEsSlot, x, 0, x + g_clientWidth, g_valClientHeight, g_clientWidth,
                    g_valClientHeight, 1);
    SelectPalette(hdc, old, FALSE);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_endscreencommand0, pad_endscreencommand1, pad_endscreencommand2, pad_endscreencommand3, pad_endscreencommand4, pad_endscreencommand5, pad_endscreencommand6, pad_endscreencommand7, pad_endscreencommand8;

/* 0x409d10 */
LRESULT EndScreenCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int code = HIWORD(wParam);

    switch (LOWORD(wParam)) {
    case 0x10f:
        StartGame(g_hInst, SW_SHOW);
        break;
    case 0x113:
        SelectLevelDialog(hwnd);
        break;
    case 0x115:
        DestroyWindow(hwnd);
        break;
    case 0x117:
        PlayLevel(hwnd);
        break;
    case 0x13a:
        if (!WinHelp(hwnd, "LEMMINGS.HLP", HELP_KEY, (DWORD)"CONTENTS"))
            ErrorBox(0x7f01);
        break;
    case 0x13c:
        if (!WinHelp(hwnd, "LEMMINGS.HLP", HELP_PARTIALKEY, (DWORD)""))
            ErrorBox(0x7f01);
        break;
    case 0x13e:
        if (!WinHelp(hwnd, 0, HELP_HELPONHELP, 0))
            ErrorBox(0x7f01);
        break;
    case 0x140:
        DialogBoxParam(g_hInst, "AboutBox", hwnd, (DLGPROC)AboutDlgProc, 0);
        break;
    case 0x177:
        DialogBoxParam(g_hInst, "OptionsDlg", hwnd, (DLGPROC)OptionsDlgProc, 0);
        break;
    case 0x187:                 /* replay */
        if (g_esPassed)
            UndoNextLevel();
        g_replaying = 1;
        PostMessage(hwnd, WM_COMMAND, 0x10f, 0);
        break;
    case 0x4d2:
        if (!code)
            PlayLevel(hwnd);
        break;
    case 0x929:
        if (!code)
            PostMessage(hwnd, WM_COMMAND, 0x113, 0);
        break;
    case 0xd80:
        if (!code)
            PostMessage(hwnd, WM_COMMAND, 0x10f, 0);
        break;
    case 0x9c47:
        if (SelectUser())
            InitFrontEnd();
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* 0x409f90: a character's frame in gfx\es.spr */
int EndCharIndex(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 30;
    if (c >= '0' && c <= '9')
        return c - '0' + 70;
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 4;
    switch (c) {
    case '!':
        return 69;
    case '$':
        return 59;
    case '%':
        return 66;
    case '&':
        return 56;
    case '\'':
        return 68;
    case '(':
        return 60;
    case ')':
        return 65;
    case ',':
        return 62;
    case '-':
        return 67;
    case '.':
        return 61;
    case ':':
        return 64;
    case ';':
        return 63;
    case '?':
        return 57;
    }
    return -1;
}

/* 0x40a090 */
int SpriteWidth(BYTE *sprites, int frame)
{
    WORD *p = (WORD *)(sprites + *(DWORD *)(sprites + 6 + frame * 4));

    return p[0] + p[2] - 3;
}

/* 0x40a0b0: the width of the widest line */
int EndTextWidth(char *text)
{
    int max = 0;
    int w = 0;

    while (*text) {
        switch (*text) {
        case '\n':
            if (w > max)
                max = w;
            w = 0;
            break;
        case ' ':
            w += 8;
            break;
        default:
            w += SpriteWidth(g_esSprites, EndCharIndex(*text));
            break;
        }
        text++;
    }
    if (w > max)
        max = w;
    return max;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawendtext0, pad_drawendtext1, pad_drawendtext2, pad_drawendtext3, pad_drawendtext4, pad_drawendtext5, pad_drawendtext6, pad_drawendtext7, pad_drawendtext8, pad_drawendtext9, pad_drawendtext10, pad_drawendtext11, pad_drawendtext12, pad_drawendtext13, pad_drawendtext14, pad_drawendtext15;

/* 0x40a110 */
void DrawEndText(int x, int y, char *text, int mode)
{
    int x0 = x;
    int f;

    while (*text) {
        switch (*text) {
        case '\n':
            x = x0;
            y += 28;
            break;
        case ' ':
            x += 8;
            break;
        default:
            f = EndCharIndex(*text);
            AddSprite(x, y, g_esSprites, f, mode);
            x += SpriteWidth(g_esSprites, f);
            break;
        }
        text++;
    }
}

/* 0x40a190 */
void DrawCentredText(int x, int y, char *text, int mode)
{
    int w = EndTextWidth(text);

    DrawEndText(x - w / 2, y, text, mode);
}

/* 0x40a1c0 */
void StartEndScreen(HWND hwnd)
{
    BYTE *pal;
    PDIB dib;
    PDIB dibTmp;
    int cols;
    int rows;
    int i;
    int x;
    int y;
    char *lines[5];
    char text[5][129];

    g_esSaved = GetLemmsHome();
    if (g_cheat)
        g_esSaved = g_levelVal.toRescue;
    FitWindow(g_hwnd);
    g_esPassed = LevelPassed(g_esSaved, g_levelVal.toRescue);
    g_esFinished = 0;
    if (g_esPassed) {
        g_esFinished = IsLastLevel();
        NextLevel();
        NextTune();
    }
    if (!g_esRendererSavedVal)
        g_esRendererSavedVal = Renderer_Initialise(0);
    pal = LoadPalFile("gfx\\fe.pal");
    dibTmp = CreateFrontDib(pal);
    dib = dibTmp;
    VSMEM_Free(pal);
    g_usedEsSlot = LoadFrontCanvas(g_esRendererSavedVal, dib);
    SetFrontPalette(dib);
    g_feListAllState = NewDisplayList(256);
    g_feList2Extra = NewDisplayList(256);
    g_feListAllState->count = 0;
    g_feList2Extra->count = 0;
    for (i = 0; i < 256; i++) {
        g_feListAllState->entries[i].slot = g_usedEsSlot;
        g_feList2Extra->entries[i].slot = g_usedEsSlot;
    }
    g_esSprites = LoadSpriteFile("gfx\\es.spr");
    g_esCentre = (WORD)GetDibWidth(dib) >> 1;
    cols = (GetDibWidth(dib) + 158) / 159;
    rows = (GetDibHeight(dib) + 135) / 136;
    for (y = 0; y < rows; y++) {
        for (x = 0; x < cols; x++)
            AddSprite(x * 159, y * 136, g_esSprites, 0, 3);
    }
    AddSprite(g_esCentre - 309, 10, g_esSprites, 3, 3);
    AddSprite(g_esCentre - 239, 306, g_esSprites, 1, 3);
    AddSprite(g_esCentre - 63, 306, g_esSprites, 1, 3);
    AddSprite(g_esCentre + 112, 306, g_esSprites, 1, 3);
    lines[0] = text[0];
    lines[1] = text[1];
    lines[2] = text[2];
    lines[3] = text[3];
    lines[4] = text[4];
    MakeResultText(lines, g_esSaved, g_levelVal.toRescue);
    for (i = 0; i < 5; i++)
        DrawCentredText(g_esCentre, 100 + i * 28, lines[i], 3);
    Renderer_RenderList(g_esRendererSavedVal, g_feListAllState);
    FreeDib(dib);
}

/* 0x40a4c0 */
void QuitEndScreen(HWND hwnd)
{
    if (g_fePalette)
        DeleteObject(g_fePalette);
    if (g_esSprites) {
        VSMEM_Free(g_esSprites);
        g_esSprites = 0;
    }
    if (g_esFont) {
        VSMEM_Free(g_esFont);
        g_esFont = 0;
    }
    if (g_esRendererSavedVal) {
        Renderer_Quit(g_esRendererSavedVal);
        g_esRendererSavedVal = 0;
    }
    FreeDisplayList(g_feListAllState);
    FreeDisplayList(g_feList2Extra);
}
