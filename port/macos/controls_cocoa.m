/*
 * macOS: the native controls of plat.h - AppKit views over a dialog's push
 * buttons, check boxes, radio buttons, group boxes, edit fields, list boxes
 * and static text, and over the game's skill panel buttons and status bar,
 * as subviews of the window's PortView (whose layer shows the framebuffer).
 * The layer (port/src/peer.c) owns their state and pushes it here; a view
 * only reports what the user did (input_control*), and the layer answers by
 * pushing its new state back. Views made here while the layer pushes are
 * quiet: they report nothing. Labels, group boxes, the skill panel's buttons
 * and the status bar are shown only: the mouse goes through them to the
 * layer's windows.
 */
#include "cocoa_win32.h"

struct PlatControl {
    NSView *view;           /* the view in the window (a list's scroll view) */
    NSView *keyView;        /* what takes the keyboard */
    NSButton *button;
    NSTableView *table;
    NSMutableArray *rows;   /* an owner-drawn list's rows, as images */
    NSTextField *label;     /* static text */
    NSImageView *image;     /* a dialog's icon */
    NSMutableArray *parts;  /* a status bar's labels */
    uint32_t *pixels;       /* an icon button's picture as shown, to tell a new one */
    int pw, ph, scale;      /* its size, and how many screen pixels each of its pixels is */
    uint32_t hwnd;
    int kind, nrows, rowH;
    int quiet;              /* our own changes: report nothing */
    int vertical, tracking, knobMoved;  /* a scroller */
};

@interface PortControls : NSObject <NSTextFieldDelegate, NSTableViewDataSource, NSTableViewDelegate>
@end

static PortControls *g_ctl;

static NSFont *small_font(void)
{
    return [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeSmall]];
}

/* ---- Views ----
 * Each takes the click that also activates its window, as the game's own
 * view does (and as Windows' controls do). */

@interface PortButton : NSButton
@end

@implementation PortButton
- (BOOL)acceptsFirstMouse:(NSEvent *)e
{
    return YES;
}
@end

@interface PortBox : NSBox
@end

@implementation PortBox
- (NSView *)hitTest:(NSPoint)p
{
    return nil;                             /* a group box lets clicks through (HTTRANSPARENT) */
}
@end

/* shown only: static text, and the parts of the game's own bars */
@interface PortLabel : NSTextField
@end

@implementation PortLabel
- (NSView *)hitTest:(NSPoint)p
{
    return nil;
}
@end

@interface PortShown : NSView
@end

@implementation PortShown
- (BOOL)isFlipped
{
    return YES;
}
- (NSView *)hitTest:(NSPoint)p
{
    return nil;                             /* the game's window under it takes the mouse */
}
@end

@interface PortField : NSTextField
@property(nonatomic) uint32_t hwnd;
@end

@implementation PortField
- (BOOL)acceptsFirstMouse:(NSEvent *)e
{
    return YES;
}
- (BOOL)becomeFirstResponder
{
    BOOL ok = [super becomeFirstResponder];

    if (ok)
        input_control(self.hwnd, PCE_FOCUS, 0);
    return ok;
}
@end

@interface PortTable : NSTableView
@property(nonatomic) PlatControl *pc;
@end

@implementation PortTable
- (BOOL)acceptsFirstMouse:(NSEvent *)e
{
    return YES;
}
- (BOOL)becomeFirstResponder
{
    BOOL ok = [super becomeFirstResponder];

    if (ok && self.pc && !self.pc->quiet)
        input_control(self.pc->hwnd, PCE_FOCUS, 0);
    return ok;
}
- (void)keyDown:(NSEvent *)e
{
    unichar c = e.charactersIgnoringModifiers.length ? [e.charactersIgnoringModifiers characterAtIndex:0] : 0;
    int vk = c == '\r' || c == 3 ? VK_RETURN : c == 27 ? VK_ESCAPE : c == '\t' || c == 25 ? VK_TAB : 0;

    if (vk && self.pc)
        input_control(self.pc->hwnd, PCE_KEY, vk);  /* the dialog's keys: the layer's dialog manager */
    else
        [super keyDown:e];
}
@end

