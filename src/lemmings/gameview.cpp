/*
 * The play area ("wndBMPClass"): loads the level into the renderer, steps
 * the lemmings each frame, plays back replays, applies skills where the
 * player clicks and scrolls the view.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0;

#include <windows.h>
extern HCURSOR g_lastCursor;
extern int pad_cursor_top0, pad_cursor_top1, pad_cursor_top2, pad_cursor_top3, pad_cursor_top4, pad_cursor_top5, pad_cursor_top6, pad_cursor_top7, pad_cursor_top8, pad_cursor_top9, pad_cursor_top10, pad_cursor_top11, pad_cursor_top12, pad_cursor_top13, pad_cursor_top14, pad_cursor_top15, pad_cursor_top16, pad_cursor_top17, pad_cursor_top18, pad_cursor_top19, pad_cursor_top20, pad_cursor_top21, pad_cursor_top22, pad_cursor_top23, pad_cursor_top24, pad_cursor_top25, pad_cursor_top26, pad_cursor_top27, pad_cursor_top28, pad_cursor_top29, pad_cursor_top30, pad_cursor_top31, pad_cursor_top32;
extern HCURSOR g_hSmaBoxCursorGameNew;
#include <string.h>
#include "winlemm.h"
#include "window.h"
extern HCURSOR g_hBoxCursorSpare;

int g_recordTime = 0;
int g_clicked = 0;
WPARAM g_clickWParam = 0;
LPARAM g_clickLParam = 0;
int g_clickSkill = 0;
WORD g_renderer = 0;
WORD g_slot = 0;
short g_scale = 1;
BYTE *g_rrSprite = 0;
HCURSOR g_lastCursor = 0;
HCURSOR g_viewCursor = 0;
int g_unused67c = 0;
char g_viewClass[] = "wndBMPClass";
int g_inGame = 0;
int g_needTitle = 0;
UINT g_scrollTimer = 0x1111;
int g_priority = 1;
int g_unused69c = 0;
int g_skillButtons[17] = {-1, -1, 0, -1, 1, -1, 2, -1, 3, -1, -1, -1, 4, -1, 7, 5, 6};

LEVEL g_levelVal;
int g_viewW, g_viewHLast;           /* the view in level pixels */
int g_viewPixW, g_viewPixHGameMain;     /* and in window pixels */
short g_dataViewWidth, g_extraViewHeight;        /* the window's client size */
short g_tempScrollX, g_valScrollY;
short g_maxXScroll, g_yMaxScroll;
short g_scrollSpeedOld;            /* while dragging with the right button */
short g_dragStartCur;
int g_skill;
int g_timeMinData, g_timeSec, g_savedFrames, g_usedFps;
DISPLAYLIST *g_list, *g_list2;
int g_nowRenderScale;
UINT g_lastDragTimer;
PDIB g_oldLevelDib;
int g_allUnused6a8;              /* unused: fills a gap in the original .bss */
char g_savedStatusBuf[128];

/* 0x404d40 */
int InGame(void)
{
    return g_inGame;
}

/* 0x404d50 */
int RegisterViewClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ViewWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, g_szAppName);
    wc.hCursor = 0;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = 0;
    wc.lpszClassName = g_viewClass;
    return RegisterClass(&wc);
}

/* 0x404dc0 */
HWND CreateGameView(HINSTANCE hInstance, HWND parent, int id, RECT *rect)
{
    g_viewCursor = g_hCrossCursorNext;
    return CreateWindowEx(0, g_viewClass, 0, WS_CHILD | WS_CLIPSIBLINGS | WS_HSCROLL | WS_VSCROLL,
                          rect->left, rect->top, rect->right, rect->bottom, parent, (HMENU)id, hInstance, 0);
}

/* 0x404e10: Ctrl or the middle button gives skills to the lemmings at the back */
void UpdatePriority(void)
{
    if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000) && !(GetAsyncKeyState(VK_MBUTTON) & 0x8000))
        g_priority = 1;
    else
        g_priority = 0;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */

extern int pad_viewwndproc0, pad_viewwndproc1, pad_viewwndproc2, pad_viewwndproc3, pad_viewwndproc4, pad_viewwndproc5, pad_viewwndproc6, pad_viewwndproc7, pad_viewwndproc8, pad_viewwndproc9, pad_viewwndproc10, pad_viewwndproc11, pad_viewwndproc12, pad_viewwndproc13, pad_viewwndproc14, pad_viewwndproc15, pad_viewwndproc16, pad_viewwndproc17, pad_viewwndproc18, pad_viewwndproc19, pad_viewwndproc20, pad_viewwndproc21, pad_viewwndproc22, pad_viewwndproc23, pad_viewwndproc24, pad_viewwndproc25, pad_viewwndproc26, pad_viewwndproc27;
/* 0x404e50 */
LRESULT CALLBACK ViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UpdatePriority();
    switch (msg) {
    case WM_CREATE:
        OnViewCreate(hwnd, wParam, lParam);
        return 0;
    case WM_DESTROY:
        OnStopDrag(hwnd, wParam, lParam);
        OnViewDestroy(hwnd, wParam, lParam);
        return 0;
    case WM_SIZE:
        g_wViewClient = LOWORD(lParam);
        g_clientViewH = GetSystemMetrics(SM_CYVTHUMB) + HIWORD(lParam);
        OnViewSize(hwnd, wParam, lParam);
        return 0;
    case WM_PAINT:
        if (g_needTitle) {
            RebuildPalette();
            g_needTitle = 0;
        }
        OnViewPaint(hwnd, wParam, lParam);
        return 0;
    case WM_SETCURSOR:
        SetCursor(g_viewCursor);
        return 0;
    case WM_KEYDOWN:
        OnKeyDown(hwnd, wParam, lParam);
        return 0;
    case WM_KEYUP:
        OnKeyUp(hwnd, wParam, lParam);
        return 0;
    case WM_TIMER:
        if (wParam == g_scrollTimer) {
            OnScrollTimer(hwnd);
            return 0;
        }
        break;
    case WM_HSCROLL:
        OnHScroll(hwnd, msg, wParam, lParam);
        return 0;
    case WM_VSCROLL:
        OnVScroll(hwnd, msg, wParam, lParam);
        return 0;
    case WM_MOUSEMOVE:
        OnMouseMove(hwnd, wParam, lParam);
        return 0;
    case WM_LBUTTONDOWN:
        g_clickWParam = wParam;
        g_clickLParam = lParam;
        g_clickSkill = g_skill;
        g_clicked = 1;
        return 0;
    case WM_RBUTTONDOWN:
        OnStartDrag(hwnd, wParam, lParam);
        return 0;
    case WM_RBUTTONUP:
        OnStopDrag(hwnd, wParam, lParam);
        return 0;
    case MM_MCINOTIFY:
        switch (LOWORD(wParam)) {
        case MCI_NOTIFY_SUCCESSFUL:
            RestartMidi();
            return 0;
        default:
            break;
        }
        break;
    case 0x400:                 /* the next frame */
        OnFrame(hwnd, wParam, lParam);
        UpdateCursor(hwnd);
        g_recordTime++;
        return 0;
    case 0x401:                 /* start the level */
        g_recordTime = 0;
        if (g_replaying)
            StartReplay();
        else
            StartRecord();
        OnStartLevel(hwnd, wParam, lParam);
        return 0;
    case 0x402:                 /* end it */
        OnEndLevel(hwnd, wParam, lParam);
        if (g_replaying)
            EndReplay();
        else
            EndRecord();
        return 0;
    case 0x403:                 /* the release rate */
    {
        int rate;
        int value;
        rate = 99;
        value = LOWORD(wParam);
        SetLemmingDelay((rate - value) / 2 + 4);
        return 0;
    }
    case 0x500:
        OnRedraw(hwnd, wParam, lParam);
        return 0;
    case 0x501:
        OnZoom(hwnd, wParam, lParam);
        return 0;
    case 0x502:
        g_skill = LOWORD(wParam);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* 0x4052c0: the level's own colours go at 128-191 of the game's palette */
void MergePalette(PDIB dib, RGBQUAD *pal)
{
    memcpy(pal + 128, (RGBQUAD *)((BYTE *)dib + sizeof(BITMAPINFOHEADER)) + 128, 64 * sizeof(RGBQUAD));
    memcpy((BYTE *)dib + sizeof(BITMAPINFOHEADER), pal, 256 * sizeof(RGBQUAD));
}

/* 0x4052f0 */
void OnStartLevel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    WORD renderer;
    char title[128];
    PDIB dib;
    int i;
    int rate;

    if (!g_renderer)
        renderer = Renderer_Initialise(0);
    if (renderer) {
        g_scale = g_zoom;
        LoadLevel(g_levelFile, renderer, &g_levelVal, g_resolutionNew);
        dib = g_levelVal.terrain;
        if (g_lastLsCustom) {
            g_levelVal.timeLimit = g_spareLsTime;
            g_levelVal.numLemmings = g_lsLemmingsSpareMain;
            g_levelVal.toRescue = g_lsRescueState;
            g_levelVal.releaseRate = g_stateLsRate;
            memcpy(g_levelVal.skills, g_infoLsSkills, sizeof(g_levelVal.skills));
        }
        strcpy(g_levelVal.name, g_lsName);
        wsprintf(title, GetString(0x114), g_userName, g_levelVal.name);
        SetWindowText(GetParent(hwnd), title);
        g_oldLevelDib = dib;
        MergePalette(dib, g_tempPalette);
        memcpy(g_palette2, g_tempPalette, sizeof(g_palette2));
        g_slot = Renderer_LoadCanvas_BitMap(renderer, g_oldLevelDib, 2, g_palette2);
        g_renderer = renderer;
        g_list = NewDisplayList(1);
        g_list2 = NewDisplayList(1);
        g_list->count = 0;
        g_list2->count = 0;
        for (i = 0; i < 1; i++) {
            g_list->entries[i].slot = g_slot;
            g_list2->entries[i].slot = g_slot;
        }
        g_rrSprite = LoadSpriteFile("gfx\\rr.spr");
        SetScale(g_resolutionNew);
        SetLevelMap(GetDibBitsAddr(dib), GetDibWidth(dib) / g_resolutionNew, GetDibHeight(dib) / g_resolutionNew);
        InitialiseLemmings(g_renderer, g_slot, &g_levelVal);
        InitALemming(450, 90);
        g_inGame = 1;
        FreeDib(g_levelVal.terrain);
        g_levelVal.terrain = 0;
        ClearPause();
        ClearNuke();
        SetViewScale(hwnd, g_scale);
        g_tempScrollX = (g_levelVal.startX - g_viewW / g_resolutionNew / 2) * g_scale * (short)g_resolutionNew;
        g_valScrollY = g_levelVal.field60 * g_scale * (short)g_resolutionNew;
        SetViewScale(hwnd, g_scale);
        g_timeMinData = g_levelVal.timeLimit;
        g_usedFps = 16;
        rate = g_levelVal.releaseRate;
        g_timeSec = 0;
        g_savedFrames = 16;
        SetLemmingDelay((99 - rate) / 2 + 4);
        SetNumLemmings(g_levelVal.numLemmings);
        SetSFXFlags(g_visualFX, g_soundFX);
        if (g_iconWnd && g_iconWnd->m_hwnd)
            PostMessage(g_iconWnd->m_hwnd, 0x403, rate, 0);
        OnMouseMove(hwnd, 0, 0);
        if (g_mapWnd && g_mapWnd->m_hwnd)
            SendMessage(g_mapWnd->m_hwnd, 0x401, 0, 0);
        OpenMidi(GetLevelMusic());
        PlayMidi(hwnd);
        SetWindowPos(g_hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        g_needTitle = 1;
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */

extern int pad_countdown0, pad_countdown1, pad_countdown2, pad_countdown3, pad_countdown4, pad_countdown5, pad_countdown6, pad_countdown7, pad_countdown8, pad_countdown9, pad_countdown10, pad_countdown11, pad_countdown12, pad_countdown13, pad_countdown14, pad_countdown15, pad_countdown16, pad_countdown17, pad_countdown18;
/* 0x4056b0 */
void CountDown(int *min, int *sec, int *frames, int fps)
{
    if (--*frames < 0) {
        *frames = fps;
        if (--*sec < 0) {
            *sec = 59;
            (*min)--;
        }
    }
}

/* 0x4056e0 */
void OnFrame(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    REPLAYEVENT ev;
    RECT r;
    int alive;

    if (g_inGame) {
        InitALemming(450, 90);
        if (IsNuking())
            ProcessArmageddon();
        alive = ProcessLemmings(&ev.time);
        CountDown(&g_timeMinData, &g_timeSec, &g_savedFrames, g_usedFps);
        SetStatusSave(g_levelVal.toRescue, g_levelVal.numLemmings);
        SetStatusHome(GetLemmsHome());
        SetStatusOut(ev.time);
        SetStatusTime(g_timeMinData, g_timeSec);
        if ((!g_timeMinData && !g_timeSec) || !alive) {
            OnEndLevel(hwnd, 0, 0);
            PostMessage(g_hwnd, 0x602, 0, 0);
        }
    }
    if (g_clicked) {
        OnClick(hwnd, g_clickWParam, g_clickLParam);
        g_clicked = 0;
    }
    if (g_replaying && g_inGame) {
        while (GetReplayEvent(g_recordTime, &ev)) {
            switch (ev.type) {
            case 0:
                {
                    int button;

                    MakeLemming(ev.x, ev.y, ev.value, ev.priority);
                    g_levelVal.skills[ButtonForSkill(ev.value)]--;
                    button = ButtonForSkill(ev.value);
                    if (g_iconWnd->m_hwnd)
                        PostMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff04, button);
                }
                break;
            case 1:
                {
                    int dx = ev.x - g_viewW / 2 - g_tempScrollX;

                    ScrollTo(hwnd, dx);
                }
                break;
            case 2:
                {
                    int rate = ev.value;
                    IconWnd *icons = g_iconWnd;

                    SendMessage(g_hwnd, 0x403, rate, 0);
                    g_releaseRate = rate;
                    CountRect(&r);
                    InvalidateRect(icons->m_plus.m_hwnd, &r, FALSE);
                }
                break;
            case 3:
                SendMessage(g_hwnd, WM_COMMAND, 0x169, 0);
                break;
            }
        }
    }
}

/* 0x405980 */
void OnEndLevel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (g_mapWnd && g_mapWnd->m_hwnd)
        SendMessage(g_mapWnd->m_hwnd, 0x402, 0, 0);
    if (g_inGame) {
        DeleteLemmings();
        LevelFree(&g_levelVal);
        g_inGame = 0;
        CloseMidi();
    }
    if (g_renderer) {
        FreeDisplayList(g_list);
        FreeDisplayList(g_list2);
        if (g_rrSprite)
            VSMEM_Free(g_rrSprite);
        Renderer_Quit(g_renderer);
        g_renderer = 0;
    }
}

/* 0x405a20 */
int GetViewHeight(int zoom)
{
    return GetSystemMetrics(SM_CYVTHUMB) + g_resolutionNew * zoom * 160;
}

/* 0x405a40 */
void OnZoom(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    SetViewScale(hwnd, wParam);
    g_wViewClient = g_viewPixW;
    g_clientViewH = GetViewHeight(g_scale);
    PostMessage(GetParent(hwnd), 0x601, 0, MAKELONG(g_viewPixW, (WORD)(g_resolutionNew * g_scale * 160)));
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_renderview0, pad_renderview1, pad_renderview2;

/* 0x405ab0 */
void RenderView(HDC hdc, PAINTSTRUCT *ps)
{
    HPALETTE old = SelectPalette(hdc, g_hPalette, FALSE);
    int x;
    int y;

    RealizePalette(hdc);
    x = g_tempScrollX / g_scale;
    y = g_valScrollY / g_scale;
    SetClipRegion(g_renderer, g_slot, x, y, g_viewW, g_viewHLast);
    if (ps)
        Renderer_Revealed(g_renderer, g_slot, ps->rcPaint.left / g_scale + x, ps->rcPaint.top / g_scale + y,
                          (ps->rcPaint.right + g_scale) / g_scale + x, (ps->rcPaint.bottom + g_scale) / g_scale + y);
    if (g_inGame)
        RenderLemmings(g_renderer, g_slot);
    if (g_replaying) {
        int n = g_list->count++;
        RLEDATA *d;
        DLENTRY *e = &g_list->entries[n];
        DISPLAYLIST *t;

        e->type = DL_RLE;
        d = &e->u.rle;
        d->mode = 1;
        d->frame = (g_recordTime >> 3) & 1;
        d->sprites = g_rrSprite;
        d->x = g_viewW + x - 40;
        d->y = 5;
        Renderer_RenderList(g_renderer, g_list);
        g_list->count = 0;
        t = g_list;
        g_list = g_list2;
        g_list2 = t;
    }
    Renderer_Render(g_renderer, hdc, g_slot, x, y, g_viewW + x, g_viewHLast + y, g_viewPixW, g_viewPixHGameMain, g_nowRenderScale);
    SelectPalette(hdc, old, FALSE);
}

/* 0x405c80 */
void OnViewPaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    if (g_renderer)
        RenderView(hdc, &ps);
    EndPaint(hwnd, &ps);
}

/* 0x405cc0 */
void OnRedraw(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HDC hdc = GetDC(hwnd);

    if (g_renderer)
        RenderView(hdc, 0);
    ReleaseDC(hwnd, hdc);
}

/* 0x405cf0: which icon-panel button a skill is on */
int ButtonForSkill(int skill)
{
    return g_skillButtons[skill];
}

/* 0x405d00: the right button drags the view */
void OnStartDrag(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (g_renderer) {
        if (g_replaying) {
            StopReplay();
            g_replaying = 0;
        }
        g_lastDragTimer = SetTimer(hwnd, g_scrollTimer, 70, 0);
        if (g_lastDragTimer) {
            g_scrollSpeedOld = 0;
            g_dragStartCur = LOWORD(lParam);
            SetCapture(hwnd);
        }
    }
}

/* 0x405d60 */
void OnStopDrag(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (g_lastDragTimer) {
        KillTimer(hwnd, g_scrollTimer);
        g_scrollSpeedOld = 0;
        ReleaseCapture();
        g_lastDragTimer = 0;
    }
}

/* 0x405da0 */
void OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
}

