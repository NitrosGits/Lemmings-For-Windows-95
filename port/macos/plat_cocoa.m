/*
 * macOS: plat.h on AppKit. Each top-level Win32 window is an NSWindow whose
 * content view shows the layer's framebuffer as a Core Animation layer
 * (nearest-neighbour scaled on Retina screens); native controls are subviews
 * of it (controls_cocoa.m). The layer's own message loop pumps AppKit's
 * events (no [NSApp run]); mouse, keyboard and window changes go to input_*.
 *
 * Screen coordinates for the layer have their origin at the top left of the
 * main display (screens[0]); AppKit's are bottom-left. Both use points.
 */
#import <QuartzCore/QuartzCore.h>
#include <mach/mach_time.h>
#include <sys/stat.h>

#include "cocoa_win32.h"
#include "keycodes.h"

static NSMutableArray *g_injected;          /* the harness's events' timestamps */
static unichar g_keyChar[128];              /* what each key typed last (TranslateMessage) */

static CGFloat screen0_height(void)
{
    NSArray *s = [NSScreen screens];

    return s.count ? ((NSScreen *)s[0]).frame.size.height : 800;
}

static NSRect cocoa_rect(int x, int y, int w, int h)
{
    return NSMakeRect(x, screen0_height() - y - h, w, h);
}

/* ---- The views and windows ---- */

@implementation PortWindow
- (BOOL)canBecomeKeyWindow
{
    return YES;
}
- (BOOL)canBecomeMainWindow
{
    return (self.styleMask & NSWindowStyleMaskTitled) != 0;
}
@end

/* The harness's events: AppKit hands back a copy of a posted event, with its
   timestamp rounded to nanoseconds, so they are known by timestamps at least
   10 us apart, which plat_pump looks up while it sends them */
static int g_sendingInjected;

static int injected(NSEvent *e)
{
    (void)e;
    return g_sendingInjected;
}

static NSTimeInterval inject_stamp(void)
{
    static NSTimeInterval last;
    NSTimeInterval t = [NSProcessInfo processInfo].systemUptime;

    if (t < last + 1e-5)
        t = last + 1e-5;
    last = t;
    /* an event a control's own tracking loop took never comes back here */
    while (g_injected.count && [g_injected[0] doubleValue] < t - 10)
        [g_injected removeObjectAtIndex:0];
    [g_injected addObject:@(t)];
    return t;
}

static int take_injected(NSTimeInterval t)
{
    for (NSNumber *n in g_injected)
        if (fabs(n.doubleValue - t) < 2e-6) {
            [g_injected removeObject:n];
            return 1;
        }
    return 0;
}

static int mods_of(NSEventModifierFlags f)
{
    return ((f & NSEventModifierFlagShift) ? PM_SHIFT : 0) | ((f & NSEventModifierFlagControl) ? PM_CTRL : 0) |
           ((f & NSEventModifierFlagOption) ? PM_ALT : 0) | ((f & NSEventModifierFlagCapsLock) ? PM_CAPS : 0) |
           ((f & NSEventModifierFlagCommand) ? PM_CMD : 0);
}

