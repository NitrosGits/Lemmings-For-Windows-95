/*
 * Dialogs: the templates of lemmings.rc become a top-level window of class
 * #32770 with its controls; DialogBoxParam runs the modal loop with the owner
 * disabled. Dialog units come from the dialog font's base units, as Windows
 * computes them for MS Sans Serif (6 x 13 pixels at 8 points, 7 x 16 at 10).
 */
#include "port_internal.h"
#include "port_res.h"

typedef struct Dlg {
    DLGPROC proc;
    INT_PTR result;
    int ended;
    HFONT font;
    int bx, by;
    HWND lastFocus;
} Dlg;

int wnd_is_dialog(Wnd *w)
{
    return w && w->dlg != NULL;
}

void dlg_units(HFONT font, int *bx, int *by)
{
    PFont *f = font_get(font);

    if (f->height <= 13) {
        *bx = 6;
        *by = 13;
    } else if (f->height <= 16) {
        *bx = 7;
        *by = 16;
    } else {
        *bx = font_avg_width(f);
        *by = f->height;
    }
}

static const PortResDialog *find_template(LPCSTR name)
{
    const PortResDialog *d;

    for (d = port_res_dialogs; d->controls; d++) {
        if (IS_INTRESOURCE(name) ? d->id == (int)(uintptr_t)name : (d->name && !SDL_strcasecmp(d->name, name)))
            return d;
    }
    return NULL;
}

static Wnd *first_tabstop(Wnd *dlg)
{
    Wnd *c;

    for (c = dlg->child; c; c = c->next)
        if ((c->style & WS_TABSTOP) && (c->style & WS_VISIBLE) && !(c->style & WS_DISABLED))
            return c;
    return NULL;
}

static LRESULT CALLBACK DlgWndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    Wnd *w = wnd_get(h);
    Dlg *d = w ? w->dlg : NULL;
    BOOL r = FALSE;

    if (!w)
        return 0;
    if (d && d->proc && msg != WM_NCCREATE && msg != WM_CREATE)
        r = d->proc(h, msg, wp, lp);
    if (!wnd_get(h))
        return r;
    switch (msg) {
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        /* a dialog procedure returns the brush (a 32-bit handle) */
        if (r)
            return (LRESULT)(uint32_t)r;
        return DefWindowProcA(h, msg, wp, lp);
    case WM_CHARTOITEM:
    case WM_VKEYTOITEM:
    case WM_INITDIALOG:
        return r;
    }
    if (r)
        return r;
    switch (msg) {
    case WM_ERASEBKGND:
        {
            RECT rc;
            LRESULT br = SendMessageA(h, WM_CTLCOLORDLG, wp, (LPARAM)h);

            GetClientRect(h, &rc);
            FillRect((HDC)wp, &rc, br ? (HBRUSH)(uintptr_t)(uint32_t)br : (HBRUSH)(uintptr_t)(COLOR_BTNFACE + 1));
        }
        return 1;
    case WM_CLOSE:
        {
            HWND cancel = GetDlgItem(h, IDCANCEL);

            if (!cancel || IsWindowEnabled(cancel))
                PostMessageA(h, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)cancel);
        }
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wp) && d) {
            Wnd *f = wnd_get(d->lastFocus);

            if (!f || wnd_toplevel(f) != w)
                f = first_tabstop(w);
            if (f)
                SetFocus(f->h);
        }
        return 0;
    case WM_SETFOCUS:
        {
            Wnd *f = first_tabstop(w);

            if (f)
                SetFocus(f->h);
        }
        return 0;
    }
    return DefWindowProcA(h, msg, wp, lp);
}

BOOL WINAPI EndDialog(HWND h, INT_PTR result)
{
    Wnd *w = wnd_get(h);

    if (!w || !w->dlg)
        return FALSE;
    w->dlg->result = result;
    w->dlg->ended = 1;
    return TRUE;
}