/* 0x405db0 */
void OnKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
}

/* 0x405dc0 */
void OnScrollTimer(HWND hwnd)
{
    ScrollTo(hwnd, g_resolutionNew * g_scale * g_scrollSpeedOld * 4);
}

/* 0x405df0: give the selected skill to the lemming under the cursor */
void OnClick(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    PNT at;
    int scale;
    int x;
    int y;
    int button;

    if (!g_renderer)
        return;
    if (g_replaying) {
        StopReplay();
        g_replaying = 0;
    }
    scale = g_scale * g_resolutionNew;
    x = (g_tempScrollX + LOWORD(lParam)) / scale;
    y = (g_valScrollY + HIWORD(lParam)) / scale;
    if (!CheckLemming(x, y, g_clickSkill, g_priority))
        return;
    if (g_levelVal.skills[ButtonForSkill(g_clickSkill)] <= 0)
        return;
    at.x = 0;
    at.y = 0;
    if (MakeLemming(x, y, g_clickSkill, g_priority)) {
        g_levelVal.skills[ButtonForSkill(g_clickSkill)]--;
        PostMessage((HWND)g_global2c, WM_COMMAND, 0xff04, ButtonForSkill(g_clickSkill));
        button = ButtonForSkill(g_clickSkill);
        if (g_iconWnd->m_hwnd)
            PostMessage(g_iconWnd->m_hwnd, WM_COMMAND, 0xff04, button);
        if (!g_replaying)
            RecordSkill(g_recordTime, x, y, g_clickSkill, g_priority);
        SoundPlayFX(at, 16);
    } else
        SoundPlayFX(at, 22);
}

