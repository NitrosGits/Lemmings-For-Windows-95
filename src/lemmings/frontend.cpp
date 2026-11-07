/*
 * The title screen: the logo tiled over the window, two buttons, two
 * lemmings slapping the credits round, letter by letter.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12, pad_top13, pad_top14;

#include <windows.h>
#include <string.h>
#include "winlemm.h"

BYTE *g_feSprites = 0;
BYTE *g_feFont = 0;
BYTE *g_slapL = 0;
BYTE *g_slapR = 0;
BYTE *g_textBuf = 0;            /* an SRLE file of 20 24x24 frames: the credits line */
int g_feCaptured = 0;
int g_feButton1 = 0;
int g_feButton2 = 0;
int g_slapLFrame = 0;
int g_slapRFrame = 0;
int g_feCentre = 0;
int g_unused71c = 0;
char g_feMenu[] = "FrontEnd";
int g_unused72c = 1;
int g_unused730 = 1;
int g_unused734 = 1;
int g_unused738 = 1;
int g_unused73c = 0;
HPALETTE g_fePalette = 0;
int g_unused744 = 0;
int g_unused748[6] = {0};
PANE g_fePanes[2] = {
    {-1, 4, g_statusText},
    {0},
};
int g_textPos[20] = {0};        /* how far each letter has flipped, 0-32 */
int g_textPrev[20] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

WORD g_feRenderer;
int g_unused800;              /* unused: fills a gap in the original .bss */
WORD g_feSlot;
STATBAR g_barFe;
DISPLAYLIST *g_feListAllState;
DISPLAYLIST *g_feList2Extra;

/* 0x406940 */
void InitFrontEnd(void)
{
    char title[256];
    int i;

    LoadGameMenu(g_feMenu);
    SetMainProc(FrontEndProc);
    ShowStatBar(&g_barFe, g_fePanes);
    for (i = 0; i < 20; i++)
        g_textPrev[i] = 1;
    for (i = 0; i < 20; i++)
        g_textPos[i] = 0;
    wsprintf(title, GetString(0x110), g_userName);
    SetWindowText(g_hwnd, title);
}

/* 0x4069d0: 1 or 2 if a point is on a button (POINT by value: FrontEndProc's store/reload of pt.x) */
int HitButton(POINT pt)
{
    static RECT s_buttons[2] = {
        {g_feCentre - 128, 186, 113, 99},
        {g_feCentre + 12, 186, 122, 99},
    };
    int i;

    for (i = 0; i < 2; i++) {
        RECT *r = &s_buttons[i];

        if (pt.x >= r->left && pt.x < r->left + r->right && pt.y >= r->top && pt.y < r->top + r->bottom)
            return i + 1;
    }
    return 0;
}

int g_unused860[6] = {0x19, 0x20, 0x21, 0x22, 0x23, 0x24};

/* 0x406a80 */
LRESULT CALLBACK FrontEndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;

    switch (msg) {
    case WM_SIZE:
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;

            hdc = BeginPaint(hwnd, &ps);
            DrawFrontEnd(hdc, &ps);
            EndPaint(hwnd, &ps);
        }
        return 0;
    case WM_COMMAND:
        FrontEndCommand(hwnd, msg, wParam, lParam);
        return 0;
    case WM_LBUTTONDOWN:
        {
            POINT pt;

            pt.x = LOWORD(lParam) - g_clientWidth / 2 + g_feCentre;
            pt.y = HIWORD(lParam);
            switch (HitButton(pt)) {
            case 1:
                g_feButton1 = 1;
                SetCapture(hwnd);
                g_feCaptured = 1;
                return 0;
            case 2:
                g_feButton2 = 1;
                SetCapture(hwnd);
                g_feCaptured = 1;
                return 0;
            }
        }
        return 0;
    case WM_LBUTTONUP:
        {
            POINT pt;

            pt.x = LOWORD(lParam) - g_clientWidth / 2 + g_feCentre;
            pt.y = HIWORD(lParam);
            switch (HitButton(pt)) {
            case 0:
                g_feButton2 = g_feButton1 = 0;
                break;
            case 1:
                PostMessage(hwnd, WM_COMMAND, 0x117, 0);
                g_feButton1 = 0;
                break;
            case 2:
                PostMessage(hwnd, WM_COMMAND, 0x113, 0);
                g_feButton2 = 0;
                break;
            }
            if (g_feCaptured) {
                ReleaseCapture();
                g_feCaptured = 0;
            }
        }
        break;
    case 0x500:
        AnimateTitle(0);
        hdc = GetDC(hwnd);
        DrawFrontEnd(hdc, 0);
        ReleaseDC(hwnd, hdc);
        return 0;
    case 0x603:
        PostMessage(hwnd, WM_COMMAND, 0x10f, 0);
        return 0;
    case 0x700:
        if (g_feCaptured) {
            ReleaseCapture();
            g_feCaptured = 0;
        }
        QuitFrontEnd(hwnd);
        return 0;
    case 0x701:
        g_feCaptured = g_feButton2 = g_feButton1 = 0;
        StartFrontEnd(hwnd);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* 0x406d60 */
