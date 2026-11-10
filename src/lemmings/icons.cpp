/*
 * The skill panel: eight skill buttons, the release rate's + and -, pause,
 * nuke and fast forward, each a small window drawing its own sprite.
 */
#include <windows.h>
#include "winlemm.h"
#include "window.h"

UINT g_iconTimer = 0x1234;
int g_releaseRate = 0;
int g_minReleaseRate = 50;
int g_resetting = 0;            /* deselecting everything: no sounds, no commands */
int g_textHeight = 10;
int g_charWidth = 10;

HWND g_hPlusButton;

/* 0x40e6c0 */
IconWnd::IconWnd()
{
}

/* 0x40e780 */
IconWnd::~IconWnd()
{
}

/* 0x40e7f0 */
void IconWnd::OnCreate()
{
    RECT r;
    int i;

    r.top = 4;
    r.left = 4;
    r.right = 40;
    r.bottom = g_textHeight + 60;
    for (i = 0; i < 13; i++) {
        switch (i) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            m_skills[i].Create(&r, m_hwnd, "icon button", i);
            break;
        case 8:
            m_plus.Create(&r, m_hwnd, "plus button", i);
            break;
        case 9:
            m_minus.Create(&r, m_hwnd, "minus button", i);
            break;
        case 10:
            m_pause.Create(&r, m_hwnd, "paws button", i);
            break;
        case 11:
            m_fast.Create(&r, m_hwnd, "ff button", i);
            break;
        case 12:
            m_nuke.Create(&r, m_hwnd, "nuke button", i);
            break;
        }
        r.top += 44;            /* r.top is x here (see Window::Create) */
    }
    m_skills[0].LoadSprite("gfx\\ico_clim.spr");
    m_skills[1].LoadSprite("gfx\\ico_flot.spr");
    m_skills[2].LoadSprite("gfx\\ico_bomb.spr");
    m_skills[3].LoadSprite("gfx\\ico_blok.spr");
    m_skills[4].LoadSprite("gfx\\ico_bild.spr");
    m_skills[5].LoadSprite("gfx\\ico_bash.spr");
    m_skills[6].LoadSprite("gfx\\ico_axe.spr");
    m_skills[7].LoadSprite("gfx\\ico_dig.spr");
    m_plus.LoadSprite("gfx\\ico_plus.spr");
    m_minus.LoadSprite("gfx\\ico_mins.spr");
    m_pause.LoadSprite("gfx\\ico_paws.spr");
    m_nuke.LoadSprite("gfx\\ico_nuke.spr");
    m_fast.LoadSprite("gfx\\ico_ff.spr");
    m_timer = SetTimer(m_hwnd, g_iconTimer, 64, 0);
    m_selected = -1;
    {
        int pressed = 1;
        m_skills[0].SetPressed(pressed);
    }
    g_minReleaseRate = 50;
    if (g_releaseRate < g_minReleaseRate)
        g_releaseRate = g_minReleaseRate;
}

