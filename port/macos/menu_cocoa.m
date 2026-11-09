/*
 * macOS: the menu bar. The application menu (About, Hide, Quit) comes first;
 * the menu of the window that has one (the game's frame) follows it. Each of
 * its drop-down menus is filled from the layer's menu as it opens (menu.c's
 * menu_native_*: WM_INITMENU and WM_INITMENUPOPUP first, so the game can
 * enable and check items), highlighting an item sends WM_MENUSELECT (the
 * game's status-bar help) and choosing one posts WM_COMMAND.
 */
#include "cocoa_win32.h"

@interface PortMenu : NSMenu
@property(nonatomic) uint32_t hmenu;        /* the layer's menu */
@property(nonatomic) uint32_t parent;       /* the menu it opens from */
@property(nonatomic) int pos;               /* its item there */
@end

@implementation PortMenu
@end

@interface PortMenuDelegate : NSObject <NSMenuDelegate>
@end

static PortMenuDelegate *g_menuDelegate;
static uint32_t g_owner, g_bar;             /* the window whose menu is shown, and that menu */
static NSMutableArray *g_barItems;          /* its items in the menu bar */

static NSString *ns(const char *utf8)
{
    return [NSString stringWithUTF8String:utf8] ?: @"";
}

static PortMenu *new_menu(NSString *title, uint32_t hmenu, uint32_t parent, int pos)
{
    PortMenu *m = [[PortMenu alloc] initWithTitle:title];

    m.hmenu = hmenu;
    m.parent = parent;
    m.pos = pos;
    m.delegate = g_menuDelegate;
    m.autoenablesItems = NO;
    return m;
}

@implementation PortMenuDelegate
- (void)menuNeedsUpdate:(NSMenu *)menu
{
    PortMenu *m = (PortMenu *)menu;
    int i, n, live;

    if (![m isKindOfClass:[PortMenu class]])
        return;
    live = g_owner && menu_native_open(g_owner, m.parent, m.pos, m.hmenu);
    n = menu_native_count(m.hmenu);
    [m removeAllItems];
    for (i = 0; i < n; i++) {
        char text[512];
        unsigned id, flags;
        uint32_t sub;
        NSMenuItem *it;

        if (!menu_native_item(m.hmenu, i, text, sizeof text, &id, &flags, &sub))
            continue;
        if (flags & MF_SEPARATOR) {
            [m addItem:[NSMenuItem separatorItem]];
            continue;
        }
        it = [[NSMenuItem alloc] initWithTitle:ns(text) action:sub ? NULL : @selector(choose:) keyEquivalent:@""];
        it.target = self;
        it.tag = (NSInteger)id;
        it.enabled = live && !(flags & (MF_GRAYED | MF_DISABLED));
        it.state = (flags & MF_CHECKED) ? NSControlStateValueOn : NSControlStateValueOff;
        if (sub)
            it.submenu = new_menu(ns(text), sub, m.hmenu, i);
        [m addItem:it];
    }
}
- (BOOL)menuHasKeyEquivalent:(NSMenu *)menu forEvent:(NSEvent *)event target:(id *)target action:(SEL *)action
{
    return NO;                              /* the game's accelerators do that */
}
- (void)menu:(NSMenu *)menu willHighlightItem:(NSMenuItem *)item
{
    PortMenu *m = (PortMenu *)menu;
    NSInteger i;

    if (![m isKindOfClass:[PortMenu class]] || !item || !g_owner)
        return;
    i = [m indexOfItem:item];
    if (item.submenu)
        menu_native_select(g_owner, m.hmenu, (unsigned)i, MF_POPUP | MF_HILITE | MF_MOUSESELECT);
    else
        menu_native_select(g_owner, m.hmenu, (unsigned)item.tag,
                           MF_HILITE | MF_MOUSESELECT | (item.enabled ? 0 : MF_GRAYED));
}
- (void)choose:(NSMenuItem *)item
{
    if (g_owner)
        menu_native_command(g_owner, (unsigned)item.tag);
}
- (void)trackingEnded:(NSNotification *)n
{
    if (g_owner)
        menu_native_close(g_owner);
}
- (void)idle:(NSTimer *)t
{
    input_tracking_idle();
}
@end

int plat_menubar_native(void)
{
    return 1;
}