static int vk_of_keycode(unsigned short k)
{
    static const struct {
        unsigned short code;
        int vk;
    } map[] = {
        {kVK_ANSI_A, 'A'}, {kVK_ANSI_B, 'B'}, {kVK_ANSI_C, 'C'}, {kVK_ANSI_D, 'D'}, {kVK_ANSI_E, 'E'},
        {kVK_ANSI_F, 'F'}, {kVK_ANSI_G, 'G'}, {kVK_ANSI_H, 'H'}, {kVK_ANSI_I, 'I'}, {kVK_ANSI_J, 'J'},
        {kVK_ANSI_K, 'K'}, {kVK_ANSI_L, 'L'}, {kVK_ANSI_M, 'M'}, {kVK_ANSI_N, 'N'}, {kVK_ANSI_O, 'O'},
        {kVK_ANSI_P, 'P'}, {kVK_ANSI_Q, 'Q'}, {kVK_ANSI_R, 'R'}, {kVK_ANSI_S, 'S'}, {kVK_ANSI_T, 'T'},
        {kVK_ANSI_U, 'U'}, {kVK_ANSI_V, 'V'}, {kVK_ANSI_W, 'W'}, {kVK_ANSI_X, 'X'}, {kVK_ANSI_Y, 'Y'},
        {kVK_ANSI_Z, 'Z'}, {kVK_ANSI_0, '0'}, {kVK_ANSI_1, '1'}, {kVK_ANSI_2, '2'}, {kVK_ANSI_3, '3'},
        {kVK_ANSI_4, '4'}, {kVK_ANSI_5, '5'}, {kVK_ANSI_6, '6'}, {kVK_ANSI_7, '7'}, {kVK_ANSI_8, '8'},
        {kVK_ANSI_9, '9'}, {kVK_Return, VK_RETURN}, {kVK_ANSI_KeypadEnter, VK_RETURN}, {kVK_Escape, VK_ESCAPE},
        {kVK_Delete, VK_BACK}, {kVK_ForwardDelete, VK_DELETE}, {kVK_Tab, VK_TAB}, {kVK_Space, VK_SPACE},
        {kVK_LeftArrow, VK_LEFT}, {kVK_RightArrow, VK_RIGHT}, {kVK_UpArrow, VK_UP}, {kVK_DownArrow, VK_DOWN},
        {kVK_Home, VK_HOME}, {kVK_End, VK_END}, {kVK_PageUp, VK_PRIOR}, {kVK_PageDown, VK_NEXT},
        {kVK_Help, VK_INSERT}, {kVK_F1, VK_F1}, {kVK_F2, VK_F1 + 1}, {kVK_F3, VK_F1 + 2}, {kVK_F4, VK_F1 + 3},
        {kVK_F5, VK_F1 + 4}, {kVK_F6, VK_F1 + 5}, {kVK_F7, VK_F1 + 6}, {kVK_F8, VK_F1 + 7}, {kVK_F9, VK_F1 + 8},
        {kVK_F10, VK_F1 + 9}, {kVK_F11, VK_F1 + 10}, {kVK_F12, VK_F1 + 11}, {kVK_ANSI_Keypad0, VK_NUMPAD0},
        {kVK_ANSI_Keypad1, VK_NUMPAD0 + 1}, {kVK_ANSI_Keypad2, VK_NUMPAD0 + 2}, {kVK_ANSI_Keypad3, VK_NUMPAD0 + 3},
        {kVK_ANSI_Keypad4, VK_NUMPAD0 + 4}, {kVK_ANSI_Keypad5, VK_NUMPAD0 + 5}, {kVK_ANSI_Keypad6, VK_NUMPAD0 + 6},
        {kVK_ANSI_Keypad7, VK_NUMPAD0 + 7}, {kVK_ANSI_Keypad8, VK_NUMPAD0 + 8}, {kVK_ANSI_Keypad9, VK_NUMPAD0 + 9},
        {kVK_ANSI_KeypadPlus, VK_ADD}, {kVK_ANSI_KeypadMinus, VK_SUBTRACT}, {kVK_ANSI_KeypadMultiply, VK_MULTIPLY},
        {kVK_ANSI_KeypadDivide, VK_DIVIDE}, {kVK_ANSI_KeypadDecimal, VK_DECIMAL}, {kVK_ANSI_Minus, 0xBD},
        {kVK_ANSI_Equal, 0xBB}, {kVK_ANSI_Comma, 0xBC}, {kVK_ANSI_Period, 0xBE}, {kVK_ANSI_Slash, 0xBF},
        {kVK_ANSI_Semicolon, 0xBA}, {kVK_ANSI_Grave, 0xC0}, {kVK_ANSI_LeftBracket, 0xDB},
        {kVK_ANSI_Backslash, 0xDC}, {kVK_ANSI_RightBracket, 0xDD}, {kVK_ANSI_Quote, 0xDE},
        {kVK_Shift, VK_SHIFT}, {kVK_RightShift, VK_SHIFT}, {kVK_Control, VK_CONTROL},
        {kVK_RightControl, VK_CONTROL}, {kVK_Option, VK_MENU}, {kVK_RightOption, VK_MENU},
    };
    size_t i;

    for (i = 0; i < sizeof map / sizeof map[0]; i++)
        if (map[i].code == k)
            return map[i].vk;
    return 0;
}

static int keycode_of_vk(int vk)
{
    int k;

    for (k = 0; k < 128; k++)
        if (vk_of_keycode((unsigned short)k) == vk)
            return k;
    return -1;
}

