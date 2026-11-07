/*
 * The level database (users\<name>.db) and the Select Level dialog: a
 * tree of ratings, groups and levels, a picture of the level chosen and
 * its numbers.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1;

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "winlemm.h"

char g_levelFile[200] = "lvl0000.lvl";
int g_dlgReady = 0;
struct {
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[256];
} g_dlgLogPal = {0x300, 256};
DATABASE *g_db = 0;
int g_dbIndex = -1;
DRAWFUNC g_drawFuncs[13] = {
    DrawPreview,
    DrawDialogIcon, DrawDialogIcon, DrawDialogIcon, DrawDialogIcon, DrawDialogIcon, DrawDialogIcon,
    DrawDialogIcon, DrawDialogIcon, DrawDialogIcon, DrawDialogIcon, DrawDialogIcon,
    DrawBlank,
};

char g_selPath[80];
HBRUSH g_gameDlgBrush;
HBITMAP g_bitmapDlgOld;
int g_savedDbIndexNowTemp;
int g_savedLevelNowCur;
int g_infoLsSkills[8];
int g_lsLemmingsSpareMain;
HBITMAP g_newPreview;
HDC g_dlgDC;
HPALETTE g_hereDlgPalette;
int g_spareLsTime;
char g_lsName[52];
int g_lastLsCustom;
HWND g_ownPreviewWnd;
int g_stateLsRate;
HBITMAP g_oldDlgBitmap2;
TREE g_treeNext;
HPALETTE g_previewPalNewUsed;
int g_lsRescueState;
BYTE g_lvlBuf[0x800];
HPALETTE g_dlgOldPalette;
HDC g_dlgDC2New;
char g_selNameInfoOwn[80];
HBITMAP g_dlgBitmapAll;

/* 0x40a540 */
int InitLevelTree(void)
{
    return TreeLoadBitmaps(g_hInst, 0x145, 4, 7, 1, &g_treeNext, 0);
}

/* 0x40a570 */
int RegisterIconClasses(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.hCursor = 0;
    wc.hIcon = 0;
    wc.lpszMenuName = 0;
    wc.lpszClassName = "IconImageClass";
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpfnWndProc = IconImageProc;
    wc.style = CS_DBLCLKS;
    wc.cbWndExtra = 4;
    wc.cbClsExtra = 0;
    if (!RegisterClass(&wc))
        return 0;
    wc.hCursor = 0;
    wc.hIcon = 0;
    wc.lpszMenuName = 0;
    wc.lpszClassName = "IconTextClass";
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpfnWndProc = IconTextProc;
    wc.style = CS_DBLCLKS;
    wc.cbWndExtra = 4;
    wc.cbClsExtra = 0;
    return RegisterClass(&wc) != 0;
}

/* 0x40a630 */
int SelectLevelDialog(HWND hwnd)
{
    LOGBRUSH lb;
    int r;

    lb.lbStyle = BS_SOLID;
    lb.lbColor = GetSysColor(COLOR_BTNFACE);
    lb.lbHatch = 0;
    g_gameDlgBrush = CreateBrushIndirect(&lb);
    g_newPreview = 0;
    g_dlgReady = 0;
    g_dlgDC = 0;
    g_dlgDC2New = 0;
    TreeLoadBitmaps(g_hInst, 0x145, 4, 7, 1, &g_treeNext, 1);
    r = DialogBoxParam(g_hInst, MAKEINTRESOURCE(0x11c), hwnd, (DLGPROC)LevelDlgProc, 0);
    if (r == -1)
        MessageBox(hwnd, "Failed to make dialog box", "Error", 0);
    TreeFreeBitmaps(&g_treeNext);
    return r;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_loaddatabase0, pad_loaddatabase1, pad_loaddatabase2, pad_loaddatabase3, pad_loaddatabase4, pad_loaddatabase5, pad_loaddatabase6, pad_loaddatabase7, pad_loaddatabase8, pad_loaddatabase9, pad_loaddatabase10;

/* 0x40a6e0 */
int LoadDatabase(void)
{
    HFILE f;
    DWORD size;

    g_db = 0;
    f = _lopen(GetUserDatabase(), OF_READ);
    if (f != HFILE_ERROR) {
        size = _llseek(f, 0, 2);
        _llseek(f, 0, 0);
        g_db = (DATABASE *)VSMEM_Allocate(size, "DataBase");
        _lread(f, g_db, size);
        _lclose(f);
        return 1;
    }
    MessageBox(0, "Oh no ! No DataBase", "Dialog Error", 0);
    return 0;
}

/* 0x40a760 */
void SaveDatabase(void)
{
    HFILE f;

    if (g_db) {
        f = _lopen(GetUserDatabase(), OF_WRITE);
        if (f != HFILE_ERROR) {
            _lwrite(f, (char *)g_db, 0x91);
            _lwrite(f, (char *)g_db + 0x91, g_db->count * sizeof(DBENTRY) - sizeof(DBENTRY));
            _lclose(f);
        }
        VSMEM_Free(g_db);
    }
}

/* 0x40a7d0: the rating a level is in */
DBENTRY *FindRating(int i)
{
    DBENTRY *found = 0;
    int j;

    for (j = 0; j < i; j++) {
        if (g_db->entries[j].type == 0x101)
            found = &g_db->entries[j];
    }
    return found;
}

/* 0x40a800: the level's number counting from 1 */
int LevelNumber(int i)
{
    int n = 0;
    int j;
    int count = g_db->count;

    for (j = 0; j < g_db->count; j++) {
        if (g_db->entries[j].type == 3) {
            n++;
            if (j == i)
                return n;
        }
    }
    return 0;
}

/* 0x40a840 */
void CloseLevelDialog(HWND hDlg, int ok)
{
    EndDialog(hDlg, ok);
    if (g_dlgBitmapAll) {
        DeleteObject(g_dlgBitmapAll);
        g_dlgBitmapAll = 0;
    }
    if (g_gameDlgBrush) {
        DeleteObject(g_gameDlgBrush);
        g_gameDlgBrush = 0;
    }
    SaveDatabase();
    TreeFreeOpen(&g_treeNext);
    if (g_hereDlgPalette)
        DeleteObject(g_hereDlgPalette);
    if (g_previewPalNewUsed)
        DeleteObject(g_previewPalNewUsed);
    if (g_newPreview)
        DeleteObject(g_newPreview);
    if (g_dlgDC) {
        SelectPalette(g_dlgDC, g_dlgOldPalette, FALSE);
        SelectObject(g_dlgDC, g_bitmapDlgOld);
        DeleteDC(g_dlgDC);
    }
    if (g_dlgDC2New) {
        SelectObject(g_dlgDC2New, g_oldDlgBitmap2);
        DeleteDC(g_dlgDC2New);
    }
    if (ok) {
        strcpy(g_levelFile, "level\\");
        strcat(g_levelFile, g_selPath);
        g_replaying = 0;
        SendMessage(g_hwnd, 0x603, 0, 0);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_leveldlgproc0, pad_leveldlgproc1, pad_leveldlgproc2;

/* 0x40a9c0 */
BOOL CALLBACK LevelDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HWND list;
    WORD sel;
    LRESULT data;
    switch (msg) {
    case WM_PAINT:
        BeginPaint(hDlg, &ps);
        EndPaint(hDlg, &ps);
        return TRUE;
    case WM_DRAWITEM:
        DrawLevelItem(hDlg, (DRAWITEMSTRUCT *)lParam);
        return TRUE;
    case WM_MEASUREITEM:
        TreeMeasureItem(hDlg, (MEASUREITEMSTRUCT *)lParam, &g_treeNext);
        return TRUE;
    case WM_CHARTOITEM:
        if (wParam == 13) {
            HWND box = GetDlgItem(hDlg, 0x11d);
            sel = (WORD)SendMessage(box, LB_GETCURSEL, 0, 0);
            data = SendMessage(box, LB_GETITEMDATA, sel, 0);
            OpenLevelItem(hDlg, box, data, sel);
            return TRUE;
        }
        break;
    case WM_SETFONT:
        if (g_dlgReady)
            TreeSetFont(GetDlgItem(hDlg, 0x11d), (HFONT)wParam, &g_treeNext);
        else
            TreeSetFont(0, (HFONT)wParam, &g_treeNext);
        break;
    case WM_INITDIALOG:
        LoadDatabase();
        FillLevelList(hDlg, wParam, lParam);
        g_dlgBitmapAll = LoadDIBitMap("gfx\\dialog.bmp", &g_hereDlgPalette);
        if (!g_dlgBitmapAll)
            MessageBox(0, "No bitmap loaded - dialog.bmp", "Dialog error", 0);
        GetPaletteEntries(g_hereDlgPalette, 0, 256, g_dlgLogPal.palPalEntry);
        g_ownPreviewWnd = GetDlgItem(hDlg, 0x14a);
        SetWindowWord(g_ownPreviewWnd, 0, 0);
        list = GetDlgItem(hDlg, 0x147);
        SetWindowWord(list, 0, 9);
        list = GetDlgItem(hDlg, 0x148);
        SetWindowWord(list, 0, 10);
        list = GetDlgItem(hDlg, 0x149);
        SetWindowWord(list, 0, 11);
        list = GetDlgItem(hDlg, 0x14b);
        SetWindowWord(list, 0, 1);
        list = GetDlgItem(hDlg, 0x14c);
        SetWindowWord(list, 0, 2);
        list = GetDlgItem(hDlg, 0x14d);
        SetWindowWord(list, 0, 3);
        list = GetDlgItem(hDlg, 0x14e);
        SetWindowWord(list, 0, 4);
        list = GetDlgItem(hDlg, 0x14f);
        SetWindowWord(list, 0, 5);
        list = GetDlgItem(hDlg, 0x150);
        SetWindowWord(list, 0, 6);
        list = GetDlgItem(hDlg, 0x151);
        SetWindowWord(list, 0, 7);
        list = GetDlgItem(hDlg, 0x152);
        SetWindowWord(list, 0, 8);
        g_dlgReady = 1;
        return TRUE;
    case WM_COMMAND:
        LevelDlgCommand(hDlg, LOWORD(wParam), (HWND)LOWORD(lParam), HIWORD(wParam));
        return TRUE;
    case WM_CTLCOLORDLG:
        return (BOOL)g_gameDlgBrush;
    }
    return FALSE;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_findnextgroup0, pad_findnextgroup1, pad_findnextgroup2;

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_findnextgroup0, pad_findnextgroup1, pad_findnextgroup2, pad_findnextgroup3, pad_findnextgroup4, pad_findnextgroup5, pad_findnextgroup6, pad_findnextgroup7, pad_findnextgroup8, pad_findnextgroup9, pad_findnextgroup10, pad_findnextgroup11, pad_findnextgroup12, pad_findnextgroup13, pad_findnextgroup14, pad_findnextgroup15;

/* 0x40acd0: the next group after a level */
DBENTRY *FindNextGroup(int i)
{
    for (; i < g_db->count - 1; i++) {
        switch (g_db->entries[i].type) {
        case 0x102:
            return &g_db->entries[i];
        }
    }
    return 0;
}

/* 0x40ad30 */
int NextIsLevel(int i)
{
    if (g_db->count - i == 1)
        return 0;
    switch (g_db->entries[i + 1].type) {
    case 3:
        return 1;
    }
    return 0;
}

/* 0x40ad70 */
int LevelDone(int i)
{
    if (g_db->count - 1 >= i && g_db->entries[i].status == 1)
        return 1;
    return 0;
}

/* 0x40adb0 */
int LevelCheated(int i)
{
    if (g_db->count - 1 >= i && g_db->entries[i].status == 2)
        return 1;
    return 0;
}

/* 0x40adf0: every level of a group done */
int GroupDone(int i)
{
    for (i++; i < g_db->count; i++) {
        switch (g_db->entries[i].type) {
        case 3:
            if (!g_db->entries[i].status)
                return 0;
            break;
        case 0x101:
        case 0x102:
            return 1;
        }
    }
    return 1;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_ratingdone0, pad_ratingdone1, pad_ratingdone2;

/* 0x40ae60: every level of a rating done */
int RatingDone(int i)
{
    for (i++; i < g_db->count - 1; i++) {
        switch (g_db->entries[i].type) {
        case 3:
            if (!g_db->entries[i].status)
                return 0;
            break;
        case 0x101:
            return 1;
        }
    }
    return 1;
}

/* 0x40aed0 */
int ItemDone(int i)
{
    switch (g_db->entries[i].type) {
    case 3:
        return LevelDone(i);
    case 0x101:
        return RatingDone(i);
    case 0x102:
        return GroupDone(i);
    }
    return 0;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawlevelitem0;

/* 0x40af30 */
void DrawLevelItem(HWND hDlg, DRAWITEMSTRUCT *dis)
{
    int i = dis->itemData;
    int type = g_db->entries[i].type;
    int status = g_db->entries[i].status;
    int lines = g_db->entries[i].unknown6;
    char text[128];
    int row;
    int level;
    int col;

    strcpy(text, g_db->entries[i].name);
    if (type & 0x100) {
        if (TreeIsOpen(&g_treeNext, i))
            col = ItemDone(i) ? 4 : 1;
        else
            col = ItemDone(i) ? 3 : 0;
    } else {
        if (LevelCheated(i))
            col = 6;
        else
            col = ItemDone(i) ? 5 : 2;
    }
    if (type == 0x101) {
        row = 0;
        level = 0;
    } else if (type == 0x102) {
        level = 1;
        row = 3;
    } else {
        row = level = 2;
        lines = FindNextGroup(i) != 0;
        if (status && NextIsLevel(i))
            lines += row;
    }
    TreeDrawItem(hDlg, dis, level, lines, text, row, col, &g_treeNext);
}

/* 0x40b090 */
int FillLevelList(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    int i;
    HWND list = GetDlgItem(hDlg, 0x11d);
    int id;

    for (i = 0; i < g_db->count; i++) {
        id = g_db->entries[i].id;
        if (g_db->entries[i].type == 0x101)
            SendMessage(list, LB_ADDSTRING, 0, id);
    }
    return 1;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_leveldlgcommand0;

/* 0x40b100 */
void LevelDlgCommand(HWND hDlg, int id, HWND ctl, int code)
{
    WORD sel;
    LRESULT data;

    GetDlgItem(hDlg, IDOK);
    GetDlgItem(hDlg, IDCANCEL);
    if (id == 0x11d && code == LBN_DBLCLK) {
        sel = (WORD)SendMessage(ctl, LB_GETCURSEL, 0, 0);
        data = SendMessage(ctl, LB_GETITEMDATA, sel, 0);
        OpenLevelItem(hDlg, ctl, data, sel);
    }
    if (id == 0x11d && code == LBN_SELCHANGE) {
        sel = (WORD)SendMessage(ctl, LB_GETCURSEL, 0, 0);
        data = SendMessage(ctl, LB_GETITEMDATA, sel, 0);
        SelectLevelItem(hDlg, ctl, data, sel);
    }
    if (id == IDOK && code == 0) {
        CloseLevelDialog(hDlg, 1);
        return;
    }
    if (id == IDCANCEL && code == 0)
        CloseLevelDialog(hDlg, 0);
}

/* 0x40b210 */
void SelectLevelItem(HWND hDlg, HWND list, int i, WORD sel)
{
    HWND ok = GetDlgItem(hDlg, IDOK);
    HWND item;

    if (g_db->entries[i].type == 3) {
        g_currentLevel = LevelNumber(i);
        ShowLevelDetails(hDlg, i, g_db->entries[i].file, g_db->entries[i].bitmap);
        g_dbIndex = i;
        strcpy(g_selNameInfoOwn, g_db->entries[i].name);
        strcpy(g_selPath, FindRating(i)->file);
        strcat(g_selPath, "\\");
        strcat(g_selPath, g_db->entries[i].file);
        EnableWindow(ok, TRUE);
        return;
    }
    if (g_newPreview) {
        DeleteObject(g_newPreview);
        DeleteObject(g_previewPalNewUsed);
        g_newPreview = 0;
        g_previewPalNewUsed = 0;
    }
    item = GetDlgItem(hDlg, 0x14a);
    InvalidateRect(item, 0, FALSE);
    EnableWindow(ok, FALSE);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_levelindex0, pad_levelindex1, pad_levelindex2;

/* 0x40b390: the database index of level n */
int LevelIndex(int n)
{
    int count = 0;
    int i;
    int id;

    for (i = 0; i < g_db->count; i++) {
        id = g_db->entries[i].id;
        if (g_db->entries[i].type == 3) {
            count++;
            if (count == n)
                return id;
        }
    }
    return 0;
}

/* 0x40b3e0: open or close a rating or group, or choose a level */
void OpenLevelItem(HWND hDlg, HWND list, int i, WORD sel)
{
    int show;
    int prev;
    int t;
    int id = g_db->entries[i].id;
    int type = g_db->entries[i].type;
    int childType = g_db->entries[i].childType;
    int j;
    int status;
    int kind;
    WORD children = g_db->entries[i].children;
    int shown = g_db->entries[i].shown;

    if (type == 3) {
        g_currentLevel = LevelNumber(id);
        CloseLevelDialog(hDlg, 1);
        return;
    }
    if (type == 0x101)
        kind = 0x101;
    else if (type == 0x102)
        kind = 0x102;
    else
        return;
    if (TreeIsOpen(&g_treeNext, i)) {
        TreeClose(&g_treeNext, i);
        sel++;
        j = SendMessage(list, LB_GETITEMDATA, sel, 0);
        t = g_db->entries[j].type;
        while (shown-- > 0 && t != kind) {
            t = g_db->entries[j].type;
            if ((t & 0x100) && TreeIsOpen(&g_treeNext, j)) {
                TreeClose(&g_treeNext, j);
                shown += g_db->entries[j].shown;
            }
            SendMessage(list, LB_DELETESTRING, sel, 0);
            if (shown <= 0 || kind == t)
                break;
            j = SendMessage(list, LB_GETITEMDATA, sel, 0);
            t = g_db->entries[j].type;
        }
        return;
    }
    prev = 1;
    TreeOpen(&g_treeNext, i);
    if (type == 0x101 || type == 0x102) {
        SendMessage(list, WM_SETREDRAW, FALSE, 0);
        WORD pos = sel;
        WORD *posPtr = &pos;
        LevelIndex(g_currentLevel);
        int added = 0;
        int *countPtr = &added;
        j = id;
        while (children) {
            j++;
            t = g_db->entries[j].type;
            status = g_db->entries[j].status;
            if (childType == t) {
                children--;
                ++*posPtr;
                show = 1;
                if (t == 3)
                    show = prev || status;
                prev = status;
                if (show) {
                    SendMessage(list, LB_INSERTSTRING, pos, j);
                    ++*countPtr;
                }
            }
        }
        short value = *(WORD *)countPtr;
        g_db->entries[i].shown = value;
        /* Keep the count live through this point for VC++ 2.2 register allocation. */
        switch (value) { case 1: break; }
        TreeEnsureVisible(&g_treeNext, list, sel, 9);
        SendMessage(list, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(list, 0, TRUE);
    }
}

/* 0x40b6f0 */
void DrawBlank(HDC hdc, RECT *r, int index)
{
    HGDIOBJ old = SelectObject(hdc, GetStockObject(DKGRAY_BRUSH));

    Rectangle(hdc, r->left, r->top, r->right, r->bottom);
    SelectObject(hdc, old);
}

/* 0x40b730: one of the pictures in gfx\dialog.bmp */
void DrawDialogIcon(HDC hdc, RECT *r, int index)
{
    int x;
    int y;
    HDC mem;
    HPALETTE oldMemPal;
    HBITMAP oldBitmap;
    HPALETTE oldPal;

    if (index > 8)
        x = (index - 9) * 32;
    else
        x = (index - 1) * 32;
    y = index > 8 ? 41 : 0;
    mem = CreateCompatibleDC(hdc);
    oldMemPal = SelectPalette(mem, g_hereDlgPalette, FALSE);
    RealizePalette(mem);
    oldBitmap = (HBITMAP)SelectObject(mem, g_dlgBitmapAll);
    oldPal = SelectPalette(hdc, g_hereDlgPalette, TRUE);
    RealizePalette(hdc);
    StretchBlt(hdc, r->left, r->top, r->right, r->bottom, mem, x, y, 32, 40, SRCCOPY);
    SelectObject(mem, oldBitmap);
    SelectPalette(hdc, oldPal, FALSE);
    SelectPalette(mem, oldMemPal, FALSE);
    DeleteDC(mem);
}

/* 0x40b810 */
void GetBitmapSize(HBITMAP bm, int *w, int *h)
{
    BITMAP b;

    GetObject(bm, sizeof(b), &b);
    *w = b.bmWidth;
    *h = b.bmHeight;
}

/* 0x40b840: the level's picture */
void DrawPreview(HDC hdc, RECT *r, int index)
{
    char *none = "No Level Selected";
    int w;
    int h;
    HDC mem;
    HPALETTE oldMemPal;
    HBITMAP oldBitmap;
    HPALETTE oldPal;
    HGDIOBJ old;
    COLORREF oldColour;
    int len;
    int x;

    if (g_newPreview) {
        char text[128];

        GetBitmapSize(g_newPreview, &w, &h);
        mem = CreateCompatibleDC(hdc);
        oldMemPal = SelectPalette(mem, g_previewPalNewUsed, FALSE);
        oldBitmap = (HBITMAP)SelectObject(mem, g_newPreview);
        oldPal = SelectPalette(hdc, g_previewPalNewUsed, TRUE);
        RealizePalette(hdc);
        if (!StretchBlt(hdc, r->left, r->top, r->right, r->bottom, mem, 0, 0, w, h, SRCCOPY)) {
            wsprintf(text, "StretchBlt failed = %d\n", GetLastError());
            MessageBox(0, text, "DEBUG", 0);
        }
        SelectObject(mem, oldBitmap);
        SelectPalette(hdc, oldPal, TRUE);
        SelectPalette(mem, oldMemPal, FALSE);
        DeleteDC(mem);
#ifdef _MSC_VER
        switch ((int)r) { case 1: break; }
        switch ((int)SelectPalette) { case 1: break; }
#endif
    } else {
        SIZE size;

        old = SelectObject(hdc, GetStockObject(DKGRAY_BRUSH));
        Rectangle(hdc, r->left, r->top, r->right, r->bottom);
        SelectObject(hdc, old);
        oldColour = SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
        len = strlen(none);
        GetTextExtentPoint(hdc, none, len, &size);
        x = (r->right - r->left) / 2 - size.cx / 2;
        ExtTextOut(hdc, x, 0, ETO_OPAQUE, r, none, len, 0);
        SetBkColor(hdc, oldColour);
    }
}

/* 0x40ba10 */
LRESULT CALLBACK IconImageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    RECT r;
    HDC hdc;
    int index;

    switch (msg) {
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &r);
        index = GetWindowWord(hwnd, 0);
        g_drawFuncs[index](hdc, &r, index);
        EndPaint(hwnd, &ps);
        return 1;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_drawicontext0, pad_drawicontext1, pad_drawicontext2, pad_drawicontext3, pad_drawicontext4, pad_drawicontext5, pad_drawicontext6, pad_drawicontext7, pad_drawicontext8, pad_drawicontext9, pad_drawicontext10, pad_drawicontext11, pad_drawicontext12, pad_drawicontext13, pad_drawicontext14, pad_drawicontext15, pad_drawicontext16, pad_drawicontext17, pad_drawicontext18, pad_drawicontext19, pad_drawicontext20, pad_drawicontext21, pad_drawicontext22, pad_drawicontext23, pad_drawicontext24, pad_drawicontext25, pad_drawicontext26, pad_drawicontext27, pad_drawicontext28;

/* 0x40ba90 */
void DrawIconText(HWND hwnd, HDC hdc, RECT *r)
{
    char text[44];
    SIZE size;
    int len = GetWindowText(hwnd, text, 40);
    COLORREF old = SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
    int x;

    GetTextExtentPoint(hdc, text, len, &size);
    x = (r->right - r->left) / 2 - size.cx / 2;
    ExtTextOut(hdc, x, 0, ETO_OPAQUE, r, text, len, 0);
    SetBkColor(hdc, old);
}

/* 0x40bb20 */
LRESULT CALLBACK IconTextProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    RECT r;
    HDC hdc;

    switch (msg) {
    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &r);
        DrawIconText(hwnd, hdc, &r);
        EndPaint(hwnd, &ps);
        return 1;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* 0x40bba0: the dialog's palette with the level picture's colours at the top */
void SetPreviewPalette(HPALETTE pal, int n)
{
    if (g_hereDlgPalette)
        DeleteObject(g_hereDlgPalette);
    GetPaletteEntries(pal, 0, n, &g_dlgLogPal.palPalEntry[128]);
    g_hereDlgPalette = CreatePalette((LOGPALETTE *)&g_dlgLogPal);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_showleveldetails0, pad_showleveldetails1, pad_showleveldetails2, pad_showleveldetails3, pad_showleveldetails4, pad_showleveldetails5, pad_showleveldetails6, pad_showleveldetails7;

/* Two independent constant loads still differ in scheduling (see HANDOFF session39). */
/* 0x40bbe0 */
void ShowLevelDetails(HWND hDlg, int i, char *file, char *bitmap)
{
    char num[20];
    char path[128];
    char picture[128];
    DBENTRY *rating = FindRating(i);
    DATABASE *db;
    HANDLE f;
    int id;
    HWND item;
    int j;
    WORD *p;
    int *s;
    WORD raw;
    int hi;
    int lo;

    strcpy(path, "level\\");
    strcat(path, rating->file);
    strcat(path, "\\");
    strcpy(picture, "level\\");
    strcat(picture, rating->file);
    strcat(picture, "\\");
    strcat(path, file);
    strcat(picture, bitmap);
    UnrealizeObject(g_hereDlgPalette);
    if (g_newPreview) {
        DeleteObject(g_newPreview);
        DeleteObject(g_previewPalNewUsed);
        g_newPreview = 0;
        g_previewPalNewUsed = 0;
    }
    g_newPreview = LoadDIBitMap(picture, &g_previewPalNewUsed);
    f = FILE_ReadOpen(path);
    FILE_Read(f, g_lvlBuf, 0x800);
    FILE_Close(f);
    db = g_db;
    g_lastLsCustom = g_db->entries[i].custom;
    if (g_lastLsCustom) {
        g_stateLsRate = g_db->entries[i].rate;
        g_lsLemmingsSpareMain = g_db->entries[i].lemmings;
        g_lsRescueState = g_db->entries[i].rescue;
        g_spareLsTime = g_db->entries[i].time;
        for (j = 0; j < 8; j++)
            g_infoLsSkills[j] = g_db->entries[i].skills[j];
    } else {
        g_lsLemmingsSpareMain = (g_lvlBuf[2] << 8) + g_lvlBuf[3];
        g_lsRescueState = (g_lvlBuf[4] << 8) + g_lvlBuf[5];
        g_spareLsTime = g_lvlBuf[7];
        p = (WORD *)(g_lvlBuf + 8);
        s = g_infoLsSkills;
        do {
            raw = *p;
            hi = HIBYTE(raw);
            lo = LOBYTE(raw);
            *s++ = hi + (lo << 8);
            p++;
        } while (s < g_infoLsSkills + 8);
    }
    strncpy(g_lsName, db->entries[i].name, 49);
    item = GetDlgItem(hDlg, 0x14a);
    InvalidateRect(item, 0, FALSE);
    item = GetDlgItem(hDlg, 0x153);
    itoa(g_lsLemmingsSpareMain, num, 10);
    SetWindowText(item, num);
    InvalidateRect(item, 0, FALSE);
    item = GetDlgItem(hDlg, 0x154);
    itoa(g_lsRescueState, num, 10);
    SetWindowText(item, num);
    InvalidateRect(item, 0, FALSE);
    item = GetDlgItem(hDlg, 0x155);
    wsprintf(num, "%c:00", g_spareLsTime + '0');
    SetWindowText(item, num);
    InvalidateRect(item, 0, FALSE);
    for (id = 0x156; id <= 0x15d; id++) {
#ifdef _MSC_VER
        switch ((int)item) { case 1: break; }
#endif
        item = GetDlgItem(hDlg, id);
        itoa(g_infoLsSkills[id - 0x156], num, 10);
        SetWindowText(item, num);
        InvalidateRect(item, 0, FALSE);
    }
}

/* 0x40bfd0 */
void LevelSelectNop(void)
{
}

/* 0x40bfe0 */
void ClampLevel(void)
{
    if (g_currentLevel < 1)
        g_currentLevel = 1;
}

/* 0x40c000 */
int IsLastLevel(void)
{
    int index;
    int last;

    LoadDatabase();
    ClampLevel();
    index = LevelIndex(g_currentLevel);
    last = g_db->count - index - 1 == 0;
    SaveDatabase();
    return last;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_nextlevelnumber0, pad_nextlevelnumber1, pad_nextlevelnumber2, pad_nextlevelnumber3, pad_nextlevelnumber4, pad_nextlevelnumber5, pad_nextlevelnumber6, pad_nextlevelnumber7, pad_nextlevelnumber8, pad_nextlevelnumber9;

/* 0x40c040: the first level after n not yet done, or 1 */
int NextLevelNumber(int n)
{
    int first = n + 1;
    int i = first;
    int index;

    for (;;) {
        index = LevelIndex(i);
        if (!index)
            return n - i != -1 ? first : 1;
        if (!g_db->entries[index].status)
            return i;
        i++;
    }
}

/* 0x40c0a0: mark this level done and move to the next */
void NextLevel(void)
{
    LoadDatabase();
    ClampLevel();
    g_savedDbIndexNowTemp = g_dbIndex;
    g_savedLevelNowCur = g_currentLevel;
    if (g_quitToMenu && g_db->entries[g_dbIndex].status != 1)
        g_db->entries[g_dbIndex].status = 2;
    else
        g_db->entries[g_dbIndex].status = 1;
    g_currentLevel = NextLevelNumber(g_currentLevel);
    g_replaying = 0;
    g_dbIndex = LevelIndex(g_currentLevel);
    LevelSelectNop();
    SaveDatabase();
}

/* 0x40c140 */
void UndoNextLevel(void)
{
    g_dbIndex = g_savedDbIndexNowTemp;
    g_currentLevel = g_savedLevelNowCur;
}

/* 0x40c160 */
void SetLevel(int i)
{
    DBENTRY *rating = FindRating(i);
    int j;

    strcpy(g_selNameInfoOwn, g_db->entries[i].name);
    strcpy(g_selPath, "level\\");
    strcat(g_selPath, rating->file);
    strcat(g_selPath, "\\");
    strcat(g_selPath, g_db->entries[i].file);
    strcpy(g_levelFile, g_selPath);
    g_lastLsCustom = g_db->entries[i].custom;
    if (g_lastLsCustom) {
        g_stateLsRate = g_db->entries[i].rate;
        g_lsLemmingsSpareMain = g_db->entries[i].lemmings;
        g_lsRescueState = g_db->entries[i].rescue;
        g_spareLsTime = g_db->entries[i].time;
        for (j = 0; j < 8; j++)
            g_infoLsSkills[j] = g_db->entries[i].skills[j];
    }
    strncpy(g_lsName, g_db->entries[i].name, 49);
}

/* 0x40c300 */
void PlayLevel(HWND hwnd)
{
    ClampLevel();
    LoadDatabase();
    if (g_dbIndex < 0)
        g_dbIndex = LevelIndex(g_currentLevel);
    SetLevel(g_dbIndex);
    PostMessage(hwnd, WM_COMMAND, 0x10f, 0);
    SaveDatabase();
}