/* a window's scroll bar: the legacy style (always shown, the space is the
   window's), the knob as large as the page */
@interface PortScroller : NSScroller
@property(nonatomic) PlatControl *pc;
@end

@implementation PortScroller
+ (BOOL)isCompatibleWithOverlayScrollers
{
    return NO;
}
- (BOOL)acceptsFirstMouse:(NSEvent *)e
{
    return YES;
}
- (void)mouseDown:(NSEvent *)e
{
    PlatControl *c = self.pc;

    if (!c) {
        [super mouseDown:e];
        return;
    }
    /* the scroller tracks the mouse until the button comes up, sending its
       action meanwhile; then the scroll ends, as Win32's do */
    c->tracking = 1;
    c->knobMoved = 0;
    [super mouseDown:e];
    if (self.pc) {
        c->tracking = 0;
        if (c->knobMoved)
            input_scroll(c->hwnd, c->vertical, SB_THUMBPOSITION, self.doubleValue);
        if (self.pc)
            input_scroll(c->hwnd, c->vertical, SB_ENDSCROLL, 0);
    }
}
@end

@implementation PortControls
- (void)scrollerUsed:(PortScroller *)s
{
    PlatControl *c = s.pc;

    if (!c)
        return;
    switch (s.hitPart) {
    case NSScrollerDecrementPage:
        input_scroll(c->hwnd, c->vertical, SB_PAGEUP, 0);
        break;
    case NSScrollerIncrementPage:
        input_scroll(c->hwnd, c->vertical, SB_PAGEDOWN, 0);
        break;
    case NSScrollerKnob:
    case NSScrollerKnobSlot:
        c->knobMoved = 1;
        input_scroll(c->hwnd, c->vertical, SB_THUMBTRACK, s.doubleValue);
        break;
    default:
        break;
    }
}
- (void)clicked:(NSControl *)sender
{
    input_control((uint32_t)sender.tag, PCE_CLICK, 0);
}
- (void)controlTextDidChange:(NSNotification *)n
{
    PortField *f = n.object;

    if ([f isKindOfClass:[PortField class]])
        input_control_text(f.hwnd, f.stringValue.UTF8String ?: "");
}
- (BOOL)control:(NSControl *)control textView:(NSTextView *)tv doCommandBySelector:(SEL)cmd
{
    int vk = cmd == @selector(insertNewline:) ? VK_RETURN : cmd == @selector(cancelOperation:) ? VK_ESCAPE
           : cmd == @selector(insertTab:) || cmd == @selector(insertBacktab:) ? VK_TAB : 0;

    if (!vk || ![control isKindOfClass:[PortField class]])
        return NO;
    input_control(((PortField *)control).hwnd, PCE_KEY, vk);
    return YES;
}
- (NSInteger)numberOfRowsInTableView:(NSTableView *)t
{
    PlatControl *c = ((PortTable *)t).pc;

    return c ? c->nrows : 0;
}
- (NSView *)tableView:(NSTableView *)t viewForTableColumn:(NSTableColumn *)col row:(NSInteger)row
{
    PlatControl *c = ((PortTable *)t).pc;

    if (!c)
        return nil;
    if (c->kind == PC_LIST_OWNERDRAW) {
        NSImageView *iv = [t makeViewWithIdentifier:@"image" owner:self];

        if (!iv) {
            iv = [NSImageView new];
            iv.identifier = @"image";
            iv.imageScaling = NSImageScaleNone;
            iv.imageAlignment = NSImageAlignTopLeft;
        }
        iv.image = row < (NSInteger)c->rows.count && c->rows[row] != [NSNull null] ? c->rows[row] : nil;
        return iv;
    } else {
        NSTableCellView *cv = [t makeViewWithIdentifier:@"text" owner:self];
        char text[512];

        if (!cv) {
            NSTextField *tf = [NSTextField labelWithString:@""];

            cv = [NSTableCellView new];
            cv.identifier = @"text";
            tf.font = small_font();
            tf.lineBreakMode = NSLineBreakByTruncatingTail;
            tf.translatesAutoresizingMaskIntoConstraints = NO;
            [cv addSubview:tf];
            cv.textField = tf;
            [tf.leadingAnchor constraintEqualToAnchor:cv.leadingAnchor constant:2].active = YES;
            [tf.trailingAnchor constraintEqualToAnchor:cv.trailingAnchor constant:-2].active = YES;
            [tf.centerYAnchor constraintEqualToAnchor:cv.centerYAnchor].active = YES;
        }
        cv.textField.stringValue = list_native_text(c->hwnd, (int)row, text, sizeof text)
                                       ? ([NSString stringWithUTF8String:text] ?: @"") : @"";
        return cv;
    }
}
- (void)tableViewSelectionDidChange:(NSNotification *)n
{
    PortTable *t = n.object;

    if (t.pc && !t.pc->quiet && t.selectedRow >= 0)
        input_control(t.pc->hwnd, PCE_SELECT, (int)t.selectedRow);
}
- (void)doubleClicked:(PortTable *)t
{
    if (t.pc && t.clickedRow >= 0)
        input_control(t.pc->hwnd, PCE_DBLCLK, (int)t.clickedRow);
}
- (void)scrolled:(NSNotification *)n
{
    NSClipView *clip = n.object;
    PortTable *t = (PortTable *)clip.documentView;
    PlatControl *c = [t isKindOfClass:[PortTable class]] ? t.pc : NULL;

    if (c && !c->quiet && c->rowH > 0)
        input_control(c->hwnd, PCE_TOP, (int)floor(clip.bounds.origin.y / c->rowH + 0.5));
}
@end