@implementation PortView
- (BOOL)isFlipped
{
    return YES;
}
- (BOOL)acceptsFirstResponder
{
    return YES;
}
- (BOOL)acceptsFirstMouse:(NSEvent *)e
{
    return YES;
}
- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    if (!self.trackingAreas.count)
        [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                           options:NSTrackingMouseMoved | NSTrackingCursorUpdate |
                                                                   NSTrackingActiveAlways | NSTrackingInVisibleRect
                                                             owner:self userInfo:nil]];
}
- (BOOL)wantsUpdateLayer
{
    return YES;
}
- (void)updateLayer
{
}
- (void)cursorUpdate:(NSEvent *)e
{
    HCURSOR c = GetCursor();

    if (c)
        cursor_apply(c);
}
- (void)mouse:(NSEvent *)e button:(int)b down:(int)down
{
    NSPoint p = [self convertPoint:e.locationInWindow fromView:nil];

    input_mouse(self.plat->wnd, (int)floor(p.x), (int)floor(p.y), b, down, (int)e.clickCount, injected(e));
}
- (void)mouseDown:(NSEvent *)e
{
    [self mouse:e button:0 down:1];
}
- (void)mouseUp:(NSEvent *)e
{
    [self mouse:e button:0 down:0];
}
- (void)rightMouseDown:(NSEvent *)e
{
    [self mouse:e button:1 down:1];
}
- (void)rightMouseUp:(NSEvent *)e
{
    [self mouse:e button:1 down:0];
}
- (void)otherMouseDown:(NSEvent *)e
{
    [self mouse:e button:2 down:1];
}
- (void)otherMouseUp:(NSEvent *)e
{
    [self mouse:e button:2 down:0];
}
- (void)moved:(NSEvent *)e
{
    static NSPoint last;
    NSPoint p = [self convertPoint:e.locationInWindow fromView:nil];
    int inj = injected(e);

    /* the tracking area and the window both send moves: one each */
    if (!inj && NSEqualPoints(p, last))
        return;
    last = p;
    input_mouse_move(self.plat->wnd, (int)floor(p.x), (int)floor(p.y), inj);
}
- (void)mouseMoved:(NSEvent *)e
{
    [self moved:e];
}
- (void)mouseDragged:(NSEvent *)e
{
    [self moved:e];
}
- (void)rightMouseDragged:(NSEvent *)e
{
    [self moved:e];
}
- (void)otherMouseDragged:(NSEvent *)e
{
    [self moved:e];
}
- (void)scrollWheel:(NSEvent *)e
{
    input_wheel((int)(e.scrollingDeltaY * (e.hasPreciseScrollingDeltas ? 4 : 40)), injected(e));
}
- (void)key:(NSEvent *)e down:(int)down
{
    unsigned short k = e.keyCode;
    int mods = mods_of(e.modifierFlags);

    if (down && k < 128) {
        NSString *s = (mods & PM_ALT) ? e.charactersIgnoringModifiers : e.characters;

        g_keyChar[k] = s.length ? [s characterAtIndex:0] : 0;
    }
    input_key(vk_of_keycode(k), k, down, down && e.isARepeat, mods, injected(e));
}
- (void)keyDown:(NSEvent *)e
{
    [self key:e down:1];
}
- (void)flagsChanged:(NSEvent *)e
{
    NSEventModifierFlags bit;

    switch (e.keyCode) {
    case kVK_Shift:
    case kVK_RightShift:
        bit = NSEventModifierFlagShift;
        break;
    case kVK_Control:
    case kVK_RightControl:
        bit = NSEventModifierFlagControl;
        break;
    case kVK_Option:
    case kVK_RightOption:
        bit = NSEventModifierFlagOption;
        break;
    default:
        return;
    }
    input_key(vk_of_keycode(e.keyCode), e.keyCode, (e.modifierFlags & bit) != 0, 0, mods_of(e.modifierFlags),
              injected(e));
}
- (void)keyUp:(NSEvent *)e
{
    [self key:e down:0];
}
@end

@interface PortWindowDelegate : NSObject <NSWindowDelegate>
@end

static Wnd *wnd_of_window(NSWindow *w)
{
    return [w isKindOfClass:[PortWindow class]] && ((PortWindow *)w).plat ? ((PortWindow *)w).plat->wnd : NULL;
}

static void content_origin(NSWindow *w, int *x, int *y)
{
    NSRect c = [w contentRectForFrameRect:w.frame];

    *x = (int)c.origin.x;
    *y = (int)(screen0_height() - c.origin.y - c.size.height);
}

