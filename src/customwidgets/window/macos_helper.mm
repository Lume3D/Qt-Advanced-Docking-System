#ifdef __APPLE__
#    include <Cocoa/Cocoa.h>
#endif
#include <QMainWindow>
#include <QWindow>
#include <QGuiApplication>
#include "macos_helper.h"
void HideTitleBar(long winId)
{
    #ifdef __APPLE__

    if(winId == -1)
    {
    QWindowList windows = QGuiApplication::allWindows();
    QWindow* win = windows.first();
    winId = win->winId();
    }

    NSView *nativeView = reinterpret_cast<NSView *>(winId);
    NSWindow* nativeWindow = [nativeView window];

    [nativeWindow setStyleMask:[nativeWindow styleMask] | NSWindowStyleMaskFullSizeContentView | NSWindowTitleHidden];
    [nativeWindow setTitlebarAppearsTransparent:YES];
    // [nativeWindow setMovableByWindowBackground:YES];
    #endif
}