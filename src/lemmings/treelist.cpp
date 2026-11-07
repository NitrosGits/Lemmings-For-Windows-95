/*
 * An owner-drawn list box shown as a tree: a bitmap of small pictures in
 * rows and columns, lines joining the items, and the list of open items.
 */
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "winlemm.h"

/* 0x40c350 */
void TreeFreeBitmaps(TREE *t)
{
    if (t->bitmap) {
        if (t->oldBitmap)
            SelectObject(t->hdc, t->oldBitmap);
        t->oldBitmap = 0;
        DeleteObject(t->bitmap);
        t->bitmap = 0;
    }
    if (t->hdc) {
        DeleteDC(t->hdc);
        t->hdc = 0;
    }
}

/* 0x40c3a0 */
void TreeFreeOpen(TREE *t)
{
    t->count = 0;
    if (t->open)
        free(t->open);
    t->open = 0;
}

/* 0x40c3d0 */
void TreeMeasureItem(HWND hDlg, MEASUREITEMSTRUCT *mis, TREE *t)
{
    mis->itemHeight = t->textHeight > t->cellHeight ? t->textHeight : t->cellHeight;
}

/* 0x40c3f0 */
void TreeSetFont(HWND list, HFONT font, TREE *t)
{
    TEXTMETRIC tm;
    HDC hdc = GetDC(list);
    HGDIOBJ old = SelectObject(hdc, font);

    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, old);
    ReleaseDC(list, hdc);
    t->textHeight = tm.tmHeight;
    t->lineHeight = t->cellHeight > tm.tmHeight ? t->cellHeight : tm.tmHeight;
    if (list)
        SendMessage(list, LB_SETITEMHEIGHT, 0, MAKELPARAM(t->lineHeight, 0));
}

/* 0x40c470 */
COLORREF SwapRGB(COLORREF c)
{
    return RGB(GetBValue(c), GetGValue(c), GetRValue(c));
}

/* 0x40c4a0 */
int TreeLoadBitmaps(HINSTANCE hInst, int id, int rows, int cols, int lines, TREE *t, int reset)
{
    HDC hdc;
    HRSRC res;
    HGLOBAL mem;
    BITMAPINFOHEADER *bi;
    BYTE *bits;
    char back;
    RGBQUAD colour;
    struct {
        BITMAPINFOHEADER bmiHeader;
        COLORREF colours[16];
    } bmi;

    if (reset) {
        t->count = 0;
        t->open = 0;
        t->lines = lines;
    }
    if (!t->hdc) {
        hdc = GetDC(0);
        t->hdc = CreateCompatibleDC(hdc);
        ReleaseDC(0, hdc);
        if (!t->hdc)
            return 0;
        t->oldBitmap = 0;
    }
    res = FindResource(hInst, MAKEINTRESOURCE(id), RT_BITMAP);
    if (!res)
        return 0;
    mem = LoadResource(hInst, res);
    if (!mem)
        return 0;
    bi = (BITMAPINFOHEADER *)LockResource(mem);
    if (!bi)
        return 0;
    bits = (BYTE *)bi + sizeof(bmi);
    back = *bits;
    back = (back >> 4) & 0xf;
    memcpy(&bmi, bi, sizeof(bmi));
    *(COLORREF *)&colour = SwapRGB(GetSysColor(COLOR_WINDOW));
    memcpy(&bmi.colours[back], &colour, sizeof(colour));
    hdc = GetDC(0);
    t->bitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, bits, (BITMAPINFO *)&bmi, DIB_RGB_COLORS);
    ReleaseDC(0, hdc);
    t->cellHeight = (WORD)bi->biHeight / rows;
    t->cellWidth = (WORD)bi->biWidth / cols;
    t->lineHeight = t->textHeight > t->cellHeight ? t->textHeight : t->cellHeight;
    FreeResource(mem);
    if (!t->bitmap)
        return 0;
    t->oldBitmap = (HBITMAP)SelectObject(t->hdc, t->bitmap);
    return t->oldBitmap != 0;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_treedrawitem0, pad_treedrawitem1, pad_treedrawitem2, pad_treedrawitem3, pad_treedrawitem4, pad_treedrawitem5, pad_treedrawitem6, pad_treedrawitem7, pad_treedrawitem8, pad_treedrawitem9, pad_treedrawitem10, pad_treedrawitem11, pad_treedrawitem12, pad_treedrawitem13, pad_treedrawitem14, pad_treedrawitem15, pad_treedrawitem16, pad_treedrawitem17, pad_treedrawitem18, pad_treedrawitem19, pad_treedrawitem20, pad_treedrawitem21;

