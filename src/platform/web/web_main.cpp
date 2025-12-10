//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Web/Emscripten platform main entry point
//

#include "../../platform.h"
#include "../../internal.h"

#include <emscripten.h>
#include <emscripten/html5.h>

extern void InitWebGL(const RendererTraits* traits, const char* canvas_id);
extern void ResizeWebGL(const Vec2Int& screen_size);
extern void ShutdownWebGL();
extern void HandleInputCharacter(char c);
extern void HandleInputKeyDown(char c);

struct WebApp {
    const ApplicationTraits* traits;
    Vec2Int screen_size;
    Vec2 mouse_position;
    Vec2 mouse_scroll;
    bool has_focus;
    void (*on_close)();
    SystemCursor cursor;
    bool mouse_on_screen;
    const char* canvas_id;
};

static WebApp g_web = {};

// Main loop callback for Emscripten
static void (*g_main_loop_callback)() = nullptr;

static void EmscriptenMainLoop() {
    if (g_main_loop_callback) {
        g_main_loop_callback();
    }
}

bool PlatformIsWindowFocused() {
    return g_web.has_focus;
}

void ThreadSleep(int milliseconds) {
    // Note: In Emscripten, we can't truly sleep without blocking
    // Use emscripten_sleep if asyncify is enabled
    (void)milliseconds;
}

void ThreadYield() {
    // No-op for single-threaded Emscripten
}

void PlatformSetThreadName(const char* name) {
    (void)name;
    // No thread naming support in web
}

// Keyboard event callback
static EM_BOOL OnKeyDown(int event_type, const EmscriptenKeyboardEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    // Handle special keys
    if (event->keyCode == 8) { // Backspace
        HandleInputKeyDown('\b');
    } else if (event->keyCode == 13) { // Enter
        HandleInputKeyDown('\r');
    } else if (event->keyCode == 27) { // Escape
        HandleInputKeyDown(27);
    } else if (event->keyCode == 46) { // Delete
        HandleInputKeyDown(127);
    }

    return EM_FALSE; // Don't consume event
}

static EM_BOOL OnKeyPress(int event_type, const EmscriptenKeyboardEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    // Handle printable characters
    if (event->charCode >= 32 && event->charCode < 127) {
        HandleInputCharacter((char)event->charCode);
    }

    return EM_FALSE;
}

// Mouse event callbacks
static EM_BOOL OnMouseMove(int event_type, const EmscriptenMouseEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    g_web.mouse_position = {
        static_cast<f32>(event->targetX),
        static_cast<f32>(event->targetY)
    };
    g_web.mouse_on_screen = true;

    return EM_FALSE;
}

static EM_BOOL OnMouseEnter(int event_type, const EmscriptenMouseEvent* event, void* user_data) {
    (void)event_type;
    (void)event;
    (void)user_data;

    g_web.mouse_on_screen = true;
    return EM_FALSE;
}

static EM_BOOL OnMouseLeave(int event_type, const EmscriptenMouseEvent* event, void* user_data) {
    (void)event_type;
    (void)event;
    (void)user_data;

    g_web.mouse_on_screen = false;
    return EM_FALSE;
}

static EM_BOOL OnMouseWheel(int event_type, const EmscriptenWheelEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    // Normalize wheel delta
    g_web.mouse_scroll.y -= static_cast<f32>(event->deltaY) / 100.0f;
    g_web.mouse_scroll.x -= static_cast<f32>(event->deltaX) / 100.0f;

    return EM_TRUE; // Consume event to prevent page scrolling
}

// Focus event callbacks
static EM_BOOL OnFocus(int event_type, const EmscriptenFocusEvent* event, void* user_data) {
    (void)event_type;
    (void)event;
    (void)user_data;

    g_web.has_focus = true;
    return EM_FALSE;
}

static EM_BOOL OnBlur(int event_type, const EmscriptenFocusEvent* event, void* user_data) {
    (void)event_type;
    (void)event;
    (void)user_data;

    g_web.has_focus = false;
    return EM_FALSE;
}

// Canvas resize callback
static EM_BOOL OnCanvasResize(int event_type, const EmscriptenUiEvent* event, void* user_data) {
    (void)event_type;
    (void)event;
    (void)user_data;

    // Get new canvas size
    double width, height;
    emscripten_get_element_css_size(g_web.canvas_id, &width, &height);

    Vec2Int new_size = {
        static_cast<i32>(width),
        static_cast<i32>(height)
    };

    if (g_web.screen_size != new_size && new_size.x > 0 && new_size.y > 0) {
        g_web.screen_size = new_size;
        emscripten_set_canvas_element_size(g_web.canvas_id, new_size.x, new_size.y);
        ResizeWebGL(new_size);
    }

    return EM_FALSE;
}