/* 0x40ea10 */
void IconWnd::OnDestroy()
{
    if (m_timer) {
        KillTimer(m_hwnd, g_iconTimer);
        m_timer = 0;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_iconwndproc0, pad_iconwndproc1, pad_iconwndproc2, pad_iconwndproc3, pad_iconwndproc4, pad_iconwndproc5, pad_iconwndproc6, pad_iconwndproc7, pad_iconwndproc8, pad_iconwndproc9, pad_iconwndproc10, pad_iconwndproc11, pad_iconwndproc12, pad_iconwndproc13, pad_iconwndproc14, pad_iconwndproc15, pad_iconwndproc16, pad_iconwndproc17, pad_iconwndproc18, pad_iconwndproc19, pad_iconwndproc20, pad_iconwndproc21, pad_iconwndproc22, pad_iconwndproc23, pad_iconwndproc24, pad_iconwndproc25, pad_iconwndproc26, pad_iconwndproc27, pad_iconwndproc28, pad_iconwndproc29;

/* 0x40ea40 */
LRESULT IconWnd::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    int state;
    int i;
    int pressed0 = 0;
    RECT r;
    int old;

    switch (msg) {
    case WM_SETFOCUS:
        SendMessage(g_hwnd, WM_NCACTIVATE, 1, 0);
        break;
    case WM_COMMAND:
        switch (wParam) {
        case 0xff00:            /* deselect everything */
            g_resetting = 1;
            for (i = 0; i < 13; i++) {
                switch (i) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                    m_skills[i].SetPressed(pressed0);
                    if (m_hwnd)
                        SendMessage(m_hwnd, WM_COMMAND, 0xff04, i);
                    break;
                case 8:
                    m_plus.SetPressed(pressed0);
                    break;
                case 9:
                    m_minus.SetPressed(pressed0);
                    break;
                case 10:
                    m_pause.SetPressed(pressed0);
                    break;
                case 11:
                    m_fast.SetPressed(pressed0);
                    break;
                case 12:
                    m_nuke.SetPressed(pressed0);
                    break;
                }
            }
            m_selected = -1;
            {
                int pressed = 1;
                m_skills[0].SetPressed(pressed);
            }
            g_resetting = 0;
            break;
        case 0xff01:            /* press a button */
            switch (LOWORD(lParam)) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                {
                    int pressed = 1;
                    m_skills[LOWORD(lParam)].SetPressed(pressed);
                }
                break;
            case 8:
                m_minus.Tick();
                break;
            case 9:
                m_plus.Tick();
                break;
            case 11:
                old = m_pause.m_pressed;
                state = old ^ 1;
                m_pause.SetPressed(state);
                break;
            case 12:
                {
                    int old = m_fast.m_pressed;
                    int pressed = old ^ 1;
                    m_fast.SetPressed(pressed);
                }
                break;
            case 13:
                {
                    int pressed = 1;
                    m_nuke.SetPressed(pressed);
                }
                break;
            }
            break;
        case 0xff04:            /* a button's number changed */
            switch (LOWORD(lParam)) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
                m_skills[LOWORD(lParam)].m_count = g_levelVal.skills[LOWORD(lParam)];
                r.top = 6;
                r.left = 20 - g_charWidth / 2;
                r.bottom = g_textHeight + 6;
                r.right = g_charWidth - g_charWidth / 2 + 20;
                InvalidateRect(m_skills[LOWORD(lParam)].m_hwnd, &r, FALSE);
                break;
            case 8:
                CountRect(&r);
                InvalidateRect(m_minus.m_hwnd, &r, FALSE);
                break;
            case 9:
                CountRect(&r);
                InvalidateRect(m_plus.m_hwnd, &r, FALSE);
                break;
            case 11:
                CountRect(&r);
                InvalidateRect(m_pause.m_hwnd, &r, FALSE);
                break;
            case 13:
                CountRect(&r);
                InvalidateRect(m_nuke.m_hwnd, &r, FALSE);
                break;
            }
            break;
        case 0xff07:            /* every skill's number changed */
            for (i = 0; i < 7; i++) {
                if (m_hwnd)
                    SendMessage(m_hwnd, WM_COMMAND, 0xff04, i);
            }
            break;
        }
        break;
    case WM_TIMER:
        if (g_iconTimer == wParam) {
            for (i = 0; i < 13; i++) {
                switch (i) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                    if (m_skills[i].m_pressed)
                        m_skills[i].Tick();
                    break;
                case 8:
                    if (m_plus.m_pressed)
                        m_plus.Tick();
                    break;
                case 9:
                    if (m_minus.m_pressed)
                        m_minus.Tick();
                    break;
                case 10:
                    if (m_pause.m_pressed)
                        m_pause.Tick();
                    break;
                case 11:
                    if (m_fast.m_pressed)
                        m_fast.Tick();
                    break;
                case 12:
                    if (m_nuke.m_pressed)
                        m_nuke.Tick();
                    break;
                }
            }
        }
        break;
    case 0x403:                 /* a new release rate */
        g_releaseRate = g_minReleaseRate = wParam;
        r.top = 6;
        r.left = 20 - g_charWidth / 2;
        r.bottom = g_textHeight + 6;
        r.right = g_charWidth - g_charWidth / 2 + 20;
        InvalidateRect(m_plus.m_hwnd, &r, FALSE);
        CountRect(&r);
        InvalidateRect(m_minus.m_hwnd, &r, FALSE);
        break;
    case 0x800:                 /* a skill button was pressed */
        if (m_selected != -1)
            m_skills[m_selected].SetPressed(pressed0);
        if (m_selected != (int)wParam) {
            m_selected = wParam;
            SendMessage(g_hwnd, WM_COMMAND, LOWORD(wParam) + 0x15e, 0);
        }
        break;
    }
    return DockWnd::WndProc(msg, wParam, lParam);
}

