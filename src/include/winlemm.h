/*
 * LEMMINGS.EXE: declarations shared by the game's source files.
 */
#ifndef WINLEMM_H
#define WINLEMM_H

#include <windows.h>
#include "port64.h"
#include "vsmem.h"
#include "filehand.h"
#include "readdib.h"
#include "render.h"
#include "loadlvl.h"
#include "procdll.h"

#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

/* winmain.cpp */
extern char g_szAppName[16];
extern char g_szIniFile[];
extern HWND g_hwnd;
extern HWND g_hwndView;
extern int g_global28;
extern int g_global2c;
extern int g_win32;
extern int g_win32s;
extern HINSTANCE g_hInst;
extern int g_nCmdShowData;
extern char g_curDir[1024];
extern HCURSOR g_hCrossCursorNext;
extern int pad_hdr_g_hboxcursor;     /* declared in gameview.cpp in the original */
extern HCURSOR g_hSmaCrossCursorState;
extern HCURSOR g_hSmaBoxCursorGameNew;
extern HCURSOR g_boxHCursorLeft;
extern HCURSOR g_boxCursorHRight;
extern HCURSOR g_leftHBoxSmaCursor;
extern HCURSOR g_hBoxCursorRightSma;
extern MEMORYSTATUS g_memStatus;

void CheckColourDepth(void);
void InitSystemInfo(void);
int InitApplication(void);
void QuitApplication(void);
int WaitTime(DWORD ms);
int InitClasses(HINSTANCE hInstance);
int InitInstance(HINSTANCE hInstance, int nCmdShow);
char *GetString(UINT id);
void ErrorBox(UINT id);
EXPORT void WINAPI FatalErrorCleanUp(void);
void FatalError(UINT id);
int WindowFits(int size, int scale);

/* options.cpp */
extern int g_optionsApplied;
int ApplyOptions(HWND hDlg, int volume);
void InitOptions(HWND hDlg);
BOOL CALLBACK OptionsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/* about.cpp */
void CentreWindow(HWND hwnd, HWND hwndParent);
char *GetSystemInfoText(void);
BOOL CALLBACK AboutDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/* icons.cpp */
extern UINT g_iconTimer;
extern int g_releaseRate;
extern int g_minReleaseRate;
extern int g_resetting;
extern int g_textHeight;
extern int g_charWidth;
extern HWND g_hPlusButton;

inline void CountRect(RECT *r)
{
    r->left = 20 - g_charWidth / 2;
    r->top = 6;
    r->bottom = g_textHeight + 6;
    r->right = g_charWidth - g_charWidth / 2 + 20;
}

/* user.cpp */
extern int g_unused08c;
extern char *g_selectedUser;
extern int g_newUser;
extern HWND g_hUserList;
extern HWND g_hUserEdit;
extern HWND g_hDeleteUser;
extern char **g_userNames;
extern int pad_hdr_numusers;    /* was g_numUsers: the original declares it in user.cpp only */
extern char g_userEdit[128];
extern char g_userDir[256];
#ifdef PORT_SDL                 /* the native build: room for a real path (user.cpp) */
extern char g_nameDb[MAX_PATH];
#else
extern char g_nameDb[32];
#endif
void LoadSettings(void);
void SaveSettings(void);
void CopyDatabase(char *dest);
char *GetUserDir(void);
char *CreateUserDatabase(char *user);
char *GetUserDatabase(void);
void NewUser(char *name);
BOOL CALLBACK SelectUserDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int SelectUser(void);

/* registry.cpp */
extern HKEY g_hkGame;
extern HKEY g_hkUsers;
extern HKEY g_hkUser;
extern char *g_userName;
extern char g_userNameBuf[0x800];
extern char *g_hereUserList[256];
extern char g_stringReg[0x400];
void OpenRegistry(void);
void CloseRegistry(void);
HKEY OpenUser(char *name);
int EnumUsers(char ***names);
void DeleteUser(char *name);
HKEY OpenKey(HKEY parent, char *subkey, char *cls);
void CloseKey(HKEY key);
void WriteRegDword(HKEY key, char *name, DWORD value);
void WriteRegString(HKEY key, char *name, char *value);
DWORD ReadRegDword(HKEY key, char *name, DWORD def);
char *ReadRegString(HKEY key, char *name, char *def);