INT_PTR WINAPI DialogBoxParamA(HINSTANCE inst, LPCSTR name, HWND parent, DLGPROC proc, LPARAM param)
{
    const PortResDialog *t = find_template(name);
    Wnd *owner = parent ? wnd_get(parent) : NULL, *w;
    Dlg *d;
    HWND h;
    int i, x, y, cw, ch, ownerWasEnabled = 0;
    DWORD style;
    MSG msg;
    INT_PTR result;

    if (!t) {
        port_trace("DialogBoxParam: no template");
        return -1;
    }
    if (owner) {
        owner = wnd_toplevel(owner);
        if (owner == port_desktop)
            owner = NULL;
    }
    d = calloc(1, sizeof *d);
    d->proc = proc;
    if (t->style & DS_SETFONT) {
        HDC hdc = GetDC(NULL);

        d->font = CreateFontA(-MulDiv(t->fontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0,
                              !SDL_strcasecmp(t->font, "System") ? FW_BOLD : FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0,
                              t->font);
        ReleaseDC(NULL, hdc);
    } else
        d->font = font_default();
    dlg_units(d->font, &d->bx, &d->by);
    cw = MulDiv(t->cx, d->bx, 4);
    ch = MulDiv(t->cy, d->by, 8);
    style = (t->style & ~(DS_SETFONT | DS_MODALFRAME | DS_SYSMODAL | WS_VISIBLE)) | WS_POPUP;
    /* the template's position is in the owner's client area; at the top left, centre it */
    if (owner && (t->x || t->y)) {
        POINT o = {0, 0};

        ClientToScreen(owner->h, &o);
        x = o.x + MulDiv(t->x, d->bx, 4);
        y = o.y + MulDiv(t->y, d->by, 8);
    } else {
        Wnd *c = owner ? owner : port_desktop;

        x = c->rc.left + (c->rc.right - c->rc.left - cw) / 2;
        y = c->rc.top + (c->rc.bottom - c->rc.top - ch) / 3;
    }
    if (x < port_desktop->rc.left)
        x = port_desktop->rc.left;
    if (y < port_desktop->rc.top)
        y = port_desktop->rc.top;
    h = CreateWindowExA(WS_EX_DLGMODALFRAME, "#32770", t->caption ? t->caption : "", style, x, y, cw, ch,
                        owner ? owner->h : NULL, NULL, inst, NULL);
    w = wnd_get(h);
    if (!w) {
        free(d);
        return -1;
    }
    w->dlg = d;
    w->font = d->font;
    /* as Windows' dialog manager: the dialog procedure hears of its font before the controls exist */
    if (t->style & DS_SETFONT)
        SendMessageA(h, WM_SETFONT, (WPARAM)d->font, 0);
    for (i = 0; i < t->count; i++) {
        const PortResControl *c = &t->controls[i];
        HWND ch2 = CreateWindowExA(0, c->cls, c->text ? c->text : "", c->style | WS_CHILD,
                                   MulDiv(c->x, d->bx, 4), MulDiv(c->y, d->by, 8), MulDiv(c->cx, d->bx, 4),
                                   MulDiv(c->cy, d->by, 8), h, (HMENU)(intptr_t)c->id, inst, NULL);

        if (ch2)
            SendMessageA(ch2, WM_SETFONT, (WPARAM)d->font, 0);
        else
            port_trace("dialog control class %s failed", c->cls);
    }
    {
        Wnd *f = first_tabstop(w);

        if (SendMessageA(h, WM_INITDIALOG, (WPARAM)(f ? f->h : NULL), param) && wnd_get(h)) {
            f = first_tabstop(w);
            if (f)
                SetFocus(f->h);
        }
    }
    if (!wnd_get(h)) {
        free(d);
        return -1;
    }
    if (owner) {
        ownerWasEnabled = !(owner->style & WS_DISABLED);
        EnableWindow(owner->h, FALSE);
    }
    if (!d->ended)
        ShowWindow(h, SW_SHOW);
    port_modal_depth++;
    while (!d->ended) {
        if (!GetMessageA(&msg, NULL, 0, 0)) {
            PostQuitMessage((int)msg.wParam);
            break;
        }
        if (!dlg_is_dialog_message(w, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        if (!wnd_get(h))
            break;
    }
    port_modal_depth--;
    result = d->result;
    if (owner && ownerWasEnabled && wnd_get(owner->h))
        EnableWindow(owner->h, TRUE);
    if (wnd_get(h))
        DestroyWindow(h);
    if (owner && wnd_get(owner->h)) {
        port_active = owner;
        if (owner->sdl)
            SDL_RaiseWindow(owner->sdl);
    }
    if (d->font != font_default())
        DeleteObject(d->font);
    free(d);
    port_trace("dialog result %d", (int)result);
    return result;
}

static Wnd *next_tab(Wnd *dlg, Wnd *from, int back)
{
    Wnd *c = from, *start = from;
    int guard = 0;

    if (!dlg->child)
        return NULL;
    do {
        if (back) {
            if (!c || !c->prev) {
                for (c = dlg->child; c->next; c = c->next)
                    ;
            } else
                c = c->prev;
        } else
            c = c && c->next ? c->next : dlg->child;
        if ((c->style & WS_TABSTOP) && (c->style & WS_VISIBLE) && !(c->style & WS_DISABLED))
            return c;
    } while (c != start && ++guard < 256);
    return NULL;
}

static Wnd *default_button(Wnd *dlg)
{
    Wnd *c;

    for (c = dlg->child; c; c = c->next)
        if (!SDL_strcasecmp(c->cls->name, "Button") && (c->style & 0xf) == BS_DEFPUSHBUTTON &&
            !(c->style & WS_DISABLED))
            return c;
    return NULL;
}

int dlg_is_dialog_message(Wnd *dlg, MSG *msg)
{
    Wnd *t = wnd_get(msg->hwnd);
    LRESULT code;

    if (!t || wnd_toplevel(t) != dlg)
        return 0;
    if (msg->message != WM_KEYDOWN && msg->message != WM_CHAR)
        return 0;
    code = SendMessageA(t->h, WM_GETDLGCODE, msg->wParam, (LPARAM)msg);
    if (code & DLGC_WANTALLKEYS)
        return 0;
    if (msg->message == WM_CHAR) {
        /* Enter, Escape and Tab were handled at WM_KEYDOWN */
        return msg->wParam == 13 || msg->wParam == 27 || msg->wParam == 9;
    }
    switch (msg->wParam) {
    case VK_TAB:
        if (code & DLGC_WANTTAB)
            return 0;
        {
            Wnd *n = next_tab(dlg, t == dlg ? NULL : t, port_key_down(VK_SHIFT));

            if (n) {
                SetFocus(n->h);
                if (!SDL_strcasecmp(n->cls->name, "Edit"))
                    SendMessageA(n->h, EM_SETSEL, 0, -1);
            }
        }
        return 1;
    case VK_RETURN:
        {
            Wnd *b = (!SDL_strcasecmp(t->cls->name, "Button") && (t->style & 0xf) <= BS_DEFPUSHBUTTON) ? t
                                                                                                         : default_button(dlg);
            int id = b ? b->id : IDOK;

            SendMessageA(dlg->h, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)(b ? b->h : NULL));
        }
        return 1;
    case VK_ESCAPE:
        {
            HWND cancel = GetDlgItem(dlg->h, IDCANCEL);

            SendMessageA(dlg->h, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)cancel);
        }
        return 1;
    }
    return 0;
}

HWND WINAPI GetDlgItem(HWND h, int id)
{
    Wnd *w = wnd_get(h), *c;

    if (!w)
        return NULL;
    for (c = w->child; c; c = c->next)
        if (c->id == id)
            return c->h;
    return NULL;
}

int WINAPI GetDlgCtrlID(HWND h)
{
    Wnd *w = wnd_get(h);

    return w ? w->id : 0;
}

UINT WINAPI GetDlgItemTextA(HWND h, int id, LPSTR buf, int size)
{
    return (UINT)GetWindowTextA(GetDlgItem(h, id), buf, size);
}

BOOL WINAPI SetDlgItemTextA(HWND h, int id, LPCSTR text)
{
    return SetWindowTextA(GetDlgItem(h, id), text);
}

UINT WINAPI GetDlgItemInt(HWND h, int id, BOOL *ok, BOOL sign)
{
    char buf[32];

    (void)sign;
    GetDlgItemTextA(h, id, buf, sizeof buf);
    if (ok)
        *ok = TRUE;
    return (UINT)atoi(buf);
}

BOOL WINAPI SetDlgItemInt(HWND h, int id, UINT value, BOOL sign)
{
    char buf[32];

    snprintf(buf, sizeof buf, sign ? "%d" : "%u", value);
    return SetDlgItemTextA(h, id, buf);
}

LRESULT WINAPI SendDlgItemMessageA(HWND h, int id, UINT msg, WPARAM wp, LPARAM lp)
{
    return SendMessageA(GetDlgItem(h, id), msg, wp, lp);
}

BOOL WINAPI CheckDlgButton(HWND h, int id, UINT check)
{
    SendMessageA(GetDlgItem(h, id), BM_SETCHECK, check, 0);
    return TRUE;
}

UINT WINAPI IsDlgButtonChecked(HWND h, int id)
{
    return (UINT)SendMessageA(GetDlgItem(h, id), BM_GETCHECK, 0, 0);
}

BOOL WINAPI CheckRadioButton(HWND h, int first, int last, int check)
{
    int id;

    for (id = first; id <= last; id++)
        SendMessageA(GetDlgItem(h, id), BM_SETCHECK, id == check, 0);
    return TRUE;
}

void dlg_init(void)
{
    wnd_register_builtin("#32770", DlgWndProc, CS_DBLCLKS | CS_SAVEBITS, 32, 1);
}