@implementation PortWindowDelegate
- (void)windowDidMove:(NSNotification *)n
{
    int x, y;

    content_origin(n.object, &x, &y);
    input_window_moved(wnd_of_window(n.object), x, y);
}
- (void)windowDidResize:(NSNotification *)n
{
    NSWindow *w = n.object;
    NSRect c = [w contentRectForFrameRect:w.frame];
    int x, y;

    content_origin(w, &x, &y);
    input_window_moved(wnd_of_window(w), x, y);
    input_window_resized(wnd_of_window(w), (int)c.size.width, (int)c.size.height);
    if (!w.miniaturized)
        input_window_state(wnd_of_window(w), w.zoomed ? SIZE_MAXIMIZED : SIZE_RESTORED);
}
- (void)windowDidMiniaturize:(NSNotification *)n
{
    input_window_state(wnd_of_window(n.object), SIZE_MINIMIZED);
}
- (void)windowDidDeminiaturize:(NSNotification *)n
{
    input_window_state(wnd_of_window(n.object), SIZE_RESTORED);
}
- (BOOL)windowShouldClose:(NSWindow *)w
{
    input_window_close(wnd_of_window(w));
    return NO;
}
- (void)windowDidBecomeKey:(NSNotification *)n
{
    input_window_focus(wnd_of_window(n.object));
}
@end

static PortWindowDelegate *g_delegate;

/* ---- The application ---- */

@interface PortAppDelegate : NSObject <NSApplicationDelegate>
@end

static PortAppDelegate *g_appDelegate;

@implementation PortAppDelegate
/* Quit: the game decides (WM_CLOSE to its main window) and exits by itself.
   Not NSTerminateLater: terminate: would then run a modal loop of its own
   until the answer, and the game would wait in it. So a logout or restart
   reports that Lemmings cancelled it, while the game quits. */
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)a
{
    input_quit();
    return NSTerminateCancel;
}
- (void)about:(id)sender
{
    input_command(320);             /* Help > About Lemmings */
}
@end

void plat_init(void)
{
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSApp.appearance = [NSAppearance appearanceNamed:NSAppearanceNameAqua];
        g_appDelegate = [PortAppDelegate new];
        NSApp.delegate = g_appDelegate;
        g_delegate = [PortWindowDelegate new];
        g_injected = [NSMutableArray new];
        cocoa_menu_init(g_appDelegate);
        [NSApp finishLaunching];
        [NSApp activateIgnoringOtherApps:YES];
    }
}

void plat_quit(void)
{
}

uint64_t plat_ticks(void)
{
    static mach_timebase_info_data_t tb;
    static uint64_t start;
    uint64_t now = mach_absolute_time();

    if (!tb.denom) {
        mach_timebase_info(&tb);
        start = now;
    }
    return (now - start) * tb.numer / tb.denom / 1000000;
}

void plat_sleep(uint32_t ms)
{
    usleep(ms * 1000);
}

static char *dir_string(NSString *s)
{
    const char *u = s.fileSystemRepresentation;
    size_t n = strlen(u);
    char *r = malloc(n + 2);

    memcpy(r, u, n);
    if (!n || r[n - 1] != '/')
        r[n++] = '/';
    r[n] = 0;
    return r;
}

char *plat_pref_dir(void)
{
    @autoreleasepool {
        NSArray *a = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        NSString *d;

        if (!a.count)
            return NULL;
        d = [a[0] stringByAppendingPathComponent:@"Visual Sciences/Lemmings"];
        [[NSFileManager defaultManager] createDirectoryAtPath:d withIntermediateDirectories:YES attributes:nil
                                                        error:nil];
        return dir_string(d);
    }
}

char *plat_base_dir(void)
{
    @autoreleasepool {
        /* an app bundle's Resources folder, else the executable's folder */
        return dir_string([NSBundle mainBundle].resourcePath);
    }
}

uint64_t plat_ram_bytes(void)
{
    return [NSProcessInfo processInfo].physicalMemory;
}

int plat_open_url(const char *url)
{
    @autoreleasepool {
        NSURL *u = [NSURL URLWithString:[NSString stringWithUTF8String:url]];

        return u && [[NSWorkspace sharedWorkspace] openURL:u];
    }
}

void plat_usable_bounds(int *x, int *y, int *w, int *h)
{
    NSArray *s = [NSScreen screens];
    NSRect v = s.count ? ((NSScreen *)s[0]).visibleFrame : NSMakeRect(0, 0, 1280, 800);

    *x = (int)v.origin.x;
    *y = (int)(screen0_height() - v.origin.y - v.size.height);
    *w = (int)v.size.width;
    *h = (int)v.size.height;
}

