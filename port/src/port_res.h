/*
 * The game's resources as C tables, generated from lemmings.rc by
 * tools/port/rc2c.py. A resource has a name (upper case) or a number (-1 when
 * named). Each table ends with an entry whose pointer fields are NULL.
 */
#ifndef PORT_RES_H
#define PORT_RES_H

#include <windows.h>

typedef struct PortResFile {        /* BITMAP, CURSOR, ICON: the file's bytes */
    const char *type;
    const char *name;
    int id;
    const unsigned char *data;
    unsigned size;
} PortResFile;

typedef struct PortResString {
    unsigned id;
    const char *text;
} PortResString;

typedef struct PortResControl {
    const char *text;
    int id;
    const char *cls;
    DWORD style;
    short x, y, cx, cy;
} PortResControl;

typedef struct PortResDialog {
    const char *name;
    int id;
    DWORD style;
    short x, y, cx, cy;
    const char *caption;
    int fontSize;
    const char *font;
    int count;
    const PortResControl *controls;
} PortResDialog;

typedef struct PortResMenuItem {
    const char *text;
    int id;
    int flags;                      /* MF_POPUP, MF_SEPARATOR */
    int count;                      /* a popup's items */
    const struct PortResMenuItem *items;
} PortResMenuItem;

typedef struct PortResMenu {
    const char *name;
    int id;
    int count;
    const PortResMenuItem *items;
} PortResMenu;

typedef struct PortResAccel {
    int flags;                      /* FVIRTKEY, FSHIFT, FCONTROL, FALT */
    int key;
    int cmd;
} PortResAccel;

typedef struct PortResAccelTable {
    const char *name;
    int id;
    int count;
    const PortResAccel *accels;
} PortResAccelTable;

typedef struct PortResVersion {     /* StringFileInfo values */
    const char *lang;
    const char *key;
    const char *value;
} PortResVersion;

extern const PortResFile port_res_files[];
extern const PortResString port_res_strings[];
extern const PortResDialog port_res_dialogs[];
extern const PortResMenu port_res_menus[];
extern const PortResAccelTable port_res_accels[];
extern const PortResVersion port_res_version[];

#endif
