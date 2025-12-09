//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include <Cocoa/Cocoa.h>
#include <QuartzCore/CAMetalLayer.h>
#include <filesystem>
#include <thread>

extern void InitMetal(const RendererTraits* traits, NSWindow* window, CAMetalLayer* layer);
extern void ResizeMetal(const Vec2Int& screen_size);
extern void ShutdownMetal();
extern void HandleInputCharacter(unichar c);
extern void HandleInputKeyDown(unsigned short keyCode);

struct MacOSApp
{
    const ApplicationTraits* traits;
    NSWindow* window;
    CAMetalLayer* metal_layer;
    NSView* view;
    Vec2Int screen_size;
    Vec2 mouse_position;
    Vec2 mouse_scroll;
    bool has_focus;
    bool is_resizing;
    void (*on_close)();
    NSCursor* cursor_default;
    NSCursor* cursor_wait;
    NSCursor* cursor_cross;
    NSCursor* cursor_move;
    SystemCursor cursor;
    noz::RectInt window_rect;
};

static MacOSApp g_macos = {};

// Custom NSView subclass to handle Metal layer
@interface MetalView : NSView
@end

@implementation MetalView
- (BOOL)wantsUpdateLayer {
    return YES;
}

- (CALayer*)makeBackingLayer {
    CAMetalLayer* layer = [CAMetalLayer layer];
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    return layer;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}
@end

// Window delegate to handle events
@interface WindowDelegate : NSObject <NSWindowDelegate>
@end

@implementation WindowDelegate
- (void)windowWillClose:(NSNotification*)notification {
    (void)notification;
    if (g_macos.on_close)
        g_macos.on_close();
}

- (void)windowDidResize:(NSNotification*)notification {
    (void)notification;
    NSRect contentRect = [g_macos.view frame];
    Vec2Int new_size = {
        static_cast<i32>(contentRect.size.width),
        static_cast<i32>(contentRect.size.height)
    };

    if (g_macos.screen_size != new_size && new_size != VEC2INT_ZERO) {
        g_macos.screen_size = new_size;

        // Update Metal layer size
        g_macos.metal_layer.drawableSize = CGSizeMake(new_size.x, new_size.y);

        ResizeMetal(new_size);
    }
}

- (void)windowDidMove:(NSNotification*)notification {
    (void)notification;
    NSRect frame = [g_macos.window frame];
    // macOS coordinates are bottom-left origin, flip to top-left
    NSRect screenFrame = [[NSScreen mainScreen] frame];
    g_macos.window_rect = noz::RectInt{
        static_cast<i32>(frame.origin.x),
        static_cast<i32>(screenFrame.size.height - frame.origin.y - frame.size.height),
        static_cast<i32>(frame.size.width),
        static_cast<i32>(frame.size.height)
    };
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    (void)notification;
    g_macos.has_focus = true;
}

- (void)windowDidResignKey:(NSNotification*)notification {
    (void)notification;
    g_macos.has_focus = false;
}

- (void)windowWillStartLiveResize:(NSNotification*)notification {
    (void)notification;
    g_macos.is_resizing = true;
}

- (void)windowDidEndLiveResize:(NSNotification*)notification {
    (void)notification;
    g_macos.is_resizing = false;
}
@end

bool GetWindowFocus()
{
    return g_macos.has_focus;
}

