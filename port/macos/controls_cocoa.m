/*
 * macOS: the native controls of plat.h - AppKit views over a dialog's push
 * buttons, check boxes, radio buttons, group boxes, edit fields and list
 * boxes, as subviews of the window's PortView (whose layer shows the
 * framebuffer). The layer (port/src/peer.c) owns their state and pushes it
 * here; a view only reports what the user did (input_control*), and the layer
 * answers by pushing its new state back. Views made here while the layer
 * pushes are quiet: they report nothing.
 */
#include "cocoa_win32.h"

struct PlatControl {
    NSView *view;           /* the view in the window (a list's scroll view) */
    NSView *keyView;        /* what takes the keyboard */
    NSButton *button;
    NSTableView *table;
    NSMutableArray *rows;   /* an owner-drawn list's rows, as images */
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
           boxes), e(dit fields), l(ists), t(he owner-drawn list); default all */
        const char *only = getenv("LEMMINGS_NATIVE_CONTROLS");
        char k = kind <= PC_RADIO ? 'b' : kind == PC_GROUPBOX ? 'g' : kind == PC_EDIT ? 'e' : kind == PC_LIST ? 'l' : 't';

        if (only && !strchr(only, k))
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
                t.backgroundColor = NSColor.whiteColor;
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
        const char *only = getenv("LEMMINGS_NATIVE_CONTROLS");
        PlatControl *c;
        PortScroller *s;

        if (only && !strchr(only, 's'))
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
    }
    free(c);
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