/* ---- plat.h ---- */

static NSButton *button(int kind)
{
    NSButton *b;

    if (kind == PC_CHECKBOX)
        b = [PortButton checkboxWithTitle:@"" target:g_ctl action:@selector(clicked:)];
    else if (kind == PC_RADIO)
        b = [PortButton radioButtonWithTitle:@"" target:g_ctl action:@selector(clicked:)];
    else {
        b = [PortButton buttonWithTitle:@"" target:g_ctl action:@selector(clicked:)];
        b.bezelStyle = NSBezelStyleRounded;
        if (kind == PC_DEFBUTTON)
            b.keyEquivalent = @"\r";
    }
    b.controlSize = NSControlSizeSmall;
    b.font = small_font();
    return b;
}

PlatControl *plat_control_create(PlatWindow *top, uint32_t hwnd, int kind)
{
    @autoreleasepool {
        PlatControl *c;

        /* LEMMINGS_NATIVE_CONTROLS: which kinds get views - b(uttons), g(roup
           boxes), e(dit fields), l(ists), t(he owner-drawn list), x (static
           text), i(con buttons of the skill panel), u (the status bar);
           default all (and c: the system's colours, s: its scrollers) */
        char k = kind <= PC_RADIO ? 'b' : kind == PC_GROUPBOX ? 'g' : kind == PC_EDIT ? 'e' : kind == PC_LIST ? 'l'
                 : kind == PC_LIST_OWNERDRAW ? 't' : kind == PC_ICONBUTTON ? 'i' : kind == PC_STATUSBAR ? 'u' : 'x';

        if (!cocoa_native(k))
            return NULL;
        if (!g_ctl)
            g_ctl = [PortControls new];
        c = calloc(1, sizeof *c);
        c->hwnd = hwnd;
        c->kind = kind;
        switch (kind) {
        case PC_BUTTON:
        case PC_DEFBUTTON:
        case PC_CHECKBOX:
            c->view = c->keyView = c->button = button(kind);
            c->button.tag = (NSInteger)hwnd;
            break;
        case PC_RADIO:
            /* AppKit makes the radio buttons of one superview a group; the
               layer keeps the dialog's WS_GROUP runs, so each gets its own */
            c->view = [NSView new];
            c->keyView = c->button = button(kind);
            c->button.tag = (NSInteger)hwnd;
            c->button.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
            [c->view addSubview:c->button];
            break;
        case PC_GROUPBOX:
            {
                PortBox *b = [PortBox new];

                b.boxType = NSBoxPrimary;
                b.titlePosition = NSAtTop;
                b.titleFont = small_font();
                c->view = b;
            }
            break;
        case PC_EDIT:
            {
                PortField *f = [PortField new];

                f.hwnd = hwnd;
                f.editable = YES;
                f.selectable = YES;
                f.bezeled = YES;
                f.bezelStyle = NSTextFieldSquareBezel;
                f.font = small_font();
                f.controlSize = NSControlSizeSmall;
                f.usesSingleLineMode = YES;
                f.cell.scrollable = YES;
                f.cell.wraps = NO;
                f.delegate = g_ctl;
                c->view = c->keyView = f;
            }
            break;
        case PC_LIST:
        case PC_LIST_OWNERDRAW:
            {
                NSScrollView *sv = [NSScrollView new];
                PortTable *t = [PortTable new];
                NSTableColumn *col = [[NSTableColumn alloc] initWithIdentifier:@"rows"];

                t.pc = c;
                [t addTableColumn:col];
                t.headerView = nil;
                t.style = NSTableViewStylePlain;
                t.intercellSpacing = NSZeroSize;
                t.columnAutoresizingStyle = NSTableViewFirstColumnOnlyAutoresizingStyle;
                t.allowsEmptySelection = YES;
                t.allowsMultipleSelection = NO;
                t.dataSource = g_ctl;
                t.delegate = g_ctl;
                t.target = g_ctl;
                t.doubleAction = @selector(doubleClicked:);
                t.backgroundColor = NSColor.textBackgroundColor;
                if (kind == PC_LIST_OWNERDRAW) {
                    t.selectionHighlightStyle = NSTableViewSelectionHighlightStyleNone;  /* the game draws it */
                    t.focusRingType = NSFocusRingTypeNone;
                    c->rows = [NSMutableArray new];
                }
                sv.documentView = t;
                sv.hasVerticalScroller = YES;
                sv.autohidesScrollers = YES;
                sv.borderType = NSBezelBorder;
                sv.contentView.postsBoundsChangedNotifications = YES;
                [[NSNotificationCenter defaultCenter] addObserver:g_ctl selector:@selector(scrolled:)
                                                             name:NSViewBoundsDidChangeNotification
                                                           object:sv.contentView];
                c->view = sv;
                c->keyView = c->table = t;
            }
            break;
        case PC_LABEL:
        case PC_LABEL_CENTER:
        case PC_LABEL_RIGHT:
        case PC_LABEL_LINE:
            {
                PortLabel *l = kind == PC_LABEL ? [PortLabel wrappingLabelWithString:@""]
                                                : [PortLabel labelWithString:@""];

                l.selectable = NO;
                l.font = small_font();
                l.alignment = kind == PC_LABEL_CENTER ? NSTextAlignmentCenter
                              : kind == PC_LABEL_RIGHT ? NSTextAlignmentRight : NSTextAlignmentLeft;
                if (kind != PC_LABEL)
                    l.lineBreakMode = NSLineBreakByClipping;
                else
                    l.cell.truncatesLastVisibleLine = YES;      /* more text than its box holds ends in "..." */
                c->view = c->label = l;
            }
            break;
        case PC_ICON:
            {
                PortShown *v = [PortShown new];
                NSImageView *iv = [NSImageView new];

                if (!cocoa_app_icon()) {
                    free(c);
                    return NULL;        /* no icon file: the layer draws the game's own */
                }
                iv.image = cocoa_app_icon();
                iv.imageScaling = NSImageScaleProportionallyUpOrDown;
                iv.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
                [v addSubview:iv];
                c->view = v;
                c->image = iv;
            }
            break;
        case PC_ICONBUTTON:
            {
                /* a push button that stays on or off, as tall as it likes, with
                   the game's number as its title over the game's picture; in a
                   view of its own that lets the mouse through */
                PortShown *v = [PortShown new];
                NSButton *b = [NSButton buttonWithTitle:@" " target:nil action:NULL];

                b.bezelStyle = NSBezelStyleFlexiblePush;
                [b setButtonType:NSButtonTypePushOnPushOff];
                b.refusesFirstResponder = YES;
                b.font = [NSFont monospacedDigitSystemFontOfSize:10 weight:NSFontWeightSemibold];
                b.imagePosition = NSImageBelow;
                b.imageScaling = NSImageScaleNone;
                b.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
                [v addSubview:b];
                c->view = v;
                c->button = b;
            }
            break;
        case PC_STATUSBAR:
            {
                PortShown *v = [PortShown new];
                NSBox *line = [NSBox new];

                line.boxType = NSBoxSeparator;
                [v addSubview:line];
                c->view = v;
                c->parts = [NSMutableArray new];
            }
            break;
        default:
            free(c);
            return NULL;
        }
        c->view.hidden = YES;
        [top->view addSubview:c->view];
        return c;
    }
}