/* 0x405f70 */
void OnMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    int scale;
    int x;
    int y;
    char *name;

    if (!g_renderer)
        return;
    if (g_lastDragTimer)
        g_scrollSpeedOld = LOWORD(lParam) - g_dragStartCur;
    scale = g_scale * g_resolutionNew;
    x = (g_tempScrollX + LOWORD(lParam)) / scale;
    y = (g_valScrollY + HIWORD(lParam)) / scale;
    name = CheckLemming(x, y, g_skill, g_priority);
    if (name) {
        SetStatusText(name);
        SetViewCursor(g_hBoxCursorSpare, g_hSmaBoxCursorGameNew);
        return;
    }
    wsprintf(g_savedStatusBuf, GetString(0x174), g_levelVal.toRescue, g_levelVal.numLemmings);
    SetStatusText(g_savedStatusBuf);
    SetViewCursor(g_hCrossCursorNext, g_hSmaCrossCursorState);
}

/* 0x406060: the cursor in the view's client coordinates, if it is over it */
int GetViewMouse(HWND hwnd, POINT *pt)
{
    POINT screen;
    RECT r;

    GetCursorPos(pt);
    screen = *pt;
    ScreenToClient(g_hwnd, &screen);
    ScreenToClient(hwnd, pt);
    GetClientRect(hwnd, &r);
    if (!PtInRect(&r, *pt))
        return 0;
    if (ChildWindowFromPoint(g_hwnd, screen) != hwnd)
        return 0;
    if (r.left < pt->x && r.top < pt->y && r.right > pt->x && r.bottom > pt->y)
        return 1;
    return 0;
}