int plat_message_box(PlatWindow *owner, const char *text, const char *caption, unsigned type)
{
    @autoreleasepool {
        NSAlert *a;

        [NSApplication sharedApplication];      /* port_fatal can come before plat_init */
        a = [NSAlert new];
        int ids[3], n = 0;
        NSModalResponse r;

        a.messageText = [NSString stringWithUTF8String:caption ? caption : ""] ?: @"";
        a.informativeText = [NSString stringWithUTF8String:text ? text : ""] ?: @"";
        a.alertStyle = (type & 0xf0) == MB_ICONHAND ? NSAlertStyleCritical
                     : (type & 0xf0) == MB_ICONEXCLAMATION ? NSAlertStyleWarning : NSAlertStyleInformational;
        switch (type & 0xf) {
        case MB_OKCANCEL:
            [a addButtonWithTitle:@"OK"];
            [a addButtonWithTitle:@"Cancel"];
            ids[n++] = IDOK;
            ids[n++] = IDCANCEL;
            break;
        case MB_YESNO:
            [a addButtonWithTitle:@"Yes"];
            [a addButtonWithTitle:@"No"];
            ids[n++] = IDYES;
            ids[n++] = IDNO;
            break;
        case MB_YESNOCANCEL:
            [a addButtonWithTitle:@"Yes"];
            [a addButtonWithTitle:@"No"];
            [a addButtonWithTitle:@"Cancel"];
            ids[n++] = IDYES;
            ids[n++] = IDNO;
            ids[n++] = IDCANCEL;
            break;
        default:
            [a addButtonWithTitle:@"OK"];
            ids[n++] = IDOK;
            break;
        }
        (void)owner;
        r = [a runModal];
        r -= NSAlertFirstButtonReturn;
        return r >= 0 && r < n ? ids[r] : ids[n - 1];
    }
}

/* ---- Windows ---- */

PlatWindow *plat_window_create(Wnd *w, int x, int y, int cw, int ch, int flags, PlatWindow *owner,
                               const char *title)
{
    @autoreleasepool {
        NSWindowStyleMask style = NSWindowStyleMaskBorderless;
        PlatWindow *p = calloc(1, sizeof *p);
        PortWindow *win;
        PortView *v;

        if (flags & PW_CAPTION) {
            style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
            if (!(flags & PW_DIALOG))
                style |= NSWindowStyleMaskMiniaturizable;
            if (flags & PW_RESIZABLE)
                style |= NSWindowStyleMaskResizable;
        }
        win = [[PortWindow alloc] initWithContentRect:cocoa_rect(x, y, cw, ch) styleMask:style
                                              backing:NSBackingStoreBuffered defer:NO];
        win.plat = p;
        win.releasedWhenClosed = NO;
        win.delegate = g_delegate;
        win.acceptsMouseMovedEvents = YES;
        win.title = [NSString stringWithUTF8String:title] ?: @"";
        if (flags & PW_TOPMOST)
            win.level = NSFloatingWindowLevel;
        if (!(flags & PW_CAPTION))
            win.hasShadow = NO;
        win.collectionBehavior = (flags & (PW_UTILITY | PW_DIALOG)) ? NSWindowCollectionBehaviorFullScreenAuxiliary
                                                                   : NSWindowCollectionBehaviorFullScreenPrimary;
        v = [[PortView alloc] initWithFrame:NSMakeRect(0, 0, cw, ch)];
        v.plat = p;
        v.wantsLayer = YES;
        v.layerContentsRedrawPolicy = NSViewLayerContentsRedrawNever;
        v.layer.magnificationFilter = kCAFilterNearest;
        v.layer.minificationFilter = kCAFilterNearest;
        v.layer.contentsGravity = kCAGravityResize;
        v.layer.backgroundColor = CGColorGetConstantColor(kCGColorBlack);
        win.contentView = v;
        [win makeFirstResponder:v];
        p->win = win;
        p->view = v;
        p->wnd = w;
        p->owner = owner;
        p->flags = flags;
        return p;
    }
}

void plat_window_destroy(PlatWindow *p)
{
    if (!p)
        return;
    @autoreleasepool {
        if (p->owner && p->showing && p->win.parentWindow)
            [p->win.parentWindow removeChildWindow:p->win];
        if (p->wnd)
            plat_menubar_set((uint32_t)(uintptr_t)p->wnd->h, 0);
        p->win.plat = NULL;
        p->view.plat = NULL;
        [p->win orderOut:nil];
        p->win.delegate = nil;
        cocoa_bury(p->win);     /* closed at the next pump: a button of it may be running its action */
        p->win = nil;
        p->view = nil;
    }
    free(p);
}