PlatControl *plat_scroller_create(PlatWindow *top, uint32_t hwnd, int vertical)
{
    @autoreleasepool {
        PlatControl *c;
        PortScroller *s;

        if (!cocoa_native('s'))
            return NULL;
        if (!g_ctl)
            g_ctl = [PortControls new];
        c = calloc(1, sizeof *c);
        c->hwnd = hwnd;
        c->kind = -1;
        c->vertical = vertical;
        /* a scroller is vertical or horizontal by the shape it starts with */
        s = [[PortScroller alloc] initWithFrame:vertical ? NSMakeRect(0, 0, 16, 100) : NSMakeRect(0, 0, 100, 16)];
        s.pc = c;
        s.scrollerStyle = NSScrollerStyleLegacy;
        s.controlSize = NSControlSizeRegular;
        s.target = g_ctl;
        s.action = @selector(scrollerUsed:);
        s.enabled = YES;
        c->view = c->keyView = s;
        c->view.hidden = YES;
        [top->view addSubview:s];
        return c;
    }
}

void plat_scroller_set(PlatControl *c, double value, double proportion)
{
    PortScroller *s = (PortScroller *)c->view;

    if (c->tracking)
        return;             /* the knob is in the user's hand */
    s.knobProportion = proportion;
    s.doubleValue = value;
}