/* 0x406120 */
void UpdateCursor(HWND hwnd)
{
    POINT pt;
    int scale;

    if (!g_renderer)
        return;
    if (!GetViewMouse(hwnd, &pt))
        return;
    pt.x += g_tempScrollX;
    pt.y += g_valScrollY;
    scale = g_scale * g_resolutionNew;
    pt.x /= scale;
    pt.y /= scale;
    if (CheckLemming(pt.x, pt.y, g_skill, g_priority))
        SetViewCursor(g_hBoxCursorSpare, g_hSmaBoxCursorGameNew);
    else
        SetViewCursor(g_hCrossCursorNext, g_hSmaCrossCursorState);
}

/* 0x4061e0 */
void OnViewCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    g_renderer = 0;
    g_inGame = 0;
    g_valScrollY = 0;
    g_tempScrollX = 0;
    g_maxXScroll = g_yMaxScroll = g_resolutionNew * 1668;
    g_lastCursor = 0;
    SetMenuHelpCallback(ShowGameStatus);
}

/* 0x406230: remember where the windows were */
void OnViewDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    RECT r;
    RECT wnd;
    POINT origin;

    GetWindowRect(g_hwnd, &wnd);
    origin.x = 0;
    origin.y = 0;
    g_mainWidthNext = wnd.right - wnd.left;
    g_mainHeightOldTemp = wnd.bottom - wnd.top;
    ClientToScreen(g_hwnd, &origin);
    GetWindowRect(g_hwndView, &r);
    r.right -= r.left;
    r.bottom -= r.top;
    r.top -= origin.y;
    r.left -= origin.x;
    g_viewRectSpare = r;
    GetWindowRect((HWND)g_global28, &r);
    r.right -= r.left;
    r.bottom -= r.top;
    r.top -= origin.y;
    r.left -= origin.x;
    g_rectStatus = r;
    SendMessage(hwnd, 0x402, 0, 0);
    SetMenuHelpCallback(0);
}