void DrawFrontEnd(HDC hdc, PAINTSTRUCT *ps)
{
    HPALETTE old = SelectPalette(hdc, g_hPalette, FALSE);
    DISPLAYLIST *t;
    int x;

    RealizePalette(hdc);
    DrawTextBuffer();
    AddSprite(g_feCentre - 290, 320, g_slapL, g_slapLFrame >> 1, 1);
    AddSprite(g_feCentre + 225, 320, g_slapR, g_slapRFrame >> 1, 1);
    if (g_feButton1)
        AddSprite(g_feCentre - 129, 186, g_feSprites, 4, 1);
    if (g_feButton2)
        AddSprite(g_feCentre + 11, 186, g_feSprites, 5, 1);
    Renderer_RenderList(g_feRenderer, g_feListAllState);
    g_feListAllState->count = 0;
    t = g_feListAllState;
    g_feListAllState = g_feList2Extra;
    g_feList2Extra = t;
    x = g_feCentre - g_clientWidth / 2;
    if (x < 0)
        x = 0;
    if (ps) {
        if (ps->rcPaint.bottom > g_valClientHeight)
            ps->rcPaint.bottom = g_valClientHeight;
        Renderer_Revealed(g_feRenderer, g_feSlot, ps->rcPaint.left + x, ps->rcPaint.top, ps->rcPaint.right + x,
                          ps->rcPaint.bottom);
    }
    Renderer_Render(g_feRenderer, hdc, g_feSlot, x, 0, x + g_clientWidth, g_valClientHeight, g_clientWidth,
                    g_valClientHeight, 1);
    SelectPalette(hdc, old, FALSE);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_frontendcommand0, pad_frontendcommand1, pad_frontendcommand2, pad_frontendcommand3, pad_frontendcommand4;

/* 0x406f00 */
LRESULT FrontEndCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char title[256];
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
    case 0x4d2:
        if (!code)
            PlayLevel(hwnd);
        break;
    case 0x929:
        if (!code)
            PostMessage(hwnd, WM_COMMAND, 0x113, 0);
        break;
    case 0x9c47:
        SelectUser();
        wsprintf(title, GetString(0x110), g_userName);
        SetWindowText(hwnd, title);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* 0x4071b0: a blank DIB the size of the screen, with the title palette */
PDIB CreateFrontDib(BYTE *pal)
{
    BITMAPINFOHEADER *bi;
    DWORD w;
    HDC hdc;
    RGBQUAD *c;
    int size;
    int h;
    BYTE *bits;
    int i;

    hdc = CreateDC("DISPLAY", 0, 0, 0);
    w = (GetDeviceCaps(hdc, HORZRES) + 15) & ~15;
    DeleteDC(hdc);
    h = (g_valClientHeight + 7) & ~7;
    size = h * w;
    bi = (BITMAPINFOHEADER *)VSMEM_Allocate(size + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD), "FEDib");
    c = (RGBQUAD *)(bi + 1);
    bits = (BYTE *)bi + sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);
    bi->biSize = sizeof(BITMAPINFOHEADER);
    bi->biWidth = w;
    bi->biHeight = h;
    bi->biPlanes = 1;
    bi->biBitCount = 8;
    bi->biCompression = 0;
    bi->biSizeImage = 0;
    bi->biXPelsPerMeter = 0;
    bi->biYPelsPerMeter = 0;
    bi->biClrUsed = 0;
    bi->biClrImportant = 0;
    if (!pal) {
        for (i = 0; i < 256; i++) {
            c[i].rgbBlue = i;
            c[i].rgbGreen = i;
            c[i].rgbRed = i;
            c[i].rgbReserved = 0;
        }
    } else {
        for (i = 0; i < ((LOGPALETTE *)(pal + 2))->palNumEntries; i++) {
            c[i].rgbBlue = ((LOGPALETTE *)(pal + 2))->palPalEntry[i].peBlue;
            c[i].rgbGreen = ((LOGPALETTE *)(pal + 2))->palPalEntry[i].peGreen;
            c[i].rgbRed = ((LOGPALETTE *)(pal + 2))->palPalEntry[i].peRed;
            c[i].rgbReserved = 0;
        }
    }
    memset(bits, 0x40, size);
    return (PDIB)bi;
}

