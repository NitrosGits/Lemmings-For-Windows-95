/*
 * The part of the Win32 API that Lemmings for Windows 95 uses, for the native
 * SDL3 build (PORTING.md). Types have their Win64 sizes: BYTE/WORD/DWORD/LONG/
 * UINT/BOOL are 8/16/32/32/32/32 bits, WPARAM/LPARAM/LRESULT and the *_PTR
 * types are as wide as a pointer. Structures the game reads from files have
 * the SDK's exact layout. Constants have the SDK's values (the resource
 * templates and the registry store some of them).
 *
 * Only what the game needs is here: a missing name is a compile error, which
 * keeps PORTING.md's inventory honest.
 */
#ifndef PORT_WINDOWS_H
#define PORT_WINDOWS_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Calling conventions: nothing on 64-bit ---- */
#define WINAPI
#define WINAPIV
#define CALLBACK
#define PASCAL
#define APIENTRY
#define FAR
#define NEAR
#define CONST const
#define VOID void

/* ---- Basic types ---- */
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef unsigned int UINT;
typedef int INT;
typedef int BOOL;
typedef short SHORT;
typedef unsigned short USHORT;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef intptr_t INT_PTR, LONG_PTR;
typedef uintptr_t UINT_PTR, ULONG_PTR, DWORD_PTR;
typedef size_t SIZE_T;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef DWORD COLORREF;
typedef WORD ATOM;

typedef char *LPSTR, *PSTR, *HPSTR;
typedef const char *LPCSTR, *PCSTR;
typedef void *LPVOID, *PVOID;
typedef const void *LPCVOID;
typedef BYTE *LPBYTE, *PBYTE;
typedef WORD *LPWORD;
typedef DWORD *LPDWORD, *PDWORD;
typedef LONG *LPLONG;
typedef BOOL *LPBOOL;
typedef int *LPINT;
typedef UINT *LPUINT, *PUINT;
typedef COLORREF *LPCOLORREF;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define MAX_PATH 260

/* ---- Handles: pointers to the layer's objects ---- */
typedef void *HANDLE;
typedef void *HGDIOBJ;
#define PORT_DECLARE_HANDLE(name) typedef struct name##__ *name
PORT_DECLARE_HANDLE(HWND);
PORT_DECLARE_HANDLE(HDC);
PORT_DECLARE_HANDLE(HBITMAP);
PORT_DECLARE_HANDLE(HBRUSH);
PORT_DECLARE_HANDLE(HPEN);
PORT_DECLARE_HANDLE(HFONT);
PORT_DECLARE_HANDLE(HPALETTE);
PORT_DECLARE_HANDLE(HRGN);
PORT_DECLARE_HANDLE(HMENU);
PORT_DECLARE_HANDLE(HICON);
PORT_DECLARE_HANDLE(HACCEL);
PORT_DECLARE_HANDLE(HINSTANCE);
PORT_DECLARE_HANDLE(HKEY);
PORT_DECLARE_HANDLE(HRSRC);
PORT_DECLARE_HANDLE(HWAVEOUT);
PORT_DECLARE_HANDLE(HMMIO);
typedef HICON HCURSOR;
typedef HINSTANCE HMODULE;
typedef HANDLE HGLOBAL;
typedef HANDLE HLOCAL;
typedef int HFILE;
typedef HKEY *PHKEY;
#define HFILE_ERROR ((HFILE)-1)
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

/* ---- Macros ---- */
#define LOWORD(l) ((WORD)(((DWORD_PTR)(l)) & 0xffff))
#define HIWORD(l) ((WORD)((((DWORD_PTR)(l)) >> 16) & 0xffff))
#define LOBYTE(w) ((BYTE)(((DWORD_PTR)(w)) & 0xff))
#define HIBYTE(w) ((BYTE)((((DWORD_PTR)(w)) >> 8) & 0xff))
#define MAKEWORD(a, b) ((WORD)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8))
#define MAKELONG(a, b) ((LONG)(((WORD)(((DWORD_PTR)(a)) & 0xffff)) | ((DWORD)((WORD)(((DWORD_PTR)(b)) & 0xffff))) << 16))
#define MAKEWPARAM(l, h) ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELPARAM(l, h) ((LPARAM)(DWORD)MAKELONG(l, h))
#define MAKELRESULT(l, h) ((LRESULT)(DWORD)MAKELONG(l, h))
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(rgb) (LOBYTE(rgb))
#define GetGValue(rgb) (LOBYTE(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) (LOBYTE((rgb) >> 16))
#define PALETTERGB(r, g, b) (0x02000000 | RGB(r, g, b))
#define PALETTEINDEX(i) ((COLORREF)(0x01000000 | (DWORD)(WORD)(i)))
#define MAKEINTRESOURCEA(i) ((LPSTR)((ULONG_PTR)((WORD)(i))))
#define MAKEINTRESOURCE MAKEINTRESOURCEA
#define IS_INTRESOURCE(p) ((((ULONG_PTR)(p)) >> 16) == 0)

/* ---- Structures ---- */
typedef struct tagPOINT {
    LONG x, y;
} POINT, *PPOINT, *LPPOINT;

typedef struct tagSIZE {
    LONG cx, cy;
} SIZE, *PSIZE, *LPSIZE;

typedef struct tagRECT {
    LONG left, top, right, bottom;
} RECT, *PRECT, *LPRECT;
typedef const RECT *LPCRECT;

typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
} MSG, *PMSG, *LPMSG;

typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL (CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void (CALLBACK *TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);

typedef struct tagWNDCLASSA {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
} WNDCLASSA, WNDCLASS, *LPWNDCLASS;

typedef struct tagCREATESTRUCTA {
    LPVOID lpCreateParams;
    HINSTANCE hInstance;
    HMENU hMenu;
    HWND hwndParent;
    int cy, cx, y, x;
    LONG style;
    LPCSTR lpszName;
    LPCSTR lpszClass;
    DWORD dwExStyle;
} CREATESTRUCTA, CREATESTRUCT, *LPCREATESTRUCT;

typedef struct tagPAINTSTRUCT {
    HDC hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

typedef struct tagMINMAXINFO {
    POINT ptReserved;
    POINT ptMaxSize;
    POINT ptMaxPosition;
    POINT ptMinTrackSize;
    POINT ptMaxTrackSize;
} MINMAXINFO, *LPMINMAXINFO;

typedef struct tagDRAWITEMSTRUCT {
    UINT CtlType;
    UINT CtlID;
    UINT itemID;
    UINT itemAction;
    UINT itemState;
    HWND hwndItem;
    HDC hDC;
    RECT rcItem;
    ULONG_PTR itemData;
} DRAWITEMSTRUCT, *LPDRAWITEMSTRUCT;

typedef struct tagMEASUREITEMSTRUCT {
    UINT CtlType;
    UINT CtlID;
    UINT itemID;
    UINT itemWidth;
    UINT itemHeight;
    ULONG_PTR itemData;
} MEASUREITEMSTRUCT, *LPMEASUREITEMSTRUCT;

typedef struct tagACCEL {
    BYTE fVirt;
    WORD key;
    WORD cmd;
} ACCEL, *LPACCEL;

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME;

typedef struct _MEMORYSTATUS {  /* DWORDs, as in the 1995 SDK */
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORD dwTotalPhys;
    DWORD dwAvailPhys;
    DWORD dwTotalPageFile;
    DWORD dwAvailPageFile;
    DWORD dwTotalVirtual;
    DWORD dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

typedef struct _OFSTRUCT {
    BYTE cBytes;
    BYTE fFixedDisk;
    WORD nErrCode;
    WORD Reserved1;
    WORD Reserved2;
    CHAR szPathName[128];
} OFSTRUCT, *LPOFSTRUCT;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

/* GDI structures (the bitmap headers are read from .BMP files) */
#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER, *LPBITMAPFILEHEADER;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagBITMAPCOREHEADER {
    DWORD bcSize;
    WORD bcWidth;
    WORD bcHeight;
    WORD bcPlanes;
    WORD bcBitCount;
} BITMAPCOREHEADER, *LPBITMAPCOREHEADER;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD, *LPRGBQUAD;

typedef struct tagRGBTRIPLE {
    BYTE rgbtBlue;
    BYTE rgbtGreen;
    BYTE rgbtRed;
} RGBTRIPLE;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;

typedef struct tagBITMAP {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    LPVOID bmBits;
} BITMAP, *LPBITMAP;

typedef struct tagPALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagLOGPALETTE {
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[1];
} LOGPALETTE, *LPLOGPALETTE;

typedef struct tagLOGBRUSH {
    UINT lbStyle;
    COLORREF lbColor;
    ULONG_PTR lbHatch;
} LOGBRUSH, *LPLOGBRUSH;

typedef struct tagTEXTMETRICA {
    LONG tmHeight;
    LONG tmAscent;
    LONG tmDescent;
    LONG tmInternalLeading;
    LONG tmExternalLeading;
    LONG tmAveCharWidth;
    LONG tmMaxCharWidth;
    LONG tmWeight;
    LONG tmOverhang;
    LONG tmDigitizedAspectX;
    LONG tmDigitizedAspectY;
    BYTE tmFirstChar;
    BYTE tmLastChar;
    BYTE tmDefaultChar;
    BYTE tmBreakChar;
    BYTE tmItalic;
    BYTE tmUnderlined;
    BYTE tmStruckOut;
    BYTE tmPitchAndFamily;
    BYTE tmCharSet;
} TEXTMETRICA, TEXTMETRIC, *LPTEXTMETRIC;

/* ---- Window styles ---- */
#define WS_OVERLAPPED       0x00000000L
#define WS_POPUP            0x80000000L
#define WS_CHILD            0x40000000L
#define WS_MINIMIZE         0x20000000L
#define WS_VISIBLE          0x10000000L
#define WS_DISABLED         0x08000000L
#define WS_CLIPSIBLINGS     0x04000000L
#define WS_CLIPCHILDREN     0x02000000L
#define WS_MAXIMIZE         0x01000000L
#define WS_CAPTION          0x00C00000L
#define WS_BORDER           0x00800000L
#define WS_DLGFRAME         0x00400000L
#define WS_VSCROLL          0x00200000L
#define WS_HSCROLL          0x00100000L
#define WS_SYSMENU          0x00080000L
#define WS_THICKFRAME       0x00040000L
#define WS_GROUP            0x00020000L
#define WS_TABSTOP          0x00010000L
#define WS_MINIMIZEBOX      0x00020000L
#define WS_MAXIMIZEBOX      0x00010000L
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW      (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_EX_DLGMODALFRAME 0x00000001L
#define WS_EX_TOPMOST       0x00000008L
#define WS_EX_TOOLWINDOW    0x00000080L
#define DS_SYSMODAL         0x02L
#define DS_SETFONT          0x40L
#define DS_MODALFRAME       0x80L
#define CW_USEDEFAULT       ((int)0x80000000)

/* Class styles */
#define CS_VREDRAW  0x0001
#define CS_HREDRAW  0x0002
#define CS_DBLCLKS  0x0008
#define CS_OWNDC    0x0020
#define CS_CLASSDC  0x0040
#define CS_PARENTDC 0x0080
#define CS_SAVEBITS 0x0800

/* Control styles */
#define BS_PUSHBUTTON      0x00L
#define BS_DEFPUSHBUTTON   0x01L
#define BS_CHECKBOX        0x02L
#define BS_AUTOCHECKBOX    0x03L
#define BS_RADIOBUTTON     0x04L
#define BS_3STATE          0x05L
#define BS_AUTO3STATE      0x06L
#define BS_GROUPBOX        0x07L
#define BS_USERBUTTON      0x08L
#define BS_AUTORADIOBUTTON 0x09L
#define BS_OWNERDRAW       0x0BL
#define BS_LEFTTEXT        0x20L
#define SS_LEFT            0x00L
#define SS_CENTER          0x01L
#define SS_RIGHT           0x02L
#define SS_ICON            0x03L
#define SS_BLACKRECT       0x04L
#define SS_GRAYRECT        0x05L
#define SS_WHITERECT       0x06L
#define SS_BLACKFRAME      0x07L
#define SS_GRAYFRAME       0x08L
#define SS_WHITEFRAME      0x09L
#define SS_SIMPLE          0x0BL
#define SS_LEFTNOWORDWRAP  0x0CL
#define SS_NOPREFIX        0x80L
#define ES_LEFT            0x00L
#define ES_AUTOHSCROLL     0x80L
#define LBS_NOTIFY            0x0001L
#define LBS_SORT              0x0002L
#define LBS_NOREDRAW          0x0004L
#define LBS_MULTIPLESEL       0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL       0x0800L
#define LBS_DISABLENOSCROLL   0x1000L

/* ---- Messages ---- */
#define WM_NULL             0x0000
#define WM_CREATE           0x0001
#define WM_DESTROY          0x0002
#define WM_MOVE             0x0003
#define WM_SIZE             0x0005
#define WM_ACTIVATE         0x0006
#define WM_SETFOCUS         0x0007
#define WM_KILLFOCUS        0x0008
#define WM_ENABLE           0x000A
#define WM_SETREDRAW        0x000B
#define WM_SETTEXT          0x000C
#define WM_GETTEXT          0x000D
#define WM_GETTEXTLENGTH    0x000E
#define WM_PAINT            0x000F
#define WM_CLOSE            0x0010
#define WM_QUERYENDSESSION  0x0011
#define WM_QUIT             0x0012
#define WM_ERASEBKGND       0x0014
#define WM_SYSCOLORCHANGE   0x0015
#define WM_ENDSESSION       0x0016
#define WM_SHOWWINDOW       0x0018
#define WM_ACTIVATEAPP      0x001C
#define WM_SETCURSOR        0x0020
#define WM_MOUSEACTIVATE    0x0021
#define WM_GETMINMAXINFO    0x0024
#define WM_DRAWITEM         0x002B
#define WM_MEASUREITEM      0x002C
#define WM_DELETEITEM       0x002D
#define WM_VKEYTOITEM       0x002E
#define WM_CHARTOITEM       0x002F
#define WM_SETFONT          0x0030
#define WM_GETFONT          0x0031
#define WM_WINDOWPOSCHANGED 0x0047
#define WM_NCCREATE         0x0081
#define WM_NCDESTROY        0x0082
#define WM_NCCALCSIZE       0x0083
#define WM_NCHITTEST        0x0084
#define WM_NCPAINT          0x0085
#define WM_NCACTIVATE       0x0086
#define WM_GETDLGCODE       0x0087
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_SYSKEYDOWN       0x0104
#define WM_SYSKEYUP         0x0105
#define WM_SYSCHAR          0x0106
#define WM_INITDIALOG       0x0110
#define WM_COMMAND          0x0111
#define WM_SYSCOMMAND       0x0112
#define WM_TIMER            0x0113
#define WM_HSCROLL          0x0114
#define WM_VSCROLL          0x0115
#define WM_INITMENU         0x0116
#define WM_INITMENUPOPUP    0x0117
#define WM_MENUSELECT       0x011F
#define WM_ENTERIDLE        0x0121
#define WM_CTLCOLORMSGBOX   0x0132
#define WM_CTLCOLOREDIT     0x0133
#define WM_CTLCOLORLISTBOX  0x0134
#define WM_CTLCOLORBTN      0x0135
#define WM_CTLCOLORDLG      0x0136
#define WM_CTLCOLORSCROLLBAR 0x0137
#define WM_CTLCOLORSTATIC   0x0138
#define WM_MOUSEMOVE        0x0200
#define WM_LBUTTONDOWN      0x0201
#define WM_LBUTTONUP        0x0202
#define WM_LBUTTONDBLCLK    0x0203
#define WM_RBUTTONDOWN      0x0204
#define WM_RBUTTONUP        0x0205
#define WM_RBUTTONDBLCLK    0x0206
#define WM_MBUTTONDOWN      0x0207
#define WM_MBUTTONUP        0x0208
#define WM_MBUTTONDBLCLK    0x0209
#define WM_CAPTURECHANGED   0x0215
#define WM_QUERYNEWPALETTE  0x030F
#define WM_PALETTEISCHANGING 0x0310
#define WM_PALETTECHANGED   0x0311
#define WM_USER             0x0400
#define WM_APP              0x8000

/* Control messages and notifications */
#define BM_GETCHECK  0x00F0
#define BM_SETCHECK  0x00F1
#define BM_GETSTATE  0x00F2
#define BM_SETSTATE  0x00F3
#define BM_SETSTYLE  0x00F4
#define BM_CLICK     0x00F5
#define BN_CLICKED   0
#define BN_DOUBLECLICKED 5
#define BST_UNCHECKED 0
#define BST_CHECKED   1
#define EM_GETSEL    0x00B0
#define EM_SETSEL    0x00B1
#define EM_LIMITTEXT 0x00C5
#define EN_SETFOCUS  0x0100
#define EN_KILLFOCUS 0x0200
#define EN_CHANGE    0x0300
#define EN_UPDATE    0x0400
#define STM_SETICON  0x0170
#define LB_ADDSTRING     0x0180
#define LB_INSERTSTRING  0x0181
#define LB_DELETESTRING  0x0182
#define LB_RESETCONTENT  0x0184
#define LB_SETSEL        0x0185
#define LB_SETCURSEL     0x0186
#define LB_GETSEL        0x0187
#define LB_GETCURSEL     0x0188
#define LB_GETTEXT       0x0189
#define LB_GETTEXTLEN    0x018A
#define LB_GETCOUNT      0x018B
#define LB_SELECTSTRING  0x018C
#define LB_GETTOPINDEX   0x018E
#define LB_FINDSTRING    0x018F
#define LB_SETTOPINDEX   0x0197
#define LB_GETITEMRECT   0x0198
#define LB_GETITEMDATA   0x0199
#define LB_SETITEMDATA   0x019A
#define LB_SETCARETINDEX 0x019E
#define LB_GETCARETINDEX 0x019F
#define LB_SETITEMHEIGHT 0x01A0
#define LB_GETITEMHEIGHT 0x01A1
#define LB_FINDSTRINGEXACT 0x01A2
#define LB_OKAY      0
#define LB_ERR       (-1)
#define LB_ERRSPACE  (-2)
#define LBN_ERRSPACE (-2)
#define LBN_SELCHANGE 1
#define LBN_DBLCLK    2
#define LBN_SELCANCEL 3
#define LBN_SETFOCUS  4
#define LBN_KILLFOCUS 5
#define DLGC_WANTARROWS 0x0001
#define DLGC_WANTTAB    0x0002
#define DLGC_WANTALLKEYS 0x0004
#define DLGC_HASSETSEL  0x0008
#define DLGC_DEFPUSHBUTTON 0x0010
#define DLGC_UNDEFPUSHBUTTON 0x0020
#define DLGC_RADIOBUTTON 0x0040
#define DLGC_WANTCHARS  0x0080
#define DLGC_STATIC     0x0100
#define DLGC_BUTTON     0x2000

/* Owner drawing */
#define ODT_MENU     1
#define ODT_LISTBOX  2
#define ODT_COMBOBOX 3
#define ODT_BUTTON   4
#define ODT_STATIC   5
#define ODA_DRAWENTIRE 0x0001
#define ODA_SELECT     0x0002
#define ODA_FOCUS      0x0004
#define ODS_SELECTED 0x0001
#define ODS_GRAYED   0x0002
#define ODS_DISABLED 0x0004
#define ODS_CHECKED  0x0008
#define ODS_FOCUS    0x0010

/* Scroll bars */
#define SB_HORZ 0
#define SB_VERT 1
#define SB_CTL  2
#define SB_BOTH 3
#define SB_LINEUP        0
#define SB_LINELEFT      0
#define SB_LINEDOWN      1
#define SB_LINERIGHT     1
#define SB_PAGEUP        2
#define SB_PAGELEFT      2
#define SB_PAGEDOWN      3
#define SB_PAGERIGHT     3
#define SB_THUMBPOSITION 4
#define SB_THUMBTRACK    5
#define SB_TOP           6
#define SB_LEFT          6
#define SB_BOTTOM        7
#define SB_RIGHT         7
#define SB_ENDSCROLL     8

/* SetWindowPos, ShowWindow, WM_SIZE */
#define SWP_NOSIZE       0x0001
#define SWP_NOMOVE       0x0002
#define SWP_NOZORDER     0x0004
#define SWP_NOREDRAW     0x0008
#define SWP_NOACTIVATE   0x0010
#define SWP_FRAMECHANGED 0x0020
#define SWP_SHOWWINDOW   0x0040
#define SWP_HIDEWINDOW   0x0080
#define HWND_TOP       ((HWND)0)
#define HWND_TOPMOST   ((HWND)(LONG_PTR)-1)
#define HWND_NOTOPMOST ((HWND)(LONG_PTR)-2)
#define SW_HIDE            0
#define SW_SHOWNORMAL      1
#define SW_NORMAL          1
#define SW_SHOWMINIMIZED   2
#define SW_SHOWMAXIMIZED   3
#define SW_MAXIMIZE        3
#define SW_SHOWNOACTIVATE  4
#define SW_SHOW            5
#define SW_MINIMIZE        6
#define SW_SHOWMINNOACTIVE 7
#define SW_SHOWNA          8
#define SW_RESTORE         9
#define SW_SHOWDEFAULT     10
#define SIZE_RESTORED  0
#define SIZE_MINIMIZED 1
#define SIZE_MAXIMIZED 2

/* Window longs and relations */
#define GWL_WNDPROC    (-4)
#define GWL_HINSTANCE  (-6)
#define GWL_HWNDPARENT (-8)
#define GWL_ID         (-12)
#define GWL_STYLE      (-16)
#define GWL_EXSTYLE    (-20)
#define GWL_USERDATA   (-21)
#define GWLP_WNDPROC   (-4)
#define GWLP_ID        (-12)
#define GWLP_USERDATA  (-21)
#define DWL_MSGRESULT  0
#define GW_HWNDFIRST 0
#define GW_HWNDLAST  1
#define GW_HWNDNEXT  2
#define GW_HWNDPREV  3
#define GW_OWNER     4
#define GW_CHILD     5

/* Hit tests, mouse and keys */
#define HTERROR       (-2)
#define HTTRANSPARENT (-1)
#define HTNOWHERE     0
#define HTCLIENT      1
#define HTCAPTION     2
#define HTSYSMENU     3
#define HTMENU        5
#define HTHSCROLL     6
#define HTVSCROLL     7
#define HTBORDER      18
#define MK_LBUTTON 0x0001
#define MK_RBUTTON 0x0002
#define MK_SHIFT   0x0004
#define MK_CONTROL 0x0008
#define MK_MBUTTON 0x0010
#define VK_LBUTTON  0x01
#define VK_RBUTTON  0x02
#define VK_CANCEL   0x03
#define VK_MBUTTON  0x04
#define VK_BACK     0x08
#define VK_TAB      0x09
#define VK_RETURN   0x0D
#define VK_SHIFT    0x10
#define VK_CONTROL  0x11
#define VK_MENU     0x12
#define VK_PAUSE    0x13
#define VK_ESCAPE   0x1B
#define VK_SPACE    0x20
#define VK_PRIOR    0x21
#define VK_NEXT     0x22
#define VK_END      0x23
#define VK_HOME     0x24
#define VK_LEFT     0x25
#define VK_UP       0x26
#define VK_RIGHT    0x27
#define VK_DOWN     0x28
#define VK_INSERT   0x2D
#define VK_DELETE   0x2E
#define VK_NUMPAD0  0x60
#define VK_NUMPAD5  0x65
#define VK_NUMPAD9  0x69
#define VK_MULTIPLY 0x6A
#define VK_ADD      0x6B
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL  0x6E
#define VK_DIVIDE   0x6F
#define VK_F1       0x70
#define VK_F10      0x79
#define VK_F12      0x7B
#define FVIRTKEY  0x01
#define FNOINVERT 0x02
#define FSHIFT    0x04
#define FCONTROL  0x08
#define FALT      0x10
#define PM_NOREMOVE 0x0000
#define PM_REMOVE   0x0001
#define SC_SIZE     0xF000
#define SC_MINIMIZE 0xF020
#define SC_MAXIMIZE 0xF030
#define SC_CLOSE    0xF060
#define SC_KEYMENU  0xF100
#define SC_RESTORE  0xF120

/* Menus */
#define MF_ENABLED    0x0000
#define MF_GRAYED     0x0001
#define MF_DISABLED   0x0002
#define MF_BITMAP     0x0004
#define MF_CHECKED    0x0008
#define MF_POPUP      0x0010
#define MF_HILITE     0x0080
#define MF_BYCOMMAND  0x0000
#define MF_BYPOSITION 0x0400
#define MF_SEPARATOR  0x0800
#define MF_SYSMENU    0x2000
#define MF_MOUSESELECT 0x8000

/* System colours and metrics */
#define COLOR_SCROLLBAR       0
#define COLOR_BACKGROUND      1
#define COLOR_ACTIVECAPTION   2
#define COLOR_INACTIVECAPTION 3
#define COLOR_MENU            4
#define COLOR_WINDOW          5
#define COLOR_WINDOWFRAME     6
#define COLOR_MENUTEXT        7
#define COLOR_WINDOWTEXT      8
#define COLOR_CAPTIONTEXT     9
#define COLOR_ACTIVEBORDER    10
#define COLOR_INACTIVEBORDER  11
#define COLOR_APPWORKSPACE    12
#define COLOR_HIGHLIGHT       13
#define COLOR_HIGHLIGHTTEXT   14
#define COLOR_BTNFACE         15
#define COLOR_BTNSHADOW       16
#define COLOR_GRAYTEXT        17
#define COLOR_BTNTEXT         18
#define COLOR_INACTIVECAPTIONTEXT 19
#define COLOR_BTNHIGHLIGHT    20
#define COLOR_3DDKSHADOW      21
#define COLOR_3DLIGHT         22
#define SM_CXSCREEN    0
#define SM_CYSCREEN    1
#define SM_CXVSCROLL   2
#define SM_CYHSCROLL   3
#define SM_CYCAPTION   4
#define SM_CXBORDER    5
#define SM_CYBORDER    6
#define SM_CXDLGFRAME  7
#define SM_CYDLGFRAME  8
#define SM_CYVTHUMB    9
#define SM_CXHTHUMB    10
#define SM_CXICON      11
#define SM_CYICON      12
#define SM_CXCURSOR    13
#define SM_CYCURSOR    14
#define SM_CYMENU      15
#define SM_CXFULLSCREEN 16
#define SM_CYFULLSCREEN 17
#define SM_CYVSCROLL   20
#define SM_CXHSCROLL   21
#define SM_CXSIZE      30
#define SM_CYSIZE      31
#define SM_CXFRAME     32
#define SM_CYFRAME     33

/* Resources */
#define IDC_ARROW    MAKEINTRESOURCE(32512)
#define IDC_IBEAM    MAKEINTRESOURCE(32513)
#define IDC_WAIT     MAKEINTRESOURCE(32514)
#define IDC_CROSS    MAKEINTRESOURCE(32515)
#define IDC_SIZE     MAKEINTRESOURCE(32640)
#define IDC_SIZENWSE MAKEINTRESOURCE(32642)
#define IDC_SIZENESW MAKEINTRESOURCE(32643)
#define IDC_SIZEWE   MAKEINTRESOURCE(32644)
#define IDC_SIZENS   MAKEINTRESOURCE(32645)
#define IDC_SIZEALL  MAKEINTRESOURCE(32646)
#define IDC_NO       MAKEINTRESOURCE(32648)
#define IDI_APPLICATION MAKEINTRESOURCE(32512)
#define RT_CURSOR       MAKEINTRESOURCE(1)
#define RT_BITMAP       MAKEINTRESOURCE(2)
#define RT_ICON         MAKEINTRESOURCE(3)
#define RT_MENU         MAKEINTRESOURCE(4)
#define RT_DIALOG       MAKEINTRESOURCE(5)
#define RT_STRING       MAKEINTRESOURCE(6)
#define RT_ACCELERATOR  MAKEINTRESOURCE(9)
#define RT_RCDATA       MAKEINTRESOURCE(10)
#define RT_GROUP_CURSOR MAKEINTRESOURCE(12)
#define RT_GROUP_ICON   MAKEINTRESOURCE(14)
#define RT_VERSION      MAKEINTRESOURCE(16)

/* Message boxes and dialog results */
#define MB_OK               0x00000000L
#define MB_OKCANCEL         0x00000001L
#define MB_YESNOCANCEL      0x00000003L
#define MB_YESNO            0x00000004L
#define MB_ICONHAND         0x00000010L
#define MB_ICONERROR        MB_ICONHAND
#define MB_ICONQUESTION     0x00000020L
#define MB_ICONEXCLAMATION  0x00000030L
#define MB_ICONASTERISK     0x00000040L
#define MB_SYSTEMMODAL      0x00001000L
#define MB_TASKMODAL        0x00002000L
#define IDOK     1
#define IDCANCEL 2
#define IDABORT  3
#define IDRETRY  4
#define IDIGNORE 5
#define IDYES    6
#define IDNO     7

/* Help */
#define HELP_CONTEXT    0x0001
#define HELP_QUIT       0x0002
#define HELP_INDEX      0x0003
#define HELP_CONTENTS   0x0003
#define HELP_HELPONHELP 0x0004
#define HELP_KEY        0x0101
#define HELP_PARTIALKEY 0x0105

/* ---- GDI constants ---- */
#define SRCCOPY     0x00CC0020UL
#define SRCPAINT    0x00EE0086UL
#define SRCAND      0x008800C6UL
#define SRCINVERT   0x00660046UL
#define NOTSRCCOPY  0x00330008UL
#define PATCOPY     0x00F00021UL
#define PATINVERT   0x005A0049UL
#define DSTINVERT   0x00550009UL
#define BLACKNESS   0x00000042UL
#define WHITENESS   0x00FF0062UL
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1
#define CBM_INIT 0x04L
#define BI_RGB 0L
#define BI_RLE8 1L
#define TRANSPARENT 1
#define OPAQUE      2
#define ETO_OPAQUE  0x0002
#define ETO_CLIPPED 0x0004
#define WHITE_BRUSH  0
#define LTGRAY_BRUSH 1
#define GRAY_BRUSH   2
#define DKGRAY_BRUSH 3
#define BLACK_BRUSH  4
#define NULL_BRUSH   5
#define HOLLOW_BRUSH NULL_BRUSH
#define WHITE_PEN    6
#define BLACK_PEN    7
#define NULL_PEN     8
#define OEM_FIXED_FONT  10
#define ANSI_FIXED_FONT 11
#define ANSI_VAR_FONT   12
#define SYSTEM_FONT     13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE 15
#define SYSTEM_FIXED_FONT 16
#define DEFAULT_GUI_FONT 17
#define BS_SOLID   0
#define BS_NULL    1
#define BS_HOLLOW  BS_NULL
#define BS_HATCHED 2
#define HS_HORIZONTAL 0
#define HS_VERTICAL   1
#define HS_FDIAGONAL  2
#define HS_BDIAGONAL  3
#define HS_CROSS      4
#define HS_DIAGCROSS  5
#define PS_SOLID 0
#define PS_NULL  5
#define HORZRES     8
#define VERTRES     10
#define BITSPIXEL   12
#define PLANES      14
#define NUMCOLORS   24
#define RASTERCAPS  38
#define LOGPIXELSX  88
#define LOGPIXELSY  90
#define SIZEPALETTE 104
#define NUMRESERVED 106
#define COLORRES    108
#define RC_PALETTE  0x0100
#define PC_RESERVED   0x01
#define PC_EXPLICIT   0x02
#define PC_NOCOLLAPSE 0x04
#define FW_DONTCARE  0
#define FW_THIN      100
#define FW_LIGHT     300
#define FW_NORMAL    400
#define FW_MEDIUM    500
#define FW_SEMIBOLD  600
#define FW_BOLD      700
#define FW_HEAVY     900
#define ANSI_CHARSET    0
#define DEFAULT_CHARSET 1
#define OEM_CHARSET     255
#define OUT_DEFAULT_PRECIS  0
#define CLIP_DEFAULT_PRECIS 0
#define DEFAULT_QUALITY 0
#define DRAFT_QUALITY   1
#define PROOF_QUALITY   2
#define DEFAULT_PITCH   0
#define FIXED_PITCH     1
#define VARIABLE_PITCH  2
#define FF_DONTCARE (0 << 4)
#define FF_ROMAN    (1 << 4)
#define FF_SWISS    (2 << 4)
#define FF_MODERN   (3 << 4)
#define FF_SCRIPT   (4 << 4)
#define FF_DECORATIVE (5 << 4)

/* ---- Kernel constants ---- */
#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH  2
#define DLL_THREAD_DETACH  3
#define GMEM_FIXED    0x0000
#define GMEM_MOVEABLE 0x0002
#define GMEM_ZEROINIT 0x0040
#define GHND          (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR          (GMEM_FIXED | GMEM_ZEROINIT)
#define GENERIC_READ  0x80000000UL
#define GENERIC_WRITE 0x40000000UL
#define FILE_SHARE_READ  0x00000001
#define FILE_SHARE_WRITE 0x00000002
#define CREATE_NEW        1
#define CREATE_ALWAYS     2
#define OPEN_EXISTING     3
#define OPEN_ALWAYS       4
#define TRUNCATE_EXISTING 5
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#define FILE_BEGIN   0
#define FILE_CURRENT 1
#define FILE_END     2
#define OF_READ      0x0000
#define OF_WRITE     0x0001
#define OF_READWRITE 0x0002
#define ERROR_SUCCESS          0L
#define ERROR_FILE_NOT_FOUND   2L
#define ERROR_PATH_NOT_FOUND   3L
#define ERROR_ACCESS_DENIED    5L
#define ERROR_INVALID_HANDLE   6L
#define ERROR_NOT_ENOUGH_MEMORY 8L
#define ERROR_INVALID_PARAMETER 87L
#define ERROR_ALREADY_EXISTS   183L
#define ERROR_MORE_DATA        234L
#define ERROR_NO_MORE_ITEMS    259L
#define ERROR_RESOURCE_NAME_NOT_FOUND 1814L

/* ---- Registry ---- */
#define HKEY_CLASSES_ROOT   ((HKEY)(ULONG_PTR)0x80000000UL)
#define HKEY_CURRENT_USER   ((HKEY)(ULONG_PTR)0x80000001UL)
#define HKEY_LOCAL_MACHINE  ((HKEY)(ULONG_PTR)0x80000002UL)
#define HKEY_USERS          ((HKEY)(ULONG_PTR)0x80000003UL)
#define REG_NONE   0
#define REG_SZ     1
#define REG_BINARY 3
#define REG_DWORD  4
#define REG_OPTION_NON_VOLATILE 0
#define REG_CREATED_NEW_KEY     1
#define REG_OPENED_EXISTING_KEY 2
#define KEY_QUERY_VALUE 0x0001
#define KEY_SET_VALUE   0x0002
#define KEY_READ        0x20019
#define KEY_WRITE       0x20006
#define KEY_ALL_ACCESS  0xF003F
typedef DWORD REGSAM;

/* ---- Functions ---- */

/* kernel32 */
DWORD WINAPI GetVersion(void);
DWORD WINAPI GetLastError(void);
void WINAPI SetLastError(DWORD err);
void WINAPI ExitProcess(UINT code);
void WINAPI Sleep(DWORD ms);
DWORD WINAPI GetTickCount(void);
void WINAPI OutputDebugStringA(LPCSTR text);
void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS status);
HGLOBAL WINAPI GlobalAlloc(UINT flags, SIZE_T size);
LPVOID WINAPI GlobalLock(HGLOBAL mem);
BOOL WINAPI GlobalUnlock(HGLOBAL mem);
HGLOBAL WINAPI GlobalFree(HGLOBAL mem);
SIZE_T WINAPI GlobalSize(HGLOBAL mem);
int WINAPI MulDiv(int number, int numerator, int denominator);
LPSTR WINAPI lstrcpyA(LPSTR dst, LPCSTR src);
LPSTR WINAPI lstrcatA(LPSTR dst, LPCSTR src);
int WINAPI lstrlenA(LPCSTR s);
int WINAPI lstrcmpA(LPCSTR a, LPCSTR b);
int WINAPI lstrcmpiA(LPCSTR a, LPCSTR b);
DWORD WINAPI GetModuleFileNameA(HMODULE module, LPSTR name, DWORD size);
HMODULE WINAPI GetModuleHandleA(LPCSTR name);
DWORD WINAPI GetCurrentDirectoryA(DWORD size, LPSTR buf);
UINT WINAPI GetWindowsDirectoryA(LPSTR buf, UINT size);
BOOL WINAPI CreateDirectoryA(LPCSTR path, LPSECURITY_ATTRIBUTES sa);
BOOL WINAPI DeleteFileA(LPCSTR path);
HANDLE WINAPI CreateFileA(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD creation,
                          DWORD flags, HANDLE templ);
BOOL WINAPI ReadFile(HANDLE file, LPVOID buf, DWORD size, LPDWORD read, LPVOID overlapped);
BOOL WINAPI WriteFile(HANDLE file, LPCVOID buf, DWORD size, LPDWORD written, LPVOID overlapped);
DWORD WINAPI GetFileSize(HANDLE file, LPDWORD high);
DWORD WINAPI SetFilePointer(HANDLE file, LONG dist, LONG *high, DWORD method);
BOOL WINAPI CloseHandle(HANDLE h);
HFILE WINAPI _lopen(LPCSTR path, int mode);
HFILE WINAPI _lcreat(LPCSTR path, int attr);
UINT WINAPI _lread(HFILE file, LPVOID buf, UINT size);
UINT WINAPI _lwrite(HFILE file, LPCSTR buf, UINT size);
LONG WINAPI _llseek(HFILE file, LONG offset, int origin);
HFILE WINAPI _lclose(HFILE file);
HRSRC WINAPI FindResourceA(HMODULE module, LPCSTR name, LPCSTR type);
HGLOBAL WINAPI LoadResource(HMODULE module, HRSRC res);
LPVOID WINAPI LockResource(HGLOBAL res);
BOOL WINAPI FreeResource(HGLOBAL res);
DWORD WINAPI SizeofResource(HMODULE module, HRSRC res);

/* version.dll */
DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR file, LPDWORD handle);
BOOL WINAPI GetFileVersionInfoA(LPCSTR file, DWORD handle, DWORD size, LPVOID data);
BOOL WINAPI VerQueryValueA(LPCVOID block, LPCSTR sub, LPVOID *buf, PUINT len);

/* advapi32 */
LONG WINAPI RegOpenKeyExA(HKEY key, LPCSTR sub, DWORD options, REGSAM sam, PHKEY result);
LONG WINAPI RegCreateKeyExA(HKEY key, LPCSTR sub, DWORD reserved, LPSTR cls, DWORD options, REGSAM sam,
                            LPSECURITY_ATTRIBUTES sa, PHKEY result, LPDWORD disposition);
LONG WINAPI RegCloseKey(HKEY key);
LONG WINAPI RegFlushKey(HKEY key);
LONG WINAPI RegDeleteKeyA(HKEY key, LPCSTR sub);
LONG WINAPI RegEnumKeyExA(HKEY key, DWORD index, LPSTR name, LPDWORD len, LPDWORD reserved, LPSTR cls,
                          LPDWORD clslen, PFILETIME time);
LONG WINAPI RegQueryValueExA(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size);
LONG WINAPI RegSetValueExA(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD size);

/* user32: windows */
ATOM WINAPI RegisterClassA(const WNDCLASSA *wc);
HWND WINAPI CreateWindowExA(DWORD exStyle, LPCSTR cls, LPCSTR name, DWORD style, int x, int y, int w, int h,
                            HWND parent, HMENU menu, HINSTANCE inst, LPVOID param);
#define CreateWindowA(cls, name, style, x, y, w, h, parent, menu, inst, param) \
    CreateWindowExA(0, cls, name, style, x, y, w, h, parent, menu, inst, param)
BOOL WINAPI DestroyWindow(HWND hwnd);
LRESULT WINAPI DefWindowProcA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI CallWindowProcA(WNDPROC proc, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI IsWindow(HWND hwnd);
BOOL WINAPI IsWindowVisible(HWND hwnd);
BOOL WINAPI ShowWindow(HWND hwnd, int cmd);
BOOL WINAPI UpdateWindow(HWND hwnd);
BOOL WINAPI SetWindowPos(HWND hwnd, HWND after, int x, int y, int cx, int cy, UINT flags);
BOOL WINAPI MoveWindow(HWND hwnd, int x, int y, int w, int h, BOOL repaint);
BOOL WINAPI GetWindowRect(HWND hwnd, LPRECT rect);
BOOL WINAPI GetClientRect(HWND hwnd, LPRECT rect);
BOOL WINAPI AdjustWindowRect(LPRECT rect, DWORD style, BOOL menu);
BOOL WINAPI AdjustWindowRectEx(LPRECT rect, DWORD style, BOOL menu, DWORD exStyle);
BOOL WINAPI ClientToScreen(HWND hwnd, LPPOINT pt);
BOOL WINAPI ScreenToClient(HWND hwnd, LPPOINT pt);
int WINAPI MapWindowPoints(HWND from, HWND to, LPPOINT pts, UINT n);
HWND WINAPI ChildWindowFromPoint(HWND parent, POINT pt);
HWND WINAPI WindowFromPoint(POINT pt);
HWND WINAPI GetParent(HWND hwnd);
HWND WINAPI SetParent(HWND hwnd, HWND parent);
HWND WINAPI GetWindow(HWND hwnd, UINT cmd);
HWND WINAPI GetDesktopWindow(void);
HWND WINAPI FindWindowA(LPCSTR cls, LPCSTR name);
LONG WINAPI GetWindowLongA(HWND hwnd, int index);
LONG WINAPI SetWindowLongA(HWND hwnd, int index, LONG value);
LONG_PTR WINAPI GetWindowLongPtrA(HWND hwnd, int index);
LONG_PTR WINAPI SetWindowLongPtrA(HWND hwnd, int index, LONG_PTR value);
WORD WINAPI GetWindowWord(HWND hwnd, int index);
WORD WINAPI SetWindowWord(HWND hwnd, int index, WORD value);
int WINAPI GetWindowTextA(HWND hwnd, LPSTR buf, int size);
int WINAPI GetWindowTextLengthA(HWND hwnd);
BOOL WINAPI SetWindowTextA(HWND hwnd, LPCSTR text);
int WINAPI GetClassNameA(HWND hwnd, LPSTR buf, int size);
BOOL WINAPI EnableWindow(HWND hwnd, BOOL enable);
BOOL WINAPI IsWindowEnabled(HWND hwnd);
HWND WINAPI SetFocus(HWND hwnd);
HWND WINAPI GetFocus(void);
HWND WINAPI GetActiveWindow(void);
HWND WINAPI SetActiveWindow(HWND hwnd);
BOOL WINAPI SetForegroundWindow(HWND hwnd);
BOOL WINAPI IsIconic(HWND hwnd);
BOOL WINAPI IsZoomed(HWND hwnd);
BOOL WINAPI InvalidateRect(HWND hwnd, const RECT *rect, BOOL erase);
BOOL WINAPI ValidateRect(HWND hwnd, const RECT *rect);
HDC WINAPI BeginPaint(HWND hwnd, LPPAINTSTRUCT ps);
BOOL WINAPI EndPaint(HWND hwnd, const PAINTSTRUCT *ps);
HDC WINAPI GetDC(HWND hwnd);
HDC WINAPI GetWindowDC(HWND hwnd);
int WINAPI ReleaseDC(HWND hwnd, HDC hdc);
BOOL WINAPI ScrollWindow(HWND hwnd, int dx, int dy, const RECT *rect, const RECT *clip);
int WINAPI SetScrollPos(HWND hwnd, int bar, int pos, BOOL redraw);
int WINAPI GetScrollPos(HWND hwnd, int bar);
BOOL WINAPI SetScrollRange(HWND hwnd, int bar, int min, int max, BOOL redraw);
BOOL WINAPI GetScrollRange(HWND hwnd, int bar, LPINT min, LPINT max);
HWND WINAPI SetCapture(HWND hwnd);
BOOL WINAPI ReleaseCapture(void);
HWND WINAPI GetCapture(void);
BOOL WINAPI GetCursorPos(LPPOINT pt);
BOOL WINAPI SetCursorPos(int x, int y);
HCURSOR WINAPI SetCursor(HCURSOR cursor);
HCURSOR WINAPI GetCursor(void);
int WINAPI ShowCursor(BOOL show);
HCURSOR WINAPI LoadCursorA(HINSTANCE inst, LPCSTR name);
BOOL WINAPI DestroyCursor(HCURSOR cursor);
HICON WINAPI LoadIconA(HINSTANCE inst, LPCSTR name);
BOOL WINAPI DestroyIcon(HICON icon);
BOOL WINAPI DrawIcon(HDC hdc, int x, int y, HICON icon);
int WINAPI GetSystemMetrics(int index);
DWORD WINAPI GetSysColor(int index);
HBRUSH WINAPI GetSysColorBrush(int index);
SHORT WINAPI GetAsyncKeyState(int vk);
SHORT WINAPI GetKeyState(int vk);
BOOL WINAPI WinHelpA(HWND hwnd, LPCSTR file, UINT cmd, ULONG_PTR data);

/* user32: rectangles */
BOOL WINAPI SetRect(LPRECT r, int left, int top, int right, int bottom);
BOOL WINAPI SetRectEmpty(LPRECT r);
BOOL WINAPI CopyRect(LPRECT dst, const RECT *src);
BOOL WINAPI IsRectEmpty(const RECT *r);
BOOL WINAPI PtInRect(const RECT *r, POINT pt);
BOOL WINAPI IntersectRect(LPRECT dst, const RECT *a, const RECT *b);
BOOL WINAPI UnionRect(LPRECT dst, const RECT *a, const RECT *b);
BOOL WINAPI OffsetRect(LPRECT r, int dx, int dy);
BOOL WINAPI InflateRect(LPRECT r, int dx, int dy);
BOOL WINAPI EqualRect(const RECT *a, const RECT *b);

/* user32: messages */
BOOL WINAPI GetMessageA(LPMSG msg, HWND hwnd, UINT min, UINT max);
BOOL WINAPI PeekMessageA(LPMSG msg, HWND hwnd, UINT min, UINT max, UINT remove);
BOOL WINAPI WaitMessage(void);
BOOL WINAPI TranslateMessage(const MSG *msg);
LRESULT WINAPI DispatchMessageA(const MSG *msg);
LRESULT WINAPI SendMessageA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI PostMessageA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void WINAPI PostQuitMessage(int code);
UINT_PTR WINAPI SetTimer(HWND hwnd, UINT_PTR id, UINT ms, TIMERPROC proc);
BOOL WINAPI KillTimer(HWND hwnd, UINT_PTR id);
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE inst, LPCSTR name);
int WINAPI TranslateAcceleratorA(HWND hwnd, HACCEL accel, LPMSG msg);

/* user32: dialogs */
INT_PTR WINAPI DialogBoxParamA(HINSTANCE inst, LPCSTR name, HWND parent, DLGPROC proc, LPARAM param);
#define DialogBoxA(inst, name, parent, proc) DialogBoxParamA(inst, name, parent, proc, 0)
BOOL WINAPI EndDialog(HWND hDlg, INT_PTR result);
HWND WINAPI GetDlgItem(HWND hDlg, int id);
int WINAPI GetDlgCtrlID(HWND hwnd);
UINT WINAPI GetDlgItemTextA(HWND hDlg, int id, LPSTR buf, int size);
BOOL WINAPI SetDlgItemTextA(HWND hDlg, int id, LPCSTR text);
UINT WINAPI GetDlgItemInt(HWND hDlg, int id, BOOL *ok, BOOL sign);
BOOL WINAPI SetDlgItemInt(HWND hDlg, int id, UINT value, BOOL sign);
LRESULT WINAPI SendDlgItemMessageA(HWND hDlg, int id, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI CheckDlgButton(HWND hDlg, int id, UINT check);
UINT WINAPI IsDlgButtonChecked(HWND hDlg, int id);
BOOL WINAPI CheckRadioButton(HWND hDlg, int first, int last, int check);
int WINAPI MessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type);

/* user32: menus */
HMENU WINAPI LoadMenuA(HINSTANCE inst, LPCSTR name);
HMENU WINAPI GetMenu(HWND hwnd);
BOOL WINAPI SetMenu(HWND hwnd, HMENU menu);
BOOL WINAPI DestroyMenu(HMENU menu);
BOOL WINAPI EnableMenuItem(HMENU menu, UINT id, UINT flags);
DWORD WINAPI CheckMenuItem(HMENU menu, UINT id, UINT flags);
BOOL WINAPI DrawMenuBar(HWND hwnd);
HMENU WINAPI GetSubMenu(HMENU menu, int pos);
int WINAPI GetMenuItemCount(HMENU menu);
UINT WINAPI GetMenuItemID(HMENU menu, int pos);
UINT WINAPI GetMenuState(HMENU menu, UINT id, UINT flags);
int WINAPI GetMenuStringA(HMENU menu, UINT id, LPSTR buf, int size, UINT flags);

/* user32: strings */
int WINAPI LoadStringA(HINSTANCE inst, UINT id, LPSTR buf, int size);
int WINAPIV wsprintfA(LPSTR buf, LPCSTR fmt, ...);
int WINAPI wvsprintfA(LPSTR buf, LPCSTR fmt, va_list args);

/* user32 drawing helpers */
int WINAPI FillRect(HDC hdc, const RECT *r, HBRUSH brush);
int WINAPI FrameRect(HDC hdc, const RECT *r, HBRUSH brush);
BOOL WINAPI DrawFocusRect(HDC hdc, const RECT *r);
BOOL WINAPI InvertRect(HDC hdc, const RECT *r);

/* gdi32 */
HDC WINAPI CreateDCA(LPCSTR driver, LPCSTR device, LPCSTR output, const void *init);
HDC WINAPI CreateCompatibleDC(HDC hdc);
BOOL WINAPI DeleteDC(HDC hdc);
HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ obj);
BOOL WINAPI DeleteObject(HGDIOBJ obj);
int WINAPI GetObjectA(HGDIOBJ obj, int size, LPVOID buf);
HGDIOBJ WINAPI GetStockObject(int index);
HBITMAP WINAPI CreateDIBSection(HDC hdc, const BITMAPINFO *bmi, UINT usage, void **bits, HANDLE section,
                                DWORD offset);