void plat_scroller_knob(PlatControl *c, int *x, int *y)
{
    NSScroller *s = (NSScroller *)c->view;
    NSRect k = [s rectForPart:NSScrollerKnob];

    k = [s convertRect:k toView:s.superview];
    *x = (int)(k.origin.x + k.size.width / 2);
    *y = (int)(k.origin.y + k.size.height / 2);
}

void plat_control_destroy(PlatControl *c)
{
    if (!c)
        return;
    @autoreleasepool {
        if ([c->view isKindOfClass:[PortScroller class]]) {
            ((PortScroller *)c->view).pc = NULL;
            ((PortScroller *)c->view).target = nil;
        }
        if (c->table) {
            [[NSNotificationCenter defaultCenter] removeObserver:g_ctl name:NSViewBoundsDidChangeNotification
                                                          object:((NSScrollView *)c->view).contentView];
            ((PortTable *)c->table).pc = NULL;
            c->table.dataSource = nil;
            c->table.delegate = nil;
        }
        c->button.target = nil;
        [c->view removeFromSuperview];
        cocoa_bury(c->view);    /* it may be the view whose action is running */
        c->view = c->keyView = nil;
        c->button = nil;
        c->table = nil;
        c->rows = nil;
        c->label = nil;
        c->image = nil;
        c->parts = nil;
    }
    free(c->pixels);
    free(c);
}