/* 0x4072b0 */
WORD LoadFrontCanvas(WORD renderer, PDIB dib)
{
    return Renderer_LoadCanvas_BitMap(renderer, dib, 2, 0);
}

/* 0x4072d0 */
void SetFrontPalette(PDIB dib)
{
    g_fePalette = GetPalette(dib);
    SetGamePalette(g_fePalette);
    SendMessage(g_hwnd, 0x702, 0, 0);
    SetWindowPos(g_hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
}

/* 0x407320 */
BYTE *ReadWholeFile(char *name)
{
    HANDLE f = FILE_ReadOpen(name);
    DWORD size = FILE_Length(f);
    BYTE *data = (BYTE *)VSMEM_Allocate(size, "frontend sprite file");

    FILE_Read(f, data, size);
    FILE_Close(f);
    return data;
}

/* 0x407360 */
BYTE *LoadPalFile(char *name)
{
    return ReadWholeFile(name);
}

/* 0x407370 */
BYTE *LoadSpriteFile(char *name)
{
    return ReadWholeFile(name);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_addsprite0, pad_addsprite1, pad_addsprite2, pad_addsprite3, pad_addsprite4, pad_addsprite5, pad_addsprite6, pad_addsprite7, pad_addsprite8, pad_addsprite9, pad_addsprite10, pad_addsprite11;

/* 0x407380 */
void AddSprite(int x, int y, BYTE *sprites, int frame, int mode)
{
    int n = g_feListAllState->count++;
    RLEDATA *d;
    DLENTRY *e = &g_feListAllState->entries[n];

    e->type = DL_RLE;
    d = &e->u.rle;
    d->mode = mode;
    d->frame = frame;
    d->sprites = sprites;
    d->x = x;
    d->y = y;
}

/* 0x4073e0: a character's frame in the font */
int CharIndex(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= '1' && c <= '9')
        return c - '1' + 26;
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 48;
    switch (c) {
    case '\t':
    case '\n':
    case '\r':
    case ' ':
        return 36;
    case '!':
        return 40;
    case '(':
        return 41;
    case ')':
        return 42;
    case '-':
        return 38;
    case '0':
        return 35;
    case '?':
        return 39;
    }
    return -1;
}

/* 0x4074c0 */
void DrawFrontText(int x, int y, char *text, int mode)
{
    int x0 = x;
    char *p = text;

    for (; *p; p++) {
        switch (*p) {
        default:
            AddSprite(x, y, g_feFont, CharIndex(*p), mode);
            x += 24;
            break;
        case '\n':
            x = x0;
            y += 24;
            break;
        }
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_inittextbuffer0, pad_inittextbuffer1, pad_inittextbuffer2, pad_inittextbuffer3, pad_inittextbuffer4, pad_inittextbuffer5, pad_inittextbuffer6;

/* 0x407530 */
void InitTextBuffer(void)
{
    int i;
    BYTE *buffer;
    DWORD offset;

    buffer = (BYTE *)VSMEM_Allocate(0x321c, "Text buffer ");
    g_textBuf = buffer;
    *(DWORD *)g_textBuf = 'ELRS';
    *(WORD *)(g_textBuf + 4) = 20;
    offset = 0x58;
    for (i = 0; i < 20; i++) {
        int j;
        BYTE *p;

        *(DWORD *)(g_textBuf + 6 + i * 4) = offset;
        p = g_textBuf + *(DWORD *)(g_textBuf + 6 + i * 4);
        ((WORD *)p)[0] = 0;
        ((WORD *)p)[1] = 0;
        ((WORD *)p)[2] = 24;
        ((WORD *)p)[3] = 24;
        ((WORD *)p)[4] = 24;
        ((WORD *)p)[5] = 24;
        p += 12;
        for (j = 24; j; j--) {
            memset(p, 0, 26);
            *p = 0x98;
            p += 26;
            p[-1] = 0x80;
        }
        offset += 0x27d;
    }
}

/* 0x4075d0 */
void DrawTextBuffer(void)
{
    int i;

    for (i = 0; i < 20; i++)
        AddSprite(g_feCentre + i * 24 - 240, 330, g_textBuf, i, 3);
}

/* 0x407620: one row of an SRLE sprite */
void UnpackRow(BYTE *dst, BYTE *src)
{
    int done = 0;

    do {
        BYTE c = *src++;

        if (c & 0x80) {
            if (c == 0x80)
                done = 1;
            else {
                c &= 0x7f;
                while (c > 0) {
                    *dst++ = *src++;
                    c--;
                }
            }
        } else
            dst += c;
    } while (!done);
}

DWORD g_flipMasks[9][2] = {
    {0xffffff, 0x000000}, {0xf7efef, 0x801001}, {0xe7e7e7, 0x111111},
    {0x676767, 0x515151}, {0x666666, 0x555555}, {0x515151, 0x676767},
    {0x111111, 0xe7e7e7}, {0x801001, 0xf7efef}, {0x000000, 0xffffff},
};

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_flipchar0, pad_flipchar1, pad_flipchar2, pad_flipchar3, pad_flipchar4, pad_flipchar5, pad_flipchar6, pad_flipchar7, pad_flipchar8, pad_flipchar9, pad_flipchar10, pad_flipchar11, pad_flipchar12, pad_flipchar13, pad_flipchar14, pad_flipchar15, pad_flipchar16, pad_flipchar17, pad_flipchar18, pad_flipchar19, pad_flipchar20, pad_flipchar21, pad_flipchar22, pad_flipchar23, pad_flipchar24, pad_flipchar25, pad_flipchar26, pad_flipchar27;

/* 0x407670: a letter part way through turning into another */
void FlipChar(int pos, int newChar, int oldChar, int step)
{
    BYTE *old = g_feFont + *(DWORD *)(g_feFont + 6 + oldChar * 4);
    BYTE *dst = g_textBuf + *(DWORD *)(g_textBuf + 6 + pos * 4) + 12;
    BYTE *src = g_feFont + *(DWORD *)(g_feFont + 6 + newChar * 4) + 12;
    DWORD mask = g_flipMasks[step][1];
    int row;

    for (row = 24; row; row--) {
        if (mask & 1) {
            UnpackRow(dst + 1, src);
            dst += 26;
        }
        while (*src != 0x80)
            src++;
        mask >>= 1;
        src++;
    }
    old += 12;
    mask = g_flipMasks[step][0];
    for (row = 24; row; row--) {
        if (mask & 1) {
            UnpackRow(dst + 1, old);
            dst += 26;
        }
        while (*old != 0x80)
            old++;
        mask >>= 1;
        old++;
    }
}

int g_leftRate = 1;
int g_rightRate = 1;

/* 0x407730: the left lemming's slap travels rightwards */
int AdvanceLeft(int *pos)
{
    int n = g_leftRate >> 3;
    int done = 0;
    int i;

    for (i = 0; i < 20; i++) {
        if (pos[i] != 32)
            pos[i]++;
        else
            done++;
        if (--n == -1)
            break;
    }
    if (g_leftRate != 160)
        g_leftRate++;
    if (done == 20) {
        g_leftRate = 1;
        memset(pos, 0, 20 * sizeof(int));
    }
    return done == 20;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_advanceright0, pad_advanceright1, pad_advanceright2, pad_advanceright3, pad_advanceright4, pad_advanceright5, pad_advanceright6, pad_advanceright7, pad_advanceright8, pad_advanceright9, pad_advanceright10, pad_advanceright11, pad_advanceright12, pad_advanceright13;

/* 0x4077a0 */
int AdvanceRight(int *pos)
{
    int n = g_rightRate >> 3;
    int done = 0;
    int i;

    for (i = 19; i > -1; i--) {
        if (pos[i] != 32)
            pos[i]++;
        else
            done++;
        if (--n == -1)
            break;
    }
    if (g_rightRate != 160)
        g_rightRate++;
    if (done == 20) {
        g_rightRate = 1;
        memset(pos, 0, 20 * sizeof(int));
    }
    return done == 20;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_updatetext0, pad_updatetext1;

/* 0x407810: nonzero when the whole line has turned */
int UpdateText(char *newText, char *oldText, int left)
{
    int i;
    char *s = newText;
    char *t = oldText;

    for (i = 0; *s; s++, t++, i++) {
        if (g_textPrev[i] != g_textPos[i])
            FlipChar(i, CharIndex(*s), CharIndex(*t), g_textPos[i] >> 2);
        g_textPrev[i] = g_textPos[i];
    }
    if (left)
        return AdvanceLeft(g_textPos);
    return AdvanceRight(g_textPos);
}

int g_textIndex = 0;
int g_textDir = 1;
int g_textBusy = 0;
int g_slapDone = 0;
int g_textWait = -1;
int g_textDelay = 50;

/* 0x4078a0 */
void AnimateTitle(int init)
{
    static char *s_texts[5] = {
        " PSYGNOSIS PRESENTS ",
        " A  abcdef ghijklmn ",
        "   PRODUCTION OF A  ",
        "   DMA DESIGN GAME  ",
        "  (C)1995 PSYGNOSIS ",
    };
    int cur = g_textIndex;
    int next = cur == 4 ? 0 : cur + 1;
    if (init) {
        UpdateText(s_texts[next], s_texts[cur], g_textDir);
        return;
    }
    if (g_textWait >= 0) {
        g_textWait--;
        g_textBusy = 0;
        g_slapDone = 0;
        return;
    }
    if (!g_slapDone) {
        if (g_textDir) {
            if (g_slapLFrame == 58)
                g_slapLFrame = 0;
            else
                g_slapLFrame++;
            g_slapDone = !g_slapLFrame;
        } else {
            if (g_slapRFrame == 58)
                g_slapRFrame = 0;
            else
                g_slapRFrame++;
            g_slapDone = !g_slapRFrame;
        }
    }
    g_textBusy = g_slapLFrame >= 32 || g_slapRFrame >= 32 || g_slapDone;
    if (g_textBusy && UpdateText(s_texts[next], s_texts[cur], g_textDir)) {
        if (++g_textIndex == 5)
            g_textIndex = 0;
        g_textDir ^= 1;
        g_textWait = g_textDelay;
    }
}

/* 0x4079e0 */
void StartFrontEnd(HWND hwnd)
{
    BYTE *pal;
    PDIB dib;
    int cols;
    int rows;
    int i;
    int x;
    int y;

    if (!g_feRenderer)
        g_feRenderer = Renderer_Initialise(0);
    pal = LoadPalFile("gfx\\fe.pal");
    dib = CreateFrontDib(pal);
    VSMEM_Free(pal);
    g_feSlot = LoadFrontCanvas(g_feRenderer, dib);
    SetFrontPalette(dib);
    g_feListAllState = NewDisplayList(256);
    g_feList2Extra = NewDisplayList(256);
    g_feListAllState->count = 0;
    g_feList2Extra->count = 0;
    for (i = 0; i < 256; i++) {
        g_feListAllState->entries[i].slot = g_feSlot;
        g_feList2Extra->entries[i].slot = g_feSlot;
    }
    g_feSprites = LoadSpriteFile("gfx\\fe.spr");
    g_feFont = LoadSpriteFile("gfx\\fefont.spr");
    g_slapL = LoadSpriteFile("gfx\\slapl.spr");
    g_slapR = LoadSpriteFile("gfx\\slapr.spr");
    cols = (GetDibWidth(dib) + 158) / 159;
    rows = (GetDibHeight(dib) + 135) / 136;
    for (y = 0; y < rows; y++) {
        for (x = 0; x < cols; x++)
            AddSprite(x * 159, y * 136, g_feSprites, 0, 3);
    }
    g_feCentre = (WORD)GetDibWidth(dib) >> 1;
    AddSprite(g_feCentre - 160, 10, g_feSprites, 1, 3);
    AddSprite(g_feCentre - 201, 175, g_feSprites, 2, 3);
    AddSprite(g_feCentre + 10, 175, g_feSprites, 3, 3);
    InitTextBuffer();
    AnimateTitle(1);
    Renderer_RenderList(g_feRenderer, g_feListAllState);
    FreeDib(dib);
}

/* 0x407c10 */
void QuitFrontEnd(HWND hwnd)
{
    if (g_fePalette) {
        DeleteObject(g_fePalette);
        g_fePalette = 0;
    }
    if (g_feSprites) {
        VSMEM_Free(g_feSprites);
        g_feSprites = 0;
    }
    if (g_textBuf) {
        VSMEM_Free(g_textBuf);
        g_textBuf = 0;
    }
    if (g_feFont) {
        VSMEM_Free(g_feFont);
        g_feFont = 0;
    }
    if (g_slapL) {
        VSMEM_Free(g_slapL);
        g_slapL = 0;
    }
    if (g_slapR) {
        VSMEM_Free(g_slapR);
        g_slapR = 0;
    }
    if (g_feRenderer) {
        Renderer_Quit(g_feRenderer);
        g_feRenderer = 0;
    }
    FreeDisplayList(g_feListAllState);
    FreeDisplayList(g_feList2Extra);
}