void plat_window_show(PlatWindow *p, int show, int activate)
{
    @autoreleasepool {
        if (show) {
            if (p->owner && p->owner->showing && !p->win.parentWindow)
                [p->owner->win addChildWindow:p->win ordered:NSWindowAbove];
            if (activate || (p->flags & (PW_CAPTION | PW_DIALOG)))
                [p->win makeKeyAndOrderFront:nil];
            else
                [p->win orderFront:nil];
            p->showing = 1;
            {
                NSRect c = [p->win contentRectForFrameRect:p->win.frame];
                int x, y;

                content_origin(p->win, &x, &y);
                input_window_moved(p->wnd, x, y);
                input_window_resized(p->wnd, (int)c.size.width, (int)c.size.height);
            }
        } else {
            if (p->win.parentWindow)
                [p->win.parentWindow removeChildWindow:p->win];
            [p->win orderOut:nil];
            p->showing = 0;
        }
    }
}

void plat_window_minimize(PlatWindow *p)
{
    [p->win miniaturize:nil];
}

void plat_window_maximize(PlatWindow *p)
{
    if (!p->win.zoomed)
        [p->win zoom:nil];
}

void plat_window_restore(PlatWindow *p)
{
    if (p->win.miniaturized)
        [p->win deminiaturize:nil];
    else if (p->win.zoomed)
        [p->win zoom:nil];
}

void plat_window_raise(PlatWindow *p)
{
    [p->win makeKeyAndOrderFront:nil];
}

void plat_window_set_title(PlatWindow *p, const char *title)
{
    @autoreleasepool {
        p->win.title = [NSString stringWithUTF8String:title] ?: @"";
    }
}

void plat_window_set_rect(PlatWindow *p, int x, int y, int cw, int ch)
{
    NSRect f = [p->win frameRectForContentRect:cocoa_rect(x, y, cw, ch)];

    if (!NSEqualRects(f, p->win.frame))
        [p->win setFrame:f display:NO];
}

void plat_window_set_minmax(PlatWindow *p, int minw, int minh, int maxw, int maxh)
{
    if (minw || minh)
        p->win.contentMinSize = NSMakeSize(minw, minh);
    if (maxw || maxh)
        p->win.contentMaxSize = NSMakeSize(maxw ? maxw : 100000, maxh ? maxh : 100000);
}

static void release_bits(void *info, const void *data, size_t size)
{
    (void)info;
    (void)size;
    free((void *)data);
}

void plat_window_present(PlatWindow *p, const Surface *fb, const Surface *overlay, int ox, int oy)
{
    @autoreleasepool {
        size_t size = (size_t)fb->pitch * (size_t)fb->h;
        uint8_t *copy = malloc(size);
        CGColorSpaceRef cs;
        CGDataProviderRef dp;
        CGImageRef img;
        int y;

        memcpy(copy, fb->bits, size);
        if (overlay) {
            for (y = 0; y < overlay->h; y++) {
                int dy = oy + y, x0 = ox < 0 ? -ox : 0, x1 = overlay->w;

                if (dy < 0 || dy >= fb->h)
                    continue;
                if (ox + x1 > fb->w)
                    x1 = fb->w - ox;
                if (x1 > x0)
                    memcpy(copy + (size_t)dy * fb->pitch + (size_t)(ox + x0) * 4,
                           overlay->bits + (size_t)y * overlay->pitch + (size_t)x0 * 4, (size_t)(x1 - x0) * 4);
            }
        }
        cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        dp = CGDataProviderCreateWithData(NULL, copy, size, release_bits);
        img = CGImageCreate((size_t)fb->w, (size_t)fb->h, 8, 32, (size_t)fb->pitch, cs,
                            kCGBitmapByteOrder32Little | (CGBitmapInfo)kCGImageAlphaNoneSkipFirst, dp, NULL, false,
                            kCGRenderingIntentDefault);
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        p->view.layer.contents = (__bridge id)img;
        [CATransaction commit];
        CGImageRelease(img);
        CGDataProviderRelease(dp);
        CGColorSpaceRelease(cs);
        [CATransaction flush];
    }
}

int plat_window_snapshot(PlatWindow *p, const char *path)
{
    @autoreleasepool {
        NSSize sz = p->view.bounds.size;
        NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:NULL pixelsWide:(NSInteger)sz.width pixelsHigh:(NSInteger)sz.height
                       bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO
                      colorSpaceName:NSDeviceRGBColorSpace bytesPerRow:0 bitsPerPixel:32];
        NSGraphicsContext *g = [NSGraphicsContext graphicsContextWithBitmapImageRep:rep];
        CGContextRef ctx = g.CGContext;
        NSData *png;

        if (!rep || !ctx)
            return 0;
        /* the layer tree draws top-down: flip Core Graphics' bottom-up context */
        CGContextTranslateCTM(ctx, 0, sz.height);
        CGContextScaleCTM(ctx, 1, -1);
        [p->view.layer renderInContext:ctx];
        CGContextFlush(ctx);
        png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
        return [png writeToFile:[NSString stringWithUTF8String:path] atomically:NO];
    }
}