/* 0x406350 */
void OnViewSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    g_dataViewWidth = LOWORD(lParam);
    g_extraViewHeight = HIWORD(lParam);
    SetViewScale(hwnd, g_scale);
}

/* 0x406380 */
void OnVScroll(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    short d;
    int n;

    switch (LOWORD(wParam)) {
    case SB_LINEUP:
        d = -(g_resolutionNew * g_scale);
        break;
    case SB_LINEDOWN:
        d = g_resolutionNew * g_scale;
        break;
    case SB_PAGEUP:
        d = g_resolutionNew * g_scale * -40;
        break;
    case SB_PAGEDOWN:
        d = g_resolutionNew * g_scale * 40;
        break;
    case SB_THUMBPOSITION:
        d = HIWORD(wParam) - g_valScrollY;
        break;
    case SB_THUMBTRACK:
        d = HIWORD(wParam) - g_valScrollY;
        break;
    case SB_TOP:
        d = -g_valScrollY;
        break;
    case SB_BOTTOM:
        d = g_yMaxScroll - g_valScrollY;
        break;
    default:
        d = 0;
        break;
    }
    n = (g_yMaxScroll - g_valScrollY < d) ? g_yMaxScroll - g_valScrollY : d;
    if (n <= -g_valScrollY)
        n = -g_valScrollY;
    if ((short)n) {
        g_valScrollY += n;
        ScrollWindow(hwnd, 0, -(short)n, 0, 0);
        SetScrollPos(hwnd, SB_VERT, g_valScrollY, TRUE);
        UpdateWindow(hwnd);
    }
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_scrollto0, pad_scrollto1, pad_scrollto2, pad_scrollto3, pad_scrollto4, pad_scrollto5, pad_scrollto6, pad_scrollto7, pad_scrollto8, pad_scrollto9, pad_scrollto10, pad_scrollto11;


/* 0x406490 */
void ScrollTo(HWND hwnd, short dx)
{
    /* Symbol padding preserves VC++ 2.2 register and comparison allocation. */
    extern int pad_p51_0,pad_p51_1,pad_p51_2,pad_p51_3,pad_p51_4,pad_p51_5,pad_p51_6,pad_p51_7,pad_p51_8,pad_p51_9,pad_p51_10,pad_p51_11,pad_p51_12,pad_p51_13,pad_p51_14,pad_p51_15,pad_p51_16,pad_p51_17,pad_p51_18,pad_p51_19,pad_p51_20,pad_p51_21,pad_p51_22,pad_p51_23,pad_p51_24,pad_p51_25,pad_p51_26,pad_p51_27,pad_p51_28,pad_p51_29,pad_p51_30;
    int scale;
    int n;
    int x;
    short mask;
    int product;
    scale = g_scale;
    /* Empty switches preserve the original VC++ 2.2 operand widths. */
    switch (scale) { case 1: break; }
    product = g_resolutionNew * scale;
    mask = ~(product - 1);
    dx &= mask;
    x = g_tempScrollX;
    n = g_maxXScroll - x;
    switch (product) { case 1: break; }
    if (n >= dx) n = dx;
    if (n <= -x) n = -x;
    if ((short)n) {
        g_tempScrollX += (short)n;
        g_tempScrollX &= mask;
        ScrollWindow(hwnd, -(short)n, 0, 0, 0);
        SetScrollPos(hwnd, SB_HORZ, g_tempScrollX, TRUE);
        SendMessage(hwnd, 0x500, 0, 0);
        UpdateWindow(hwnd);
        if (!g_replaying)
            RecordScroll(g_recordTime, g_viewW / 2 + g_tempScrollX, 0);
    }
}



extern int pad_s50_onh0,pad_s50_onh1,pad_s50_onh2,pad_s50_onh3,pad_s50_onh4,pad_s50_onh5,pad_s50_onh6,pad_s50_onh7;
/* 0x406560 */
void OnHScroll(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    short d;

    switch (LOWORD(wParam)) {
    case SB_LINELEFT:
        d = -(g_resolutionNew * g_scale * 4);
        break;
    case SB_LINERIGHT:
        d = g_resolutionNew * g_scale * 4;
        break;
    case SB_PAGELEFT:
        d = g_resolutionNew * g_scale * -40;
        break;
    case SB_PAGERIGHT:
        d = g_resolutionNew * g_scale * 40;
        break;
    case SB_THUMBPOSITION:
        d = HIWORD(wParam) - g_tempScrollX;
        break;
    case SB_THUMBTRACK:
        d = HIWORD(wParam) - g_tempScrollX;
        break;
    case SB_LEFT:
        d = -g_tempScrollX;
        break;
    case SB_RIGHT:
        d = g_maxXScroll - g_tempScrollX;
        break;
    default:
        d = 0;
        break;
    }
    ScrollTo(hwnd, d);
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_levelheight0;

extern int pad_s50_after0;
/* 0x406620 */
int LevelHeight(int zoom)
{
    return g_resolutionNew * zoom * 160;
}

extern int pad_setviewscale0, pad_setviewscale1, pad_setviewscale2, pad_setviewscale3, pad_setviewscale4, pad_setviewscale5, pad_setviewscale6, pad_setviewscale7, pad_setviewscale8, pad_setviewscale9, pad_setviewscale10, pad_setviewscale11, pad_setviewscale12, pad_setviewscale13, pad_setviewscale14, pad_setviewscale15, pad_setviewscale16, pad_setviewscale17, pad_setviewscale18, pad_setviewscale19, pad_setviewscale20, pad_setviewscale21, pad_setviewscale22, pad_setviewscale23, pad_setviewscale24, pad_setviewscale25, pad_setviewscale26, pad_setviewscale27, pad_setviewscale28, pad_setviewscale29, pad_setviewscale30, pad_setviewscale31, pad_setviewscale32, pad_setviewscale33, pad_setviewscale34, pad_setviewscale35, pad_setviewscale36, pad_setviewscale37, pad_setviewscale38, pad_setviewscale39, pad_setviewscale40, pad_setviewscale41, pad_setviewscale42, pad_setviewscale43, pad_setviewscale44, pad_setviewscale45, pad_setviewscale46, pad_setviewscale47, pad_setviewscale48, pad_setviewscale49, pad_setviewscale50, pad_setviewscale51, pad_setviewscale52, pad_setviewscale53, pad_setviewscale54, pad_setviewscale55, pad_setviewscale56, pad_setviewscale57, pad_setviewscale58, pad_setviewscale59, pad_setviewscale60, pad_setviewscale61, pad_setviewscale62, pad_setviewscale63, pad_setviewscale64, pad_setviewscale65, pad_setviewscale66, pad_setviewscale67, pad_setviewscale68, pad_setviewscale69, pad_setviewscale70, pad_setviewscale71, pad_setviewscale72, pad_setviewscale73, pad_setviewscale74, pad_setviewscale75, pad_setviewscale76, pad_setviewscale77, pad_setviewscale78, pad_setviewscale79, pad_setviewscale80, pad_setviewscale81, pad_setviewscale82, pad_setviewscale83, pad_setviewscale84, pad_setviewscale85, pad_setviewscale86, pad_setviewscale87, pad_setviewscale88, pad_setviewscale89, pad_setviewscale90, pad_setviewscale91;
extern int s64_pad0;
extern int s64_pad1;
extern int s64_pad2;
extern int s64_pad3;
extern int s64_pad4;
extern int s64_pad5;
extern int s64_pad6;
extern int s64_pad7;
extern int s64_pad8;
extern int s64_pad9;
/* 0x406640: fit the view to a zoom and the window's size */
void SetViewScale(HWND hwnd, short scale)
{
    int s;
    int levelW = g_resolutionNew * 1668;
    int levelH = g_resolutionNew * 160;
    int w;
    int h;
    int n;

    g_tempScrollX = g_tempScrollX / g_scale;
    g_valScrollY = g_valScrollY / g_scale;
    s = scale;
    g_scale = scale;
    switch (s) {
    case 1:
        g_nowRenderScale = 1;
        break;
    case 2:
    case 4:
        g_nowRenderScale = 2;
        break;
    }
    w = g_dataViewWidth / scale;
    if (w >= levelW)
        w = levelW;
    g_viewW = w;
    h = g_extraViewHeight / scale;
    if (h >= levelH)
        h = levelH;
    g_viewHLast = h;
    g_viewPixW = w * s;
    g_viewPixHGameMain = h * s;
    n = levelH - h;
    if (n >= g_valScrollY) n = g_valScrollY;
    g_valScrollY = n;
    n = levelW - w;
    if (n >= g_tempScrollX) n = g_tempScrollX;
#ifdef _MSC_VER
    __asm { mov ax, g_valScrollY }
    g_tempScrollX = n;
    __asm {
        test ax, ax
        jg skip_y
        xor ax, ax
    skip_y:
        mov g_valScrollY, ax
        mov ax, g_tempScrollX
        test ax, ax
        jg skip_x
        xor ax, ax
    skip_x:
    }
    levelW *= s;
    __asm { mov g_tempScrollX, ax }
#else
    g_tempScrollX = n;
    g_valScrollY = g_valScrollY > 0 ? g_valScrollY : 0;
    g_tempScrollX = g_tempScrollX > 0 ? g_tempScrollX : 0;
    levelW *= s;
#endif
    levelH *= s;
    levelH -= g_extraViewHeight;
    if (levelH <= 0)
        levelH = 0;
    g_yMaxScroll = levelH;
    levelW -= g_dataViewWidth;
    if (levelW <= 0)
        levelW = 0;
    g_maxXScroll = levelW;
    g_valScrollY = scale * g_valScrollY;
    g_tempScrollX = scale * g_tempScrollX;
    SetScrollRange(hwnd, SB_VERT, 0, g_yMaxScroll, FALSE);
    SetScrollPos(hwnd, SB_VERT, g_valScrollY, TRUE);
    SetScrollRange(hwnd, SB_HORZ, 0, g_maxXScroll, FALSE);
    SetScrollPos(hwnd, SB_HORZ, g_tempScrollX, TRUE);
    InvalidateRect(hwnd, 0, FALSE);
}

extern HCURSOR g_lastCursor;



extern int pad_svc_0, pad_svc_1, pad_svc_2, pad_svc_3, pad_svc_4, pad_svc_5, pad_svc_6, pad_svc_7;
/* 0x406810: the cursor over the view, with arrows when Left/Right are held */
void SetViewCursor(HCURSOR big, HCURSOR sma)
{
    HCURSOR cursor;
    BYTE keys;
    POINT pt;

    g_lastCursor = GetCursor();
    if (g_resolutionNew * g_zoom == 1)
        cursor = sma;
    else
        cursor = big;
    keys = ((GetAsyncKeyState(VK_LEFT) & 0x8000) ? 1 : 0) | ((GetAsyncKeyState(VK_RIGHT) & 0x8000) ? 2 : 0);
    if (cursor == g_hBoxCursorSpare) {
        if (keys != 3 && (keys & 1))
            cursor = g_boxHCursorLeft;
        if (keys != 3 && (keys & 2))
            cursor = g_boxCursorHRight;
    }
    if (cursor == g_hSmaBoxCursorGameNew) {
        if (keys != 3 && (keys & 1))
            cursor = g_leftHBoxSmaCursor;
        if (keys != 3 && (keys & 2))
            cursor = g_hBoxCursorRightSma;
    }
    if (cursor && g_lastCursor != cursor) {
        g_viewCursor = cursor;
        GetCursorPos(&pt);
        if (WindowFromPoint(pt) == g_hwndView)
            SetCursor(cursor);
    }
    g_lastCursor = cursor;
}

/* 0x406900: the status bar while no menu item is highlighted */
void ShowGameStatus(void)
{
    SetStatusMessage(g_helpMenu);
    SetStatusHome(GetLemmsHome());
    SetStatusOut(0);
    SetStatusTime(g_timeMinData, g_timeSec);
}