/* ---- Shown only: labels, the skill panel's buttons, the status bar ---- */

/* static text that doesn't fit its Windows-sized box at the small size gets the mini size */
static void fit_label(PlatControl *c)
{
    NSTextField *l = c->label;
    NSFont *f = small_font();
    NSRect need;

    if (c->kind < PC_LABEL || c->kind > PC_LABEL_LINE || NSIsEmptyRect(l.frame))
        return;
    need = [l.stringValue boundingRectWithSize:NSMakeSize(c->kind == PC_LABEL ? l.frame.size.width - 4 : 10000, 10000)
                                       options:NSStringDrawingUsesLineFragmentOrigin
                                    attributes:@{NSFontAttributeName : f}];
    if (need.size.width > l.frame.size.width - 2 || need.size.height > l.frame.size.height + 4)
        f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeMini]];
    l.font = f;
}

static void layout_status(PlatControl *c)
{
    NSRect b = c->view.bounds;
    CGFloat x = b.size.width - 12, h = 14, y = floor((b.size.height - h) / 2);
    NSUInteger i;

    c->view.subviews.firstObject.frame = NSMakeRect(0, 0, b.size.width, 1);     /* the line above it */
    for (i = c->parts.count; i-- > 1;) {    /* the counters, from the right */
        NSTextField *l = c->parts[i];
        CGFloat w = ceil(l.attributedStringValue.size.width) + 6;

        x -= w;
        l.frame = NSMakeRect(x, y, w, h);
        x -= 18;
    }
    if (c->parts.count)
        ((NSTextField *)c->parts[0]).frame = NSMakeRect(10, y, MAX(0, x - 10), h);
}

void plat_iconbutton_set(PlatControl *c, const uint32_t *argb, int w, int h, const char *count, int on)
{
    @autoreleasepool {
        /* a button without a number has a blank title: the pictures stay in one row */
        NSString *num = count[0] ? ([NSString stringWithUTF8String:count] ?: @" ") : @" ";
        NSControlStateValue st = on ? NSControlStateValueOn : NSControlStateValueOff;
        /* the game's pixels, each a whole number of the screen's: sharp, as the level is */
        int k = c->view.window.backingScaleFactor > 1 ? 2 : 1;

        if (c->kind != PC_ICONBUTTON)
            return;
        if (argb && w > 0 && h > 0 && (!c->pixels || c->pw != w || c->ph != h || c->scale != k ||
                                       memcmp(c->pixels, argb, (size_t)w * (size_t)h * 4))) {
            int x, y;
            NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
                initWithBitmapDataPlanes:NULL pixelsWide:w * k pixelsHigh:h * k bitsPerSample:8 samplesPerPixel:4
                                hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
                            bitmapFormat:NSBitmapFormatAlphaNonpremultiplied bytesPerRow:w * k * 4 bitsPerPixel:32];
            unsigned char *d = rep.bitmapData;
            NSImage *img;

            for (y = 0; y < h * k; y++)
                for (x = 0; x < w * k; x++) {
                    uint32_t p = argb[(y / k) * w + x / k];
                    unsigned char *o = d + ((size_t)y * (size_t)(w * k) + (size_t)x) * 4;

                    o[0] = (unsigned char)(p >> 16);
                    o[1] = (unsigned char)(p >> 8);
                    o[2] = (unsigned char)p;
                    o[3] = (unsigned char)(p >> 24);
                }
            rep = [rep bitmapImageRepByRetaggingWithColorSpace:NSColorSpace.sRGBColorSpace] ?: rep;
            rep.size = NSMakeSize(w, h);
            img = [[NSImage alloc] initWithSize:NSMakeSize(w, h)];
            [img addRepresentation:rep];
            c->button.image = img;
            free(c->pixels);
            c->pixels = malloc((size_t)w * (size_t)h * 4);
            memcpy(c->pixels, argb, (size_t)w * (size_t)h * 4);
            c->pw = w;
            c->ph = h;
            c->scale = k;
        }
        if (![c->button.title isEqualToString:num])
            c->button.title = num;
        if (c->button.state != st)
            c->button.state = st;
    }
}