void ThreadSleep(int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void ThreadYield()
{
    std::this_thread::yield();
}

void SetThreadName(const char* name)
{
    pthread_setname_np(name);
}

void InitApplication(const ApplicationTraits* traits)
{
    g_macos = {};
    g_macos.traits = traits;
    g_macos.cursor = SYSTEM_CURSOR_DEFAULT;
    g_macos.cursor_default = [NSCursor arrowCursor];
    g_macos.cursor_wait = [NSCursor arrowCursor]; // macOS doesn't have a wait cursor that works the same way
    g_macos.cursor_cross = [NSCursor crosshairCursor];
    g_macos.cursor_move = [NSCursor openHandCursor];
}

void InitWindow(void (*on_close)())
{
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        // Create window
        NSRect frame = NSMakeRect(
            g_macos.traits->x < 0 ? 100 : g_macos.traits->x,
            g_macos.traits->y < 0 ? 100 : g_macos.traits->y,
            g_macos.traits->width,
            g_macos.traits->height
        );

        NSWindowStyleMask styleMask = NSWindowStyleMaskTitled |
                                      NSWindowStyleMaskClosable |
                                      NSWindowStyleMaskMiniaturizable |
                                      NSWindowStyleMaskResizable;

        g_macos.window = [[NSWindow alloc] initWithContentRect:frame
                                                     styleMask:styleMask
                                                       backing:NSBackingStoreBuffered
                                                         defer:NO];

        [g_macos.window setTitle:@(g_macos.traits->title)];
        [g_macos.window setAcceptsMouseMovedEvents:YES];

        // Create Metal view
        g_macos.view = [[MetalView alloc] initWithFrame:frame];
        [g_macos.window setContentView:g_macos.view];

        // Get the Metal layer
        g_macos.metal_layer = (CAMetalLayer*)[g_macos.view layer];

        // Set window delegate
        WindowDelegate* delegate = [[WindowDelegate alloc] init];
        [g_macos.window setDelegate:delegate];

        g_macos.on_close = on_close;

        // Initial screen size
        NSRect contentRect = [g_macos.view frame];
        g_macos.screen_size = {
            static_cast<i32>(contentRect.size.width),
            static_cast<i32>(contentRect.size.height)
        };

        // Set Metal layer drawable size
        g_macos.metal_layer.drawableSize = CGSizeMake(
            g_macos.screen_size.x,
            g_macos.screen_size.y
        );

        InitMetal(&g_macos.traits->renderer, g_macos.window, g_macos.metal_layer);

        [g_macos.window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    }
}

void ShutdownApplication()
{
    if (g_macos.window) {
        [g_macos.window close];
        g_macos.window = nil;
    }
}

bool HasFocus()
{
    return g_macos.has_focus;
}

bool IsResizing()
{
    return g_macos.is_resizing;
}

bool UpdateApplication()
{
    @autoreleasepool {
        g_macos.mouse_scroll = {0, 0};

        // Process events
        NSEvent* event;
        int count = 0;
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                           untilDate:nil
                                              inMode:NSDefaultRunLoopMode
                                             dequeue:YES]) && count < 10)
        {
            [NSApp sendEvent:event];
            count++;
        }

        g_macos.has_focus = [g_macos.window isKeyWindow];

        return true;
    }
}

Vec2Int GetScreenSize()
{
    return g_macos.screen_size;
}

void SetCursor(SystemCursor cursor)
{
    g_macos.cursor = cursor;

    NSCursor* ns_cursor = nil;
    switch (cursor) {
        case SYSTEM_CURSOR_DEFAULT:
            ns_cursor = g_macos.cursor_default;
            break;
        case SYSTEM_CURSOR_MOVE:
            ns_cursor = g_macos.cursor_move;
            break;
        case SYSTEM_CURSOR_SELECT:
            ns_cursor = g_macos.cursor_cross;
            break;
        case SYSTEM_CURSOR_WAIT:
            ns_cursor = g_macos.cursor_wait;
            break;
    }

    [ns_cursor set];
}

void ShowCursor(bool show)
{
    if (show)
        [NSCursor unhide];
    else
        [NSCursor hide];
}

Vec2 GetMousePosition()
{
    NSPoint mouse_location = [g_macos.window mouseLocationOutsideOfEventStream];
    // Flip Y coordinate (Cocoa uses bottom-left origin)
    NSRect contentRect = [g_macos.view frame];
    return Vec2{
        static_cast<f32>(mouse_location.x),
        static_cast<f32>(contentRect.size.height - mouse_location.y)
    };
}

Vec2 GetMouseScroll()
{
    return g_macos.mouse_scroll;
}

void FocusWindow()
{
    if (g_macos.window) {
        [g_macos.window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    }
}

std::filesystem::path GetSaveGamePath()
{
    @autoreleasepool {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(
            NSApplicationSupportDirectory,
            NSUserDomainMask,
            YES
        );

        if ([paths count] > 0) {
            NSString* app_support = paths[0];
            std::filesystem::path save_path = [app_support UTF8String];
            save_path /= g_macos.traits->name;

            std::error_code ec;
            std::filesystem::create_directories(save_path, ec);

            return save_path;
        }

        return std::filesystem::path(".");
    }
}

extern int main(int argc, char* argv[]);

int main(int argc, char* argv[])
{
    @autoreleasepool {
        return main(argc, argv);
    }
}

void Log(LogType type, const char* message)
{
    (void)type;
    NSLog(@"%s", message);
}

noz::RectInt GetWindowRect()
{
    return g_macos.window_rect;
}