/* 0x40f220 */
void IconWnd::ClampPosition(POINT *pt)
{
    DockWnd::ClampPosition(pt);
}

/* 0x40f230 */
void IconWnd::OnDock(RECT *rect, int docked)
{
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_buttonbutton0, pad_buttonbutton1, pad_buttonbutton2, pad_buttonbutton3, pad_buttonbutton4, pad_buttonbutton5, pad_buttonbutton6, pad_buttonbutton7, pad_buttonbutton8, pad_buttonbutton9, pad_buttonbutton10, pad_buttonbutton11, pad_buttonbutton12, pad_buttonbutton13, pad_buttonbutton14, pad_buttonbutton15, pad_buttonbutton16, pad_buttonbutton17, pad_buttonbutton18, pad_buttonbutton19, pad_buttonbutton20;

/* 0x40f240 */
Button::Button()
{
    HDC hdc;
    HFONT old;
    TEXTMETRIC tm;
    int height;

    m_cursor = LoadCursor(0, IDC_ARROW);
    SetCursor(m_cursor);
    m_bkColor = GetSysColor(COLOR_BTNFACE);
    m_pressed = 0;
    m_sprite = 0;
    m_count = 10;
    m_frame = 0;
    m_bitmap = new Bitmap(34, 42, GetSysColor(COLOR_BTNFACE));
    hdc = GetDC(GetDesktopWindow());
    height = MulDiv(-10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    m_font = CreateFont(height, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, 0, 0, 0, 0, FF_SWISS | 6, "MS Sans Serif");
    old = (HFONT)SelectObject(hdc, m_font);
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, old);
    ReleaseDC(GetDesktopWindow(), hdc);
    g_textHeight = tm.tmAscent;
    g_charWidth = tm.tmAveCharWidth / 2 + 2 * tm.tmAveCharWidth;
    if (g_textHeight > 30)
        g_textHeight = 10;
}

/* 0x40f370 */
Button::~Button()
{
    if (m_bitmap)
        delete m_bitmap;
    if (m_sprite)
        VSMEM_Free(m_sprite);
    if (m_font)
        DeleteObject(m_font);
}

/* 0x40f3c0: a one-pixel bevel round the button */
void Button::DrawBorder(HDC hdc, RECT *rect, HBRUSH light, HBRUSH dark)
{
    RECT r;

    r.top = 0;
    r.left = 0;
    r.bottom = 1;
    r.right = 39;
    FillRect(hdc, &r, light);
    r.bottom = g_textHeight + 59;
    r.right = 1;
    FillRect(hdc, &r, light);
    r.right = 40;
    r.top = g_textHeight + 59;
    r.bottom = g_textHeight + 60;
    FillRect(hdc, &r, dark);
    r.top = 0;
    r.left = 39;
    FillRect(hdc, &r, dark);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_buttondrawface0, pad_buttondrawface1, pad_buttondrawface2, pad_buttondrawface3, pad_buttondrawface4, pad_buttondrawface5, pad_buttondrawface6;

/* 0x40f470: the box with the button's number */
void Button::DrawFace(HDC hdc, RECT *rect, HBRUSH light, HBRUSH dark)
{
    HBRUSH black = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RECT r;
    char text[4];
    HFONT old;

    CountRect(&r);
    FillRect(hdc, &r, black);
    DeleteObject(black);
    r.top = 5;
    r.bottom = 6;
    r.left = 19 - g_charWidth / 2;
    r.right = g_charWidth - g_charWidth / 2 + 21;
    FillRect(hdc, &r, dark);
    r.bottom = g_textHeight + 7;
    r.right = 20 - g_charWidth / 2;
    FillRect(hdc, &r, dark);
    r.top = 5;
    r.left = g_charWidth - g_charWidth / 2 + 20;
    r.bottom = g_textHeight + 7;
    r.right = g_charWidth - g_charWidth / 2 + 21;
    FillRect(hdc, &r, light);
    r.top = g_textHeight + 6;
    r.left = 20 - g_charWidth / 2;
    r.bottom = g_textHeight + 7;
    r.right = g_charWidth - g_charWidth / 2 + 21;
    FillRect(hdc, &r, light);
    if (m_count) {
        old = (HFONT)SelectObject(hdc, m_font);
        SetBkColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        wsprintf(text, "%02d", m_count);
        r.left = 20 - g_charWidth / 2;
        r.top = 6;
        r.bottom = g_textHeight + 5;
        r.right = g_charWidth - g_charWidth / 2 + 19;
        ExtTextOut(hdc, 21 - g_charWidth / 2, 4, ETO_CLIPPED, &r, text, 2, 0);
        SelectObject(hdc, old);
    }
}

/* 0x40f680 */
void Button::Paint(HDC hdc, RECT *rect)
{
    HBRUSH light;
    HBRUSH dark;
    HBRUSH t;

#ifdef PORT_SDL                 /* the native build: a system control shows the button (PORTING.md) */
    if (port_native_icon(m_hwnd, m_sprite, m_frame, m_bitmap->m_colours, m_id < 10 ? m_count : -1, m_pressed))
        return;
#endif
    if (g_win32)
        light = (HBRUSH)GetStockObject(WHITE_BRUSH);
    else
        light = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT));
    dark = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
    if (m_pressed) {
        t = light;
        light = dark;
        dark = t;
    }
    DrawBorder(hdc, rect, light, dark);
    DrawFace(hdc, rect, light, dark);
    m_bitmap->Clear();
    if (m_sprite)
        DrawSprite(0, 0, m_frame);
    m_bitmap->Draw(hdc, 2, g_textHeight + 12, 34, 42, 0, 0);
    DeleteObject(light);
    DeleteObject(dark);
}