void plat_statusbar_set(PlatControl *c, int n, const char *const *utf8)
{
    @autoreleasepool {
        int i, changed = 0;

        if (c->kind != PC_STATUSBAR)
            return;
        while ((int)c->parts.count < n) {
            NSTextField *l = [NSTextField labelWithString:@""];
            CGFloat size = [NSFont systemFontSizeForControlSize:NSControlSizeSmall];

            /* the message as text, the counters with digits of one width */
            l.font = c->parts.count ? [NSFont monospacedDigitSystemFontOfSize:size weight:NSFontWeightRegular]
                                    : [NSFont systemFontOfSize:size];
            l.textColor = c->parts.count ? NSColor.secondaryLabelColor : NSColor.labelColor;
            l.lineBreakMode = NSLineBreakByTruncatingTail;
            [c->view addSubview:l];
            [c->parts addObject:l];
            changed = 1;
        }
        while ((int)c->parts.count > n) {
            [c->parts.lastObject removeFromSuperview];
            [c->parts removeLastObject];
            changed = 1;
        }
        for (i = 0; i < n; i++) {
            NSString *t = [NSString stringWithUTF8String:utf8[i]] ?: @"";
            NSTextField *l = c->parts[(NSUInteger)i];

            if (![l.stringValue isEqualToString:t]) {
                l.stringValue = t;
                changed = 1;
            }
        }
        if (changed)
            layout_status(c);
    }
}

void plat_control_describe(PlatControl *c, char *out, int size)
{
    @autoreleasepool {
        NSMutableString *t = [NSMutableString new];

        if (c->kind == PC_ICONBUTTON) {
            int i, n = 0;

            for (i = 0; c->pixels && i < c->pw * c->ph; i++)
                n += c->pixels[i] >> 24 != 0;
            [t appendFormat:@"number '%@' %s, picture %d x %d with %d pixels", c->button.title,
                            c->button.state == NSControlStateValueOn ? "on" : "off", c->pw, c->ph, n];
        } else if (c->kind == PC_STATUSBAR) {
            for (NSTextField *l in c->parts)
                [t appendFormat:@"%s'%@'", t.length ? " " : "", l.stringValue];
        } else if (c->label)
            [t appendFormat:@"'%@'", c->label.stringValue];
        snprintf(out, (size_t)size, "%s", t.UTF8String ?: "");
    }
}

/* a push button whose title doesn't fit at the small size gets the mini size
   (Windows' dialog layouts are tighter than the Mac's bezels) */
static void fit_button(PlatControl *c)
{
    NSButton *b = c->button;
    NSFont *f;

    if (!b || (c->kind != PC_BUTTON && c->kind != PC_DEFBUTTON) || NSIsEmptyRect(b.frame))
        return;
    f = small_font();
    b.controlSize = NSControlSizeSmall;
    if ([b.title sizeWithAttributes:@{NSFontAttributeName : f}].width + 20 > b.frame.size.width) {
        b.controlSize = NSControlSizeMini;
        f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeMini]];
    }
    b.font = f;
}

void plat_control_frame(PlatControl *c, int x, int y, int w, int h, int visible, int enabled)
{
    c->view.frame = NSMakeRect(x, y, w, h);
    if (c->button && c->button != c->view)
        c->button.frame = c->view.bounds;
    c->view.hidden = !visible;
    fit_button(c);
    if (c->kind == PC_STATUSBAR)
        layout_status(c);
    else if (c->kind == PC_ICON)
        c->image.frame = c->view.bounds;
    else if (c->label) {
        fit_label(c);
        c->label.textColor = enabled ? NSColor.labelColor : NSColor.disabledControlTextColor;
    }
    if ([c->keyView isKindOfClass:[NSControl class]])
        ((NSControl *)c->keyView).enabled = enabled != 0;
    if (c->table)
        c->table.tableColumns[0].width = ((NSScrollView *)c->view).contentSize.width;
}

