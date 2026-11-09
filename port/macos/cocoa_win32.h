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

/* menu_cocoa.m */
void cocoa_menu_init(id appDelegate);

#endif