int plat_window_is_key(PlatWindow *p)
{
    return p->win.keyWindow;
}

int plat_app_active(void)
{
    return NSApp.active;
}

/* ---- Events and input state ---- */

static NSMutableArray *g_graveyard;

void cocoa_bury(id object)
{
    if (!g_graveyard)
        g_graveyard = [NSMutableArray new];
    [g_graveyard addObject:object];
}

int plat_pump(int timeout_ms)
{
    @autoreleasepool {
        if (g_graveyard.count) {
            NSArray *dead = [g_graveyard copy];

            [g_graveyard removeAllObjects];
            for (id o in dead)
                if ([o isKindOfClass:[NSWindow class]])
                    [(NSWindow *)o close];
        }
        NSDate *until = timeout_ms > 0 ? [NSDate dateWithTimeIntervalSinceNow:timeout_ms / 1000.0]
                                       : [NSDate distantPast];
        NSEvent *e;
        int n = 0;

        while ((e = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:until inMode:NSDefaultRunLoopMode
                                         dequeue:YES]) != nil) {
            g_sendingInjected = g_injected.count && take_injected(e.timestamp);
            [NSApp sendEvent:e];
            g_sendingInjected = 0;
            n++;
            until = [NSDate distantPast];
        }
        [NSApp updateWindows];
        if (!NSApp.active)
            input_app_deactivated();
        return n;
    }
}

int plat_key_down(int vk)
{
    NSEventModifierFlags f = [NSEvent modifierFlags];
    int k;

    switch (vk) {
    case VK_SHIFT:
        return (f & NSEventModifierFlagShift) != 0;
    case VK_CONTROL:
        return (f & NSEventModifierFlagControl) != 0;
    case VK_MENU:
        return (f & NSEventModifierFlagOption) != 0;
    }
    k = keycode_of_vk(vk);
    return k >= 0 && CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, (CGKeyCode)k);
}

int plat_mods(void)
{
    return mods_of([NSEvent modifierFlags]);
}

unsigned plat_mouse_buttons(void)
{
    NSUInteger b = [NSEvent pressedMouseButtons];

    return (unsigned)(b & 7);
}

void plat_mouse_pos(int *x, int *y)
{
    NSPoint p = [NSEvent mouseLocation];

    *x = (int)floor(p.x);
    *y = (int)floor(screen0_height() - p.y);
}

void plat_set_mouse_pos(int x, int y)
{
    CGWarpMouseCursorPosition(CGPointMake(x, y));
}

void plat_capture_mouse(int on)
{
    (void)on;                   /* AppKit sends a drag to the view it started in */
}

int plat_key_char(int scancode, int mods)
{
    (void)mods;
    return scancode >= 0 && scancode < 128 ? g_keyChar[scancode] : 0;
}

static int g_injectedDown, g_swallowUp;

void plat_inject_mouse(PlatWindow *p, int x, int y, int button, int down, int clicks, int moveOnly)
{
    @autoreleasepool {
        NSEventType t;
        NSEvent *e;
        NSPoint loc = NSMakePoint(x + 0.5, p->view.bounds.size.height - y - 0.5);

        NSView *hit;

        if (moveOnly)
            t = g_injectedDown ? NSEventTypeLeftMouseDragged : NSEventTypeMouseMoved;
        else if (button == 1)
            t = down ? NSEventTypeRightMouseDown : NSEventTypeRightMouseUp;
        else
            t = down ? NSEventTypeLeftMouseDown : NSEventTypeLeftMouseUp;
        if (!moveOnly && !down && g_swallowUp) {
            g_swallowUp = 0;        /* sent already, with the down */
            g_injectedDown = 0;
            return;
        }
        if (!moveOnly)
            g_injectedDown = down;
        e = [NSEvent mouseEventWithType:t location:loc modifierFlags:0 timestamp:inject_stamp()
                           windowNumber:p->win.windowNumber context:nil eventNumber:0 clickCount:moveOnly ? 0 : clicks
                               pressure:down ? 1 : 0];
        [NSApp postEvent:e atStart:NO];
        /* a native control tracks the mouse in a loop of its own until the
           button comes up, and the harness can't run meanwhile: the up goes
           with the down */
        hit = down && !moveOnly ? [p->view hitTest:loc] : nil;
        if (hit && hit != p->view) {
            e = [NSEvent mouseEventWithType:button == 1 ? NSEventTypeRightMouseUp : NSEventTypeLeftMouseUp
                                   location:loc modifierFlags:0 timestamp:inject_stamp()
                               windowNumber:p->win.windowNumber context:nil eventNumber:0 clickCount:clicks
                                   pressure:0];
            [NSApp postEvent:e atStart:NO];
            g_swallowUp = 1;
        }
    }
}