void plat_control_text(PlatControl *c, const char *utf8)
{
    @autoreleasepool {
        NSString *s = [NSString stringWithUTF8String:utf8] ?: @"";

        if (c->kind == PC_GROUPBOX)
            ((NSBox *)c->view).title = s;
        else if (c->kind == PC_EDIT)
            ((NSTextField *)c->view).stringValue = s;
        else if (c->label) {
            c->label.stringValue = s;
            fit_label(c);
        }
        else if (c->button) {
            c->button.title = s;
            fit_button(c);
        }
    }
}

void plat_control_check(PlatControl *c, int check)
{
    NSControlStateValue v = check == 1 ? NSControlStateValueOn : check == 2 ? NSControlStateValueMixed
                                                                          : NSControlStateValueOff;

    if (c->button && c->button.state != v)
        c->button.state = v;
}

void plat_control_focus(PlatWindow *top, PlatControl *c)
{
    NSResponder *want = c && c->keyView ? c->keyView : top->view, *now = top->win.firstResponder;

    /* an edit field's keyboard is its field editor */
    if ([now isKindOfClass:[NSTextView class]] && ((NSTextView *)now).delegate == (id)want)
        return;
    if (now == want)
        return;
    if (c)
        c->quiet++;
    [top->win makeFirstResponder:want];
    if (c)
        c->quiet--;
}

void plat_list_reload(PlatControl *c, int rows, int rowHeight)
{
    @autoreleasepool {
        if (!c->table)
            return;
        c->quiet++;
        c->nrows = rows;
        c->rowH = rowHeight > 0 ? rowHeight : 16;
        c->table.rowHeight = c->rowH;
        if (c->rows) {
            int width = (int)c->table.tableColumns[0].width, i;
            uint32_t *px = width > 0 ? malloc((size_t)width * (size_t)c->rowH * 4) : NULL;
            CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

            [c->rows removeAllObjects];
            for (i = 0; i < rows; i++) {
                id img = [NSNull null];

                /* the game draws each row (WM_DRAWITEM) */
                if (px && list_native_draw(c->hwnd, i, width, c->rowH, px)) {
                    CGContextRef ctx = CGBitmapContextCreate(px, (size_t)width, (size_t)c->rowH, 8,
                                                             (size_t)width * 4, cs,
                                                             kCGBitmapByteOrder32Little |
                                                                 (CGBitmapInfo)kCGImageAlphaNoneSkipFirst);
                    CGImageRef ci = ctx ? CGBitmapContextCreateImage(ctx) : NULL;

                    if (ci)
                        img = [[NSImage alloc] initWithCGImage:ci size:NSMakeSize(width, c->rowH)];
                    if (ci)
                        CGImageRelease(ci);
                    if (ctx)
                        CGContextRelease(ctx);
                }
                [c->rows addObject:img];
            }
            CGColorSpaceRelease(cs);
            free(px);
        }
        [c->table reloadData];
        c->quiet--;
    }
}

void plat_list_select(PlatControl *c, int row)
{
    if (!c->table)
        return;
    c->quiet++;
    if (row >= 0 && row < c->nrows)
        [c->table selectRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)row] byExtendingSelection:NO];
    else
        [c->table deselectAll:nil];
    c->quiet--;
}

void plat_list_top(PlatControl *c, int row)
{
    NSScrollView *sv = (NSScrollView *)c->view;

    if (!c->table)
        return;
    c->quiet++;
    [sv.contentView scrollToPoint:NSMakePoint(0, row * c->rowH)];
    [sv reflectScrolledClipView:sv.contentView];
    c->quiet--;
}