HBITMAP WINAPI CreateDIBitmap(HDC hdc, const BITMAPINFOHEADER *header, DWORD init, const void *bits,
                              const BITMAPINFO *bmi, UINT usage);
HBITMAP WINAPI CreateCompatibleBitmap(HDC hdc, int w, int h);
HBITMAP WINAPI CreateBitmap(int w, int h, UINT planes, UINT bits, const void *data);
int WINAPI GetDIBits(HDC hdc, HBITMAP bm, UINT start, UINT lines, LPVOID bits, LPBITMAPINFO bmi, UINT usage);
UINT WINAPI SetDIBColorTable(HDC hdc, UINT start, UINT n, const RGBQUAD *colors);
BOOL WINAPI BitBlt(HDC dst, int x, int y, int w, int h, HDC src, int sx, int sy, DWORD rop);
BOOL WINAPI StretchBlt(HDC dst, int x, int y, int w, int h, HDC src, int sx, int sy, int sw, int sh, DWORD rop);
int WINAPI StretchDIBits(HDC hdc, int x, int y, int w, int h, int sx, int sy, int sw, int sh, const void *bits,
                         const BITMAPINFO *bmi, UINT usage, DWORD rop);
int WINAPI SetDIBitsToDevice(HDC hdc, int x, int y, DWORD w, DWORD h, int sx, int sy, UINT start, UINT lines,
                             const void *bits, const BITMAPINFO *bmi, UINT usage);