/* register.cpp */
extern HWND g_hRegEdit;
extern char *g_key;
extern char g_regName[0x200];
extern char g_encodedInfo[0x200];
char *EncodeName(char *name);
int SaveRegistration(HWND hEdit);
BOOL CALLBACK RegisterDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/* mainwnd.cpp */
extern int g_replaying;
extern int g_quitToMenu;
extern int g_currentSkill;
extern int g_unusedTable[23];
extern int g_paused;
extern int g_nuking;
extern int g_fastForward;
extern char g_helpMenu[104];
extern char g_savedTitle[80];
extern void (*g_ownMenuHelpCallback)(void);
extern int g_spareGameRunning;
int StartGame(HINSTANCE hInstance, int nCmdShow);
void EndGame(void);
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT OnCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void SelectSkill(HWND hwnd, int skill);
void SetZoom(HWND hwnd, int zoom);
void SaveLayout(void);
void LayoutWindows(HWND hwnd, int width, int height);
void ClearPause(void);
void TogglePause(HWND hwnd);
int IsPaused(void);
void PauseOnDeactivate(void);
void ClearNuke(void);
void StartNuke(HWND hwnd);
int IsNuking(void);
void ClearFastForward(void);
void ToggleFastForward(HWND hwnd);
int IsFastForward(void);
void SetMenuHelpCallback(void (*callback)(void));
void CallMenuHelp(void);
void MenuSelect(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* idle.cpp */
extern DWORD g_lastTick;
extern DWORD g_frameTime;
int GameIdle(void);
void AssertFailed(char *expr, char *file, int line);

/* statusbar.cpp */
struct PANE {
    int width;                  /* in average characters; -1: what is left */
    int border;
    char *text;
    RECT rect;
    int textWidth;
};

struct STATBAR {
    int border;
    int border2;
    int border3;
    int border8;
    int border9;
    int height;
    int charWidth;
    HFONT font;
    PANE *panes;
    int unused24;
};

int RegisterStatBar(HINSTANCE hInstance, STATBAR *bar, PANE *panes);
HWND CreateStatBar(HINSTANCE hInstance, HWND parent, int id, RECT *r, STATBAR *bar, PANE *panes);
void ResetStatBar(HWND hwnd, STATBAR *bar, PANE *panes);
LRESULT CALLBACK StatBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void StatBarDestroy(HWND hwnd);
void StatBarCreate(HWND hwnd);
void StatBarPaint(HWND hwnd);
void DrawPaneText(HDC hdc, STATBAR *bar, PANE *pane);
int PaneWidth(STATBAR *bar, PANE *pane);
void PlacePane(STATBAR *bar, PANE *pane, RECT *r);
void DrawPane(HDC hdc, STATBAR *bar, PANE *pane);
void DrawBarBorder(HDC hdc, STATBAR *bar, RECT *rect);
void DrawStatBar(HWND hwnd, HDC hdc, STATBAR *bar);
void InitStatBar(STATBAR *bar, PANE *panes);
int GetBarHeight(STATBAR *bar);
int TotalPaneWidth(STATBAR *bar);
extern char g_statusText[256];
extern HWND g_hStatus;
int RegisterStatusClass(HINSTANCE hInstance);
void CreateStatusBar(HINSTANCE hInstance, HWND parent, int id, RECT *rect);
int GetStatusHeight(void);
int GetStatusWidth(void);
void SetStatusTime(int min, int sec);
void SetStatusSave(int save, int total);
void SetStatusMessage(char *text);
void SetStatusHome(int home);
void SetStatusOut(int out);
void SetStatusText(char *text);

/* gameview.cpp */
typedef struct REPLAYEVENT {    /* 0x20 bytes */
    int time;
    int type;                   /* 0 skill, 1 scroll, 2 release rate, 3 nuke */
    int x, y;
    int value;
    int priority;
    int unused18, unused1c;
} REPLAYEVENT;

extern int g_recordTime;
extern int g_clicked;
extern WPARAM g_clickWParam;
extern LPARAM g_clickLParam;
extern int g_clickSkill;
extern WORD g_renderer;
extern WORD g_slot;
extern short g_scrollSpeedOld;
extern BYTE *g_rrSprite;
extern int pad_hdr_g_lastcursor;    /* local to gameview.cpp in the original */
extern HCURSOR g_viewCursor;
extern char g_viewClass[];
extern int g_inGame;
extern int g_needTitle;
extern UINT g_scrollTimer;
extern int g_priority;
extern int g_skillButtons[17];
extern LEVEL g_levelVal;
extern int g_viewW, g_viewHLast;
extern int g_viewPixW, g_viewPixHGameMain;
extern short g_dataViewWidth, g_extraViewHeight;
extern short g_tempScrollX, g_valScrollY;
extern short g_maxXScroll, g_yMaxScroll;
extern short g_scale;
extern short g_dragStartCur;
extern int g_skill;
extern int g_timeMinData, g_timeSec, g_savedFrames, g_usedFps;
extern DISPLAYLIST *g_list, *g_list2;
extern int g_nowRenderScale;
extern UINT g_lastDragTimer;
extern PDIB g_oldLevelDib;
extern char g_savedStatusBuf[128];
extern RGBQUAD g_tempPalette[256];
extern RGBQUAD g_palette2[256];
extern int g_wViewClient, g_clientViewH;
extern int g_mainWidthNext, g_mainHeightOldTemp;
int InGame(void);
int RegisterViewClass(HINSTANCE hInstance);
HWND CreateGameView(HINSTANCE hInstance, HWND parent, int id, RECT *rect);
void UpdatePriority(void);
LRESULT CALLBACK ViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void MergePalette(PDIB dib, RGBQUAD *pal);
void OnStartLevel(HWND hwnd, WPARAM wParam, LPARAM lParam);
void CountDown(int *min, int *sec, int *frames, int fps);
void OnFrame(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnEndLevel(HWND hwnd, WPARAM wParam, LPARAM lParam);
int GetViewHeight(int zoom);
void OnZoom(HWND hwnd, WPARAM wParam, LPARAM lParam);
void RenderView(HDC hdc, PAINTSTRUCT *ps);
void OnViewPaint(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnRedraw(HWND hwnd, WPARAM wParam, LPARAM lParam);
int ButtonForSkill(int skill);
void OnStartDrag(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnStopDrag(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnKeyDown(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnKeyUp(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnScrollTimer(HWND hwnd);
void OnClick(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam);
int GetViewMouse(HWND hwnd, POINT *pt);
void UpdateCursor(HWND hwnd);
void OnViewCreate(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnViewDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnViewSize(HWND hwnd, WPARAM wParam, LPARAM lParam);
void OnVScroll(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void ScrollTo(HWND hwnd, short dx);
void OnHScroll(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int LevelHeight(int zoom);
void SetViewScale(HWND hwnd, short scale);
void SetViewCursor(HCURSOR big, HCURSOR sma);
void ShowGameStatus(void);

/* frontend.cpp */
extern BYTE *g_feSprites, *g_feFont, *g_slapL, *g_slapR, *g_textBuf;
extern int g_feCaptured, g_feButton1, g_feButton2;
extern int g_slapLFrame, g_slapRFrame, g_feCentre;
extern char g_feMenu[];
extern HPALETTE g_fePalette;
extern PANE g_fePanes[2];
extern int g_textPos[20], g_textPrev[20];
extern WORD g_feRenderer, g_feSlot;
extern STATBAR g_barFe;
extern DISPLAYLIST *g_feListAllState, *g_feList2Extra;
extern DWORD g_flipMasks[9][2];
extern int g_leftRate, g_rightRate;
extern int g_textIndex, g_textDir, g_textBusy, g_slapDone, g_textWait, g_textDelay;
void InitFrontEnd(void);
int HitButton(POINT pt);
LRESULT CALLBACK FrontEndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void DrawFrontEnd(HDC hdc, PAINTSTRUCT *ps);
LRESULT FrontEndCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
PDIB CreateFrontDib(BYTE *pal);
WORD LoadFrontCanvas(WORD renderer, PDIB dib);
void SetFrontPalette(PDIB dib);
BYTE *ReadWholeFile(char *name);
BYTE *LoadPalFile(char *name);
BYTE *LoadSpriteFile(char *name);
void AddSprite(int x, int y, BYTE *sprites, int frame, int mode);
int CharIndex(char c);
void DrawFrontText(int x, int y, char *text, int mode);
void InitTextBuffer(void);
void DrawTextBuffer(void);
void UnpackRow(BYTE *dst, BYTE *src);
void FlipChar(int pos, int newChar, int oldChar, int step);
int AdvanceLeft(int *pos);
int AdvanceRight(int *pos);
int UpdateText(char *newText, char *oldText, int left);
void AnimateTitle(int init);
void StartFrontEnd(HWND hwnd);
void QuitFrontEnd(HWND hwnd);

/* settings.cpp */
extern int g_zoom, g_currentLevel, g_musicAvailable, g_waveAvailable;
extern int g_music, g_soundFX, g_visualFX, g_pauseFocus, g_volume;
extern char g_iniWidth[], g_iniHeight[], g_iniMaximised[], g_iniZoom[], g_iniLevel[], g_iniResolution[];
extern char g_iniCheat[], g_iniMusic[], g_iniSoundFX[], g_iniVisualFX[], g_iniPauseFocus[];
extern char g_iniIconDocked[], g_iniIconX[], g_iniIconY[], g_iniMapDocked[], g_iniMapX[], g_iniMapY[];
extern char g_szRegistered[], g_szSerialNum[], g_szParams[];
extern int g_taster;
extern char g_regUserUsed[84];
extern int g_cheat, g_maximised;
extern char g_dbHeader[128];
int IsTaster(void);
int CheckSerial(char *params, char *name);
int IsRegistered(void);
int CopyFileTo(char *dest, char *src);
void InstallUninstaller(void);
void ReadIniSettings(char *file);
void WriteIniSettings(char *file);

/* frame.cpp */
extern int g_musicPaused;
extern HPALETTE g_hPalette;
extern WNDPROC g_mainProc;
extern char g_frameClass[];
extern int g_paintCount;
extern PANE g_mainPanes[2];
extern STATBAR g_mainBar;
extern STATBAR *g_curBar;
extern PANE *g_curPanes;
extern RECT g_frameAdjust;
extern PALETTEENTRY g_sysPalSpare[256];
int RegisterFrameClass(HINSTANCE hInstance);
HWND FindPreviousInstance(void);
HWND CreateMainWindow(HINSTANCE hInstance);
LRESULT CALLBACK FrameProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void SetMainProc(WNDPROC proc);
HMENU LoadGameMenu(char *name);
void ShowStatBar(STATBAR *bar, PANE *panes);
void MakeUnique(BYTE *e, LOGPALETTE *pal, int count);
HPALETTE BuildPalette(RGBQUAD *colours, int n);
HPALETTE CopyPalette(HPALETTE pal);
void SetGamePalette(HPALETTE pal);
void RebuildPalette(void);
void ResetSystemPalette(void);
void InitPalette(void);
int GetFrameWidth(void);
int GetFrameHeight(void);
void FitWindow(HWND hwnd);

/* endscreen.cpp */
extern char *g_esText[10][2];
extern int g_esSaved;
extern BYTE *g_esSprites, *g_esFont;
extern int g_esCentre, g_esButton1, g_esButton2, g_esButton3, g_esCaptured;
extern char g_esMenu[];
extern PANE g_esPanes[2];
extern int g_esPassed, g_esFinished;
extern STATBAR g_esBarNew;
extern int g_valEsResult;
extern char g_line1Es[48], g_esLine2MainOwn[48], g_esLine3OwnNext[40];
extern WORD g_usedEsSlot, g_esRendererSavedVal;
void ShowEndScreen(void);
int EndHitButton(int x, int y);
LRESULT CALLBACK EndScreenProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int LevelPassed(int saved, int toRescue);
void MakeResultText(char **lines, int saved, int toRescue);
void DrawEndScreen(HDC hdc, PAINTSTRUCT *ps);
LRESULT EndScreenCommand(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int EndCharIndex(char c);
int SpriteWidth(BYTE *sprites, int frame);
int EndTextWidth(char *text);
void DrawEndText(int x, int y, char *text, int mode);
void DrawCentredText(int x, int y, char *text, int mode);
void StartEndScreen(HWND hwnd);
void QuitEndScreen(HWND hwnd);

/* levelsel.cpp */
#pragma pack(1)
typedef struct DBENTRY {        /* the user's level database, 0x89 bytes */
    WORD id;                    /* 0x00 */
    WORD type;                  /* 0x02: 3 a level, 0x101 a rating, 0x102 a group */
    WORD childType;             /* 0x04: what a rating or group holds */
    WORD unknown6;              /* 0x06 */
    WORD unknown8;              /* 0x08 */
    char name[40];              /* 0x0a */
    char file[34];              /* 0x32: a level's file, a rating's directory */
    char bitmap[34];            /* 0x54: the level's picture */
    WORD children;              /* 0x76 */
    WORD shown;                 /* 0x78: children in the list now */
    WORD status;                /* 0x7a: 1 done, 2 done by cheating */
    BYTE custom;                /* 0x7c: use these instead of the level file's */
    BYTE skills[8];             /* 0x7d */
    BYTE time;                  /* 0x85 */
    BYTE rescue;                /* 0x86 */
    BYTE lemmings;              /* 0x87 */
    BYTE rate;                  /* 0x88 */
} DBENTRY;
#pragma pack()

typedef struct DATABASE {
    int unknown0;
    int count;
    DBENTRY entries[1];
} DATABASE;

typedef struct TREE {           /* an owner-drawn list box shown as a tree */
    HDC hdc;                    /* 0x00: holds the bitmaps */
    HBITMAP bitmap;             /* 0x04 */
    HBITMAP oldBitmap;          /* 0x08 */
    int cellHeight;             /* 0x0c */
    int cellWidth;              /* 0x10 */
    int textHeight;             /* 0x14 */
    int lineHeight;             /* 0x18 */
    int lines;                  /* 0x1c: draw the connecting lines */
    int count;                  /* 0x20 */
    int *open;                  /* 0x24: the items that are open */
} TREE;

typedef void (*DRAWFUNC)(HDC hdc, RECT *r, int index);

extern char g_levelFile[200];
extern int g_dlgReady;
extern DATABASE *g_db;
extern int g_dbIndex;
extern DRAWFUNC g_drawFuncs[13];
extern char g_selPath[80];
extern HBRUSH g_gameDlgBrush;
extern HBITMAP g_bitmapDlgOld;
extern int g_savedDbIndexNowTemp, g_savedLevelNowCur;
extern int g_infoLsSkills[8];
extern int g_lsLemmingsSpareMain;
extern HBITMAP g_newPreview;
extern HPALETTE g_dlgOldPalette;
extern HPALETTE g_hereDlgPalette;
extern int g_spareLsTime;
extern char g_lsName[52];
extern int g_lastLsCustom;
extern HWND g_ownPreviewWnd;
extern int g_stateLsRate;
extern HDC g_dlgDC2New;
extern TREE g_treeNext;
extern HPALETTE g_previewPalNewUsed;
extern int g_lsRescueState;
extern BYTE g_lvlBuf[0x800];
extern HDC g_dlgDC;
extern HBITMAP g_oldDlgBitmap2;
extern char g_selNameInfoOwn[80];
extern HBITMAP g_dlgBitmapAll;
int InitLevelTree(void);
int RegisterIconClasses(HINSTANCE hInstance);
int SelectLevelDialog(HWND hwnd);
int LoadDatabase(void);
void SaveDatabase(void);
DBENTRY *FindRating(int i);
int LevelNumber(int i);
void CloseLevelDialog(HWND hDlg, int ok);
BOOL CALLBACK LevelDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
DBENTRY *FindNextGroup(int i);
int NextIsLevel(int i);
int LevelDone(int i);
int LevelCheated(int i);
int GroupDone(int i);
int RatingDone(int i);
int ItemDone(int i);
void DrawLevelItem(HWND hDlg, DRAWITEMSTRUCT *dis);
int FillLevelList(HWND hDlg, WPARAM wParam, LPARAM lParam);
void LevelDlgCommand(HWND hDlg, int id, HWND ctl, int code);
void SelectLevelItem(HWND hDlg, HWND list, int i, WORD sel);
int LevelIndex(int n);
void OpenLevelItem(HWND hDlg, HWND list, int i, WORD sel);
void DrawBlank(HDC hdc, RECT *r, int index);
void DrawDialogIcon(HDC hdc, RECT *r, int index);
void GetBitmapSize(HBITMAP bm, int *w, int *h);
void DrawPreview(HDC hdc, RECT *r, int index);
LRESULT CALLBACK IconImageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void DrawIconText(HWND hwnd, HDC hdc, RECT *r);
LRESULT CALLBACK IconTextProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void SetPreviewPalette(HPALETTE pal, int n);
void ShowLevelDetails(HWND hDlg, int i, char *file, char *bitmap);
void LevelSelectNop(void);
void ClampLevel(void);
int IsLastLevel(void);
int NextLevelNumber(int n);
void NextLevel(void);
void UndoNextLevel(void);
void SetLevel(int i);
void PlayLevel(HWND hwnd);

/* treelist.cpp */
void TreeFreeBitmaps(TREE *t);
void TreeFreeOpen(TREE *t);
void TreeMeasureItem(HWND hDlg, MEASUREITEMSTRUCT *mis, TREE *t);
void TreeSetFont(HWND list, HFONT font, TREE *t);
COLORREF SwapRGB(COLORREF c);
int TreeLoadBitmaps(HINSTANCE hInst, int id, int rows, int cols, int lines, TREE *t, int reset);
void TreeDrawItem(HWND hDlg, DRAWITEMSTRUCT *dis, int level, int lines, char *text, int row, int col, TREE *t);
void FastRect(HDC hdc, int x, int y, int w, int h);
int TreeIsOpen(TREE *t, int id);
void TreeOpen(TREE *t, int id);
void TreeClose(TREE *t, int id);
void TreeEnsureVisible(TREE *t, HWND list, WORD sel, WORD n);

/* midi.cpp */
extern int g_musicOn;
extern UINT g_midiDevice;
extern int g_midiPlaying;
extern DWORD g_midiPos;
extern HWND g_midiNotify;
extern int g_tune, g_tuneCount;
void SetMusic(int on);
int HaveMidi(void);
DWORD OpenMidi(char *file);
DWORD CloseMidi(void);
DWORD PauseMidi(void);
DWORD RestartMidi(void);
DWORD PlayMidi(HWND hwnd);
char *GetLevelMusic(void);
void NextTune(void);
void RandomTune(void);

/* splash.cpp */
extern HFONT g_splashFont;
extern HWND g_splashWnd;
extern HBITMAP g_splashBitmap;
extern HPALETTE g_splashPal;
extern char g_splashClass[];
extern int g_splashH, g_splashW;
extern char g_splashTextGame[128];
LRESULT CALLBACK SplashProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int RegisterSplashClass(HINSTANCE hInstance);
HWND CreateSplash(HINSTANCE hInstance);
void ShowSplash(HINSTANCE hInstance);
void HideSplash(void);

/* replay.cpp */
extern REPLAYEVENT *g_replayPtrVal;
extern int g_recordingMain;
extern REPLAYEVENT *g_replayEnd;
extern int g_replaySize;
extern REPLAYEVENT *g_mainReplayBuf;
extern int g_newReplayActive;
void InitPlayback(int size);
void QuitPlayback(void);
int IsRecording(void);
int IsReplaying(void);
int RewindReplay(void);
int StopReplayBuffer(void);
int StartRecord(void);
void StopReplay(void);
int EndRecord(void);
int StartReplay(void);
int EndReplay(void);
int RecordEvent(int time, int type, int x, int y, int value, int priority, int unused18, int unused1c);
int RecordSkill(int time, int x, int y, int skill, int priority);
int RecordScroll(int time, int x, int y);
int RecordRate(int time, int rate);
int RecordNuke(int time);
int GetReplayEvent(int time, REPLAYEVENT *ev);

/* defined in frame.cpp (g_clientWidth/Height) and settings.cpp (the rest) */
extern int g_clientWidth;                       /* 0x416680 */
extern int g_valClientHeight;                      /* 0x416698 */
extern RECT g_rectStatus;                       /* 0x4158a0: x, y, w, h */
extern RECT g_viewRectSpare;                         /* 0x4158e8: x, y, w, h */
extern int g_mapDock;                           /* 0x4158b0 */
extern int g_mapXUsed, g_usedMapY;              /* 0x4158b4 */
extern int g_savedIconDock;                          /* 0x415978 */
extern int g_iconXInfo, g_mainIconY;            /* 0x4158dc */
extern int g_resolutionNew;                        /* 0x4158d8 */
int CalcStatusHeight(void);                     /* 0x404880 */
int GetDocksHeight(void);                       /* 0x403620 */

#endif