/* 0x40c640 */
void TreeDrawItem(HWND hDlg, DRAWITEMSTRUCT *dis, int level, int lines, char *text, int row, int col, TREE *t)
{
    HDC hdc;
    RECT r;
    int x;
    WORD mid;
    WORD textY;
    WORD bitmapY;
    int bit;
    int lx;
    int n;
    int bottom;

    if (dis->itemID == -1)
        return;
    hdc = dis->hDC;
    CopyRect(&r, &dis->rcItem);
    x = (WORD)(t->cellWidth * (WORD)level + (WORD)r.left + 2);
    r.left = x + t->cellWidth;
    mid = (r.bottom - r.top) / 2;
    textY = mid - t->textHeight / 2 + (WORD)r.top;
    bitmapY = mid - t->cellHeight / 2 + (WORD)r.top;
    if (dis->itemAction == ODA_FOCUS)
        goto focus;
    if (dis->itemAction == ODA_SELECT)
        goto select;
    if (t->lines && level) {
        bit = 1;
        SetBkColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        lx = t->cellWidth / 2 + 2;
        for (n = 0; n < level; n++) {
            if (lines & bit)
                FastRect(hdc, lx, r.top, 1, r.bottom - r.top);
            lx += t->cellWidth;
            bit += bit;
        }
        bit += bit;
        lx = (level - 1) * t->cellWidth + t->cellWidth / 2 + 2;
        bottom = r.bottom;
        if (!(lines & bit))
            bottom -= t->lineHeight / 2;
        FastRect(hdc, lx, r.top, 1, bottom - r.top);
        FastRect(hdc, lx, r.bottom - t->lineHeight / 2, t->cellWidth / 2, 1);
    }
    BitBlt(hdc, x, bitmapY, t->cellWidth, t->cellHeight, t->hdc, col * t->cellWidth, row * t->cellHeight, SRCCOPY);
select:
    if (dis->itemState & ODS_SELECTED) {
        SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
        SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    } else {
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    }
    ExtTextOut(hdc, r.left + 1, textY, ETO_CLIPPED | ETO_OPAQUE, &r, text, lstrlen(text), 0);
    if (!(dis->itemState & ODS_FOCUS) || dis->itemAction == ODA_SELECT)
        return;
focus:
    DrawFocusRect(hdc, &r);
}

/* 0x40c890: a filled rectangle in the background colour */
void FastRect(HDC hdc, int x, int y, int w, int h)
{
    RECT r;

    r.left = x;
    r.right = x + w;
    r.top = y;
    r.bottom = y + h;
    ExtTextOut(hdc, x, y, ETO_OPAQUE, &r, 0, 0, 0);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_treeisopen0, pad_treeisopen1, pad_treeisopen2, pad_treeisopen3, pad_treeisopen4, pad_treeisopen5, pad_treeisopen6, pad_treeisopen7, pad_treeisopen8, pad_treeisopen9, pad_treeisopen10, pad_treeisopen11, pad_treeisopen12, pad_treeisopen13, pad_treeisopen14, pad_treeisopen15, pad_treeisopen16, pad_treeisopen17, pad_treeisopen18, pad_treeisopen19;

/* 0x40c8e0 */
int TreeIsOpen(TREE *t, int id)
{
    int i;

    for (i = 0; i < t->count; i++) {
        if (t->open[i] == id)
            return 1;
    }
    return 0;
}

/* 0x40c910 */
void TreeOpen(TREE *t, int id)
{
    t->count++;
    if (!t->open)
        t->open = (int *)malloc(t->count * sizeof(int));
    else
        t->open = (int *)realloc(t->open, t->count * sizeof(int));
    t->open[t->count - 1] = id;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_treeclose0;

/* 0x40c960 */
void TreeClose(TREE *t, int id)
{
    int i;

    for (i = 0; i < t->count; i++) {
        if (t->open[i] == id) {
            if (--t->count == 0) {
                free(t->open);
                t->open = 0;
            } else {
                if (i < t->count)
                    memmove(&t->open[i], &t->open[i + 1], (t->count - i) * sizeof(int));
                t->open = (int *)realloc(t->open, t->count * sizeof(int));
            }
        }
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_treeensurevisible0;

/* 0x40c9d0: scroll so that n items after sel can be seen */
void TreeEnsureVisible(TREE *t, HWND list, WORD sel, WORD n)
{
    RECT r;
    WORD top = (WORD)SendMessage(list, LB_GETTOPINDEX, 0, 0);
    int first;

    GetClientRect(list, &r);
    if (n >= (WORD)((r.bottom + 1) / t->lineHeight - sel + top)) {
        first = top - (WORD)((r.bottom + 1) / t->lineHeight - sel + top) + (n + 1);
        first = first < sel ? first : sel;
        SendMessage(list, LB_SETTOPINDEX, (WORD)first, 0);
    }
}