static int keycode_of_name(const char *name, NSString **chars)
{
    static const struct {
        const char *name;
        int code;
        const char *chars;
    } names[] = {
        {"Return", kVK_Return, "\r"}, {"Escape", kVK_Escape, "\x1b"}, {"Tab", kVK_Tab, "\t"},
        {"Space", kVK_Space, " "}, {"Backspace", kVK_Delete, "\x7f"}, {"Left", kVK_LeftArrow, ""},
        {"Right", kVK_RightArrow, ""}, {"Up", kVK_UpArrow, ""}, {"Down", kVK_DownArrow, ""},
        {"F1", kVK_F1, ""}, {"F2", kVK_F2, ""}, {"F3", kVK_F3, ""}, {"F10", kVK_F10, ""},
    };
    size_t i;
    int vk;

    for (i = 0; i < sizeof names / sizeof names[0]; i++)
        if (!strcasecmp(name, names[i].name)) {
            *chars = [NSString stringWithUTF8String:names[i].chars];
            return names[i].code;
        }
    if (strlen(name) == 1) {
        vk = toupper((unsigned char)name[0]);
        *chars = [[NSString stringWithUTF8String:name] lowercaseString];
        return keycode_of_vk(vk);
    }
    return -1;
}

void plat_inject_key(PlatWindow *p, const char *name, int down)
{
    @autoreleasepool {
        NSString *chars = @"";
        int code = keycode_of_name(name, &chars);
        NSEvent *e;

        if (code < 0)
            return;
        e = [NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp location:NSZeroPoint
                        modifierFlags:0 timestamp:inject_stamp()
                         windowNumber:p->win.windowNumber context:nil characters:chars
          charactersIgnoringModifiers:chars isARepeat:NO keyCode:(unsigned short)code];
        [NSApp postEvent:e atStart:NO];
    }
}

/* ---- Cursors ---- */

void *plat_cursor_system(int idc)
{
    NSCursor *c;

    switch (idc) {
    case 32513:
        c = [NSCursor IBeamCursor];
        break;
    case 32515:
        c = [NSCursor crosshairCursor];
        break;
    case 32640:
    case 32646:
        c = [NSCursor openHandCursor];
        break;
    case 32644:
        c = [NSCursor resizeLeftRightCursor];
        break;
    case 32645:
        c = [NSCursor resizeUpDownCursor];
        break;
    case 32648:
        c = [NSCursor operationNotAllowedCursor];
        break;
    default:
        c = [NSCursor arrowCursor];
        break;
    }
    return (__bridge_retained void *)c;
}

void *plat_cursor_image(const uint32_t *argb, int w, int h, int hx, int hy)
{
    @autoreleasepool {
        CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        CGContextRef ctx = CGBitmapContextCreate(NULL, (size_t)w, (size_t)h, 8, (size_t)w * 4, cs,
                                                 kCGBitmapByteOrder32Little | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
        uint32_t *px = CGBitmapContextGetData(ctx);
        CGImageRef img;
        NSImage *ni;
        NSCursor *c;
        int i;

        for (i = 0; i < w * h; i++)
            px[i] = (argb[i] >> 24) ? argb[i] : 0;     /* fully opaque or clear: already premultiplied */
        img = CGBitmapContextCreateImage(ctx);
        ni = [[NSImage alloc] initWithCGImage:img size:NSMakeSize(w, h)];
        c = [[NSCursor alloc] initWithImage:ni hotSpot:NSMakePoint(hx, hy)];
        CGImageRelease(img);
        CGContextRelease(ctx);
        CGColorSpaceRelease(cs);
        return (__bridge_retained void *)c;
    }
}

static int g_cursorHidden;

void plat_cursor_set(void *c)
{
    if (!c) {
        if (!g_cursorHidden) {
            [NSCursor hide];
            g_cursorHidden = 1;
        }
        return;
    }
    if (g_cursorHidden) {
        [NSCursor unhide];
        g_cursorHidden = 0;
    }
    [(__bridge NSCursor *)c set];
}

void plat_cursor_show(int show)
{
    if (show && g_cursorHidden) {
        [NSCursor unhide];
        g_cursorHidden = 0;
    } else if (!show && !g_cursorHidden) {
        [NSCursor hide];
        g_cursorHidden = 1;
    }
}