/* 0x40f780 */
LRESULT Button::WndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_MOUSEMOVE:
        SetStatusText(GetString(m_id + 0x127));
        break;
    case WM_LBUTTONDOWN:
        SetPressed(1);
        InvalidateRect(m_hwnd, 0, TRUE);
        break;
    }
    return Window::WndProc(msg, wParam, lParam);
}

/* 0x40f820 */
void Button::OnCreate()
{
}

/* 0x40f830: the next frame of the pressed button's animation */
void Button::Tick()
{
    RECT r;

    if (m_sprite && m_pressed) {
        if (++m_frame == ((WORD *)m_sprite)[2])
            m_frame = 0;
    }
    r.left = 2;
    r.right = 36;
    r.top = g_textHeight + 12;
    r.bottom = g_textHeight + 54;
    InvalidateRect(m_hwnd, &r, FALSE);
}

/* 0x40f8a0 */
void Button::LoadSprite(char *file)
{
    HANDLE f = FILE_ReadOpen(file);
    DWORD size = FILE_Length(f);

    m_sprite = (BYTE *)VSMEM_Allocate(size, "Icon button sprite file");
    FILE_Read(f, m_sprite, size);
    FILE_Close(f);
    if (*(DWORD *)m_sprite != 'ELRS')
        MessageBox(0, "File not an SRLE file", file, 0);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_buttondrawsprite0, pad_buttondrawsprite1, pad_buttondrawsprite2, pad_buttondrawsprite3, pad_buttondrawsprite4, pad_buttondrawsprite5, pad_buttondrawsprite6, pad_buttondrawsprite7, pad_buttondrawsprite8, pad_buttondrawsprite9, pad_buttondrawsprite10, pad_buttondrawsprite11, pad_buttondrawsprite12, pad_buttondrawsprite13, pad_buttondrawsprite14, pad_buttondrawsprite15, pad_buttondrawsprite16, pad_buttondrawsprite17, pad_buttondrawsprite18, pad_buttondrawsprite19, pad_buttondrawsprite20, pad_buttondrawsprite21, pad_buttondrawsprite22;

/* 0x40f900: unpack one frame of the sprite into the bitmap */
void Button::DrawSprite(int x, int y, int frame)
{
    BYTE *spr = m_sprite + *(DWORD *)(m_sprite + 6 + frame * 4);
    Bitmap *bm;
    int pitch;
    BYTE *row;

    y += ((WORD *)spr)[1];
    x += ((WORD *)spr)[0];
    bm = m_bitmap;
    pitch = bm->m_pitch;
    row = x + bm->m_rows[y];
    BYTE *src = spr + 12;
    int j;

    for (j = 0; j < ((WORD *)spr)[3]; j++) {
        BYTE *dst = row;
        BYTE c;

        do {
            c = *src++;
            if (c < 0x80)
                dst += c;
            else if (c > 0x80) {
                c &= 0x7f;
                memcpy(dst, src, c);
                dst += c;
                src += c;
            }
        } while (c != 0x80);
        row += pitch;
    }
}