void plat_menubar_set(uint32_t hwnd, uint32_t hmenu)
{
    @autoreleasepool {
        NSMenu *bar = NSApp.mainMenu;
        int i, n;

        if (!hmenu && hwnd != g_owner)
            return;
        for (NSMenuItem *it in g_barItems)
            [bar removeItem:it];
        [g_barItems removeAllObjects];
        NSApp.helpMenu = nil;
        g_owner = hmenu ? hwnd : 0;
        g_bar = hmenu;
        n = hmenu ? menu_native_count(hmenu) : 0;
        for (i = 0; i < n; i++) {
            char text[512];
            unsigned id, flags;
            uint32_t sub;
            NSMenuItem *it;

            if (!menu_native_item(hmenu, i, text, sizeof text, &id, &flags, &sub) || !sub)
                continue;           /* the bar holds drop-down menus only */
            it = [[NSMenuItem alloc] initWithTitle:ns(text) action:NULL keyEquivalent:@""];
            it.submenu = new_menu(ns(text), sub, hmenu, i);
            it.enabled = !(flags & (MF_GRAYED | MF_DISABLED));
            [bar addItem:it];
            [g_barItems addObject:it];
            if (!strcmp(text, "Help"))
                NSApp.helpMenu = it.submenu;
        }
    }
}

static void dump(NSMenu *m, int depth)
{
    if (m.delegate)
        [m.delegate menuNeedsUpdate:m];
    for (NSMenuItem *it in m.itemArray) {
        port_harness_log("  %*s%s%s%s", depth * 2, "", it.separatorItem ? "----" : it.title.UTF8String,
                         it.enabled || it.submenu ? "" : " (disabled)",
                         it.state == NSControlStateValueOn ? " (checked)" : "");
        if (it.submenu)
            dump(it.submenu, depth + 1);
    }
    if ([m isKindOfClass:[PortMenu class]] && g_owner)
        menu_native_close(g_owner);
}

/* the harness: "File/Exit" opens File (as the system would), highlights and
   chooses Exit; "?" lists the whole menu bar */
int plat_menu_choose(const char *path)
{
    @autoreleasepool {
        if (!strcmp(path, "?")) {
            dump(NSApp.mainMenu, 0);
            return 1;
        }
        NSArray *parts = [ns(path) componentsSeparatedByString:@"/"];
        NSMenu *m = NSApp.mainMenu;
        NSInteger i;
        int ok = 0;

        for (NSString *part in parts) {
            NSMenuItem *it = nil;

            if (m.delegate)
                [m.delegate menuNeedsUpdate:m];
            if (m == NSApp.mainMenu && [part isEqualToString:@"App"])
                it = m.itemArray.firstObject;
            else
                it = [m itemWithTitle:part];
            if (!it || !it.enabled)
                break;
            if (it.submenu) {
                m = it.submenu;
                continue;
            }
            if ([m.delegate respondsToSelector:@selector(menu:willHighlightItem:)])
                [m.delegate menu:m willHighlightItem:it];
            i = [m indexOfItem:it];
            [m performActionForItemAtIndex:i];
            ok = 1;
            break;
        }
        if (g_owner)
            menu_native_close(g_owner);
        return ok;
    }
}

void cocoa_menu_init(id appDelegate)
{
    NSMenu *bar = [NSMenu new], *app = [NSMenu new];
    NSMenuItem *item = [NSMenuItem new], *i;
    NSTimer *t;

    g_menuDelegate = [PortMenuDelegate new];
    g_barItems = [NSMutableArray new];
    item.submenu = app;
    [bar addItem:item];
    i = [app addItemWithTitle:@"About Lemmings" action:@selector(about:) keyEquivalent:@""];
    i.target = appDelegate;
    [app addItem:[NSMenuItem separatorItem]];
    [app addItemWithTitle:@"Hide Lemmings" action:@selector(hide:) keyEquivalent:@"h"];
    i = [app addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
    i.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagOption;
    [app addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
    [app addItem:[NSMenuItem separatorItem]];
    [app addItemWithTitle:@"Quit Lemmings" action:@selector(terminate:) keyEquivalent:@"q"];
    NSApp.mainMenu = bar;
    [[NSNotificationCenter defaultCenter] addObserver:g_menuDelegate selector:@selector(trackingEnded:)
                                                 name:NSMenuDidEndTrackingNotification object:bar];
    /* the game's loop waits while AppKit tracks a menu (or a window drag): the
       status bar's menu help still reaches the screen */
    t = [NSTimer timerWithTimeInterval:0.05 target:g_menuDelegate selector:@selector(idle:) userInfo:nil
                               repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:t forMode:NSEventTrackingRunLoopMode];
}