BOOL WINAPI PatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop);
BOOL WINAPI Rectangle(HDC hdc, int left, int top, int right, int bottom);
BOOL WINAPI MoveToEx(HDC hdc, int x, int y, LPPOINT old);
BOOL WINAPI LineTo(HDC hdc, int x, int y);
COLORREF WINAPI SetPixel(HDC hdc, int x, int y, COLORREF c);
COLORREF WINAPI GetPixel(HDC hdc, int x, int y);
HBRUSH WINAPI CreateSolidBrush(COLORREF c);
HBRUSH WINAPI CreateHatchBrush(int style, COLORREF c);
HBRUSH WINAPI CreateBrushIndirect(const LOGBRUSH *lb);
HPEN WINAPI CreatePen(int style, int width, COLORREF c);
BOOL WINAPI GdiFlush(void);
int WINAPI GetDeviceCaps(HDC hdc, int index);
HPALETTE WINAPI CreatePalette(const LOGPALETTE *lp);
HPALETTE WINAPI SelectPalette(HDC hdc, HPALETTE pal, BOOL background);
UINT WINAPI RealizePalette(HDC hdc);
BOOL WINAPI UnrealizeObject(HGDIOBJ obj);
UINT WINAPI GetPaletteEntries(HPALETTE pal, UINT start, UINT n, LPPALETTEENTRY entries);
UINT WINAPI SetPaletteEntries(HPALETTE pal, UINT start, UINT n, const PALETTEENTRY *entries);
UINT WINAPI GetSystemPaletteEntries(HDC hdc, UINT start, UINT n, LPPALETTEENTRY entries);
UINT WINAPI GetNearestPaletteIndex(HPALETTE pal, COLORREF c);
BOOL WINAPI AnimatePalette(HPALETTE pal, UINT start, UINT n, const PALETTEENTRY *entries);
int WINAPI UpdateColors(HDC hdc);
HFONT WINAPI CreateFontA(int h, int w, int esc, int orient, int weight, DWORD italic, DWORD underline,
                         DWORD strike, DWORD charset, DWORD outPrec, DWORD clipPrec, DWORD quality,
                         DWORD pitch, LPCSTR face);