void PlatformInit(const ApplicationTraits* traits) {
    g_web = {};
    g_web.traits = traits;
    g_web.cursor = SYSTEM_CURSOR_DEFAULT;
    g_web.canvas_id = "#canvas";
    g_web.has_focus = true;
}

void PlatformInitWindow(void (*on_close)()) {
    g_web.on_close = on_close;

    // Get initial canvas size
    double width, height;
    emscripten_get_element_css_size(g_web.canvas_id, &width, &height);

    g_web.screen_size = {
        static_cast<i32>(width),
        static_cast<i32>(height)
    };

    // Set canvas size to match CSS size
    emscripten_set_canvas_element_size(g_web.canvas_id, g_web.screen_size.x, g_web.screen_size.y);

    // Initialize WebGL
    InitWebGL(&g_web.traits->renderer, g_web.canvas_id);

    // Register event callbacks
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, OnKeyDown);
    emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, OnKeyPress);
    emscripten_set_mousemove_callback(g_web.canvas_id, nullptr, EM_TRUE, OnMouseMove);
    emscripten_set_mouseenter_callback(g_web.canvas_id, nullptr, EM_TRUE, OnMouseEnter);
    emscripten_set_mouseleave_callback(g_web.canvas_id, nullptr, EM_TRUE, OnMouseLeave);
    emscripten_set_wheel_callback(g_web.canvas_id, nullptr, EM_TRUE, OnMouseWheel);
    emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnFocus);
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnBlur);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, OnCanvasResize);
}

void PlatformShutdown() {
    ShutdownWebGL();
}

bool PlatformIsResizing() {
    return false; // Web doesn't have the same resize behavior as desktop
}

bool PlatformUpdate() {
    // Reset per-frame state
    g_web.mouse_scroll = {0, 0};

    // Check for canvas size changes
    double width, height;
    emscripten_get_element_css_size(g_web.canvas_id, &width, &height);

    Vec2Int new_size = {
        static_cast<i32>(width),
        static_cast<i32>(height)
    };

    if (g_web.screen_size != new_size && new_size.x > 0 && new_size.y > 0) {
        g_web.screen_size = new_size;
        emscripten_set_canvas_element_size(g_web.canvas_id, new_size.x, new_size.y);
        ResizeWebGL(new_size);
    }

    return true; // Always running in web context
}

Vec2Int PlatformGetWindowSize() {
    return g_web.screen_size;
}

void PlatformSetCursor(SystemCursor cursor) {
    g_web.cursor = cursor;

    const char* cursor_style = "default";
    switch (cursor) {
        case SYSTEM_CURSOR_NONE:
            cursor_style = "none";
            break;
        case SYSTEM_CURSOR_DEFAULT:
            cursor_style = "default";
            break;
        case SYSTEM_CURSOR_MOVE:
            cursor_style = "move";
            break;
        case SYSTEM_CURSOR_SELECT:
            cursor_style = "crosshair";
            break;
        case SYSTEM_CURSOR_WAIT:
            cursor_style = "wait";
            break;
    }

    EM_ASM({
        document.body.style.cursor = UTF8ToString($0);
    }, cursor_style);
}

Vec2 PlatformGetMousePosition() {
    return g_web.mouse_position;
}

Vec2 PlatformGetMouseScroll() {
    return g_web.mouse_scroll;
}

void PlatformFocusWindow() {
    EM_ASM({
        document.getElementById('canvas').focus();
    });
}

std::filesystem::path PlatformGetSaveGamePath() {
    // Use IndexedDB path for Emscripten virtual filesystem
    return std::filesystem::path("/save");
}

noz::RectInt PlatformGetWindowRect() {
    return noz::RectInt{0, 0, g_web.screen_size.x, g_web.screen_size.y};
}

bool PlatformIsMouseOverWindow() {
    return g_web.mouse_on_screen;
}

void PlatformLog(LogType type, const char* message) {
    (void)type;

    // Log to browser console
    EM_ASM({
        console.log(UTF8ToString($0));
    }, message);
}

// Emscripten main loop setup - called from main()
void PlatformSetMainLoop(void (*callback)()) {
    g_main_loop_callback = callback;
    emscripten_set_main_loop(EmscriptenMainLoop, 0, 1);
}
