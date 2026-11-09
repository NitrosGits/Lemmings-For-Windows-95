/*
 * What the macOS backend's Objective-C files share: the layer's Win32 headers,
 * with the names AppKit's headers also have renamed (these files use none of
 * the Win32 ones), and the backend's own internals.
 */
#ifndef PORT_MACOS_COCOA_WIN32_H
#define PORT_MACOS_COCOA_WIN32_H

#import <AppKit/AppKit.h>

#define BOOL WINBOOL
#define LoadResource Win32_LoadResource
#undef FSHIFT
#include "port_internal.h"
#undef BOOL
#undef LoadResource

/* plat_cocoa.m: a top-level window, and the view showing its framebuffer
   (the native controls are its subviews) */
@interface PortWindow : NSWindow
@property(nonatomic) PlatWindow *plat;
@end

@interface PortView : NSView
@property(nonatomic) PlatWindow *plat;
@end

struct PlatWindow {
    PortWindow *win;
    PortView *view;
    Wnd *wnd;
    PlatWindow *owner;
    int flags;
    int showing;
};

/* keeps a window or view until the next pump: AppKit may still be inside it */
void cocoa_bury(id object);

/* menu_cocoa.m */
void cocoa_menu_init(id appDelegate);

#endif