BOOL WINAPI TextOutA(HDC hdc, int x, int y, LPCSTR text, int n);
BOOL WINAPI ExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT *r, LPCSTR text, UINT n, const INT *dx);
BOOL WINAPI GetTextExtentPointA(HDC hdc, LPCSTR text, int n, LPSIZE size);
BOOL WINAPI GetTextExtentPoint32A(HDC hdc, LPCSTR text, int n, LPSIZE size);
BOOL WINAPI GetTextMetricsA(HDC hdc, LPTEXTMETRIC tm);
COLORREF WINAPI SetTextColor(HDC hdc, COLORREF c);
COLORREF WINAPI GetTextColor(HDC hdc);
COLORREF WINAPI SetBkColor(HDC hdc, COLORREF c);
COLORREF WINAPI GetBkColor(HDC hdc);
int WINAPI SetBkMode(HDC hdc, int mode);
int WINAPI SetStretchBltMode(HDC hdc, int mode);

/* The game's entry point, called by the layer's main() */
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdLine, int show);

/* ANSI names */
#define GetModuleFileName GetModuleFileNameA
#define GetModuleHandle GetModuleHandleA
#define GetCurrentDirectory GetCurrentDirectoryA
#define GetWindowsDirectory GetWindowsDirectoryA
#define CreateDirectory CreateDirectoryA
#define DeleteFile DeleteFileA
#define CreateFile CreateFileA
#define OutputDebugString OutputDebugStringA
#define FindResource FindResourceA
#define lstrcpy lstrcpyA
#define lstrcat lstrcatA
#define lstrlen lstrlenA
#define lstrcmp lstrcmpA
#define lstrcmpi lstrcmpiA
#define GetFileVersionInfoSize GetFileVersionInfoSizeA
#define GetFileVersionInfo GetFileVersionInfoA
#define VerQueryValue VerQueryValueA
#define RegOpenKeyEx RegOpenKeyExA
#define RegCreateKeyEx RegCreateKeyExA
#define RegDeleteKey RegDeleteKeyA
#define RegEnumKeyEx RegEnumKeyExA
#define RegQueryValueEx RegQueryValueExA
#define RegSetValueEx RegSetValueExA
#define RegisterClass RegisterClassA
#define CreateWindowEx CreateWindowExA
#define CreateWindow CreateWindowA
#define DefWindowProc DefWindowProcA
#define CallWindowProc CallWindowProcA
#define FindWindow FindWindowA
#define GetWindowLong GetWindowLongA
#define SetWindowLong SetWindowLongA
#define GetWindowLongPtr GetWindowLongPtrA
#define SetWindowLongPtr SetWindowLongPtrA
#define GetWindowText GetWindowTextA
#define GetWindowTextLength GetWindowTextLengthA
#define SetWindowText SetWindowTextA
#define GetClassName GetClassNameA
#define LoadCursor LoadCursorA
#define LoadIcon LoadIconA
#define WinHelp WinHelpA
#define GetMessage GetMessageA
#define PeekMessage PeekMessageA
#define DispatchMessage DispatchMessageA
#define SendMessage SendMessageA
#define PostMessage PostMessageA
#define LoadAccelerators LoadAcceleratorsA
#define TranslateAccelerator TranslateAcceleratorA
#define DialogBoxParam DialogBoxParamA
#define DialogBox DialogBoxA
#define GetDlgItemText GetDlgItemTextA
#define SetDlgItemText SetDlgItemTextA
#define SendDlgItemMessage SendDlgItemMessageA
#define MessageBox MessageBoxA
#define LoadMenu LoadMenuA
#define GetMenuString GetMenuStringA
#define LoadString LoadStringA
#define wsprintf wsprintfA
#define wvsprintf wvsprintfA
#define CreateDC CreateDCA
#define GetObject GetObjectA
#define CreateFont CreateFontA
#define TextOut TextOutA
#define ExtTextOut ExtTextOutA
#define GetTextExtentPoint GetTextExtentPointA
#define GetTextExtentPoint32 GetTextExtentPoint32A
#define GetTextMetrics GetTextMetricsA

#ifdef __cplusplus
}
#endif

#include <mmsystem.h>

#endif
