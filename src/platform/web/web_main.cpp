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
    bool is_mobile;
    bool is_portrait;
    bool fullscreen_requested;
};

static WebApp g_web = {};

// Check if device is mobile based on touch support and screen size
static bool DetectMobile() {
    return EM_ASM_INT({
        return ('ontouchstart' in window) ||
               (navigator.maxTouchPoints > 0) ||
               (window.innerWidth <= 1024 && window.innerHeight <= 1024);
    }) != 0;
}

// Check if screen is in portrait mode (taller than wide)
static bool IsPortrait() {
    return EM_ASM_INT({
        return window.innerHeight > window.innerWidth ? 1 : 0;
    }) != 0;
}

// Request fullscreen and lock to landscape orientation
static void RequestLandscapeFullscreen() {
    EM_ASM({
        var canvas = document.getElementById('canvas');
        if (!canvas) return;

        var requestFullscreen = canvas.requestFullscreen ||
                                canvas.webkitRequestFullscreen ||
                                canvas.mozRequestFullScreen ||
                                canvas.msRequestFullscreen;

        if (requestFullscreen) {
            requestFullscreen.call(canvas).then(function() {
                // Try to lock orientation to landscape
                if (screen.orientation && screen.orientation.lock) {
                    screen.orientation.lock('landscape').catch(function(e) {
                        console.log('Could not lock orientation:', e);
                    });
                }
            }).catch(function(e) {
                console.log('Could not enter fullscreen:', e);
            });
        }
    });
}

static void EmscriptenMainLoop() {
    if (!IsApplicationRunning()) {
        ShutdownApplication();
        emscripten_cancel_main_loop();
        return;
    }
    RunApplicationFrame();
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

    // Scale mouse position by device pixel ratio to match canvas buffer coordinates
    double dpr = emscripten_get_device_pixel_ratio();
    g_web.mouse_position = {
        static_cast<f32>(event->targetX * dpr),
        static_cast<f32>(event->targetY * dpr)
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

// Touch event callbacks - treat touch as mouse for mobile support
extern bool* GetKeyStates();

static EM_BOOL OnTouchStart(int event_type, const EmscriptenTouchEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    // On mobile in portrait mode, request fullscreen + landscape on first touch
    if (g_web.is_mobile && !g_web.fullscreen_requested && IsPortrait()) {
        g_web.fullscreen_requested = true;
        RequestLandscapeFullscreen();
    }

    if (event->numTouches > 0) {
        const EmscriptenTouchPoint& touch = event->touches[0];
        double dpr = emscripten_get_device_pixel_ratio();
        g_web.mouse_position = {
            static_cast<f32>(touch.targetX * dpr),
            static_cast<f32>(touch.targetY * dpr)
        };
        g_web.mouse_on_screen = true;

        // Simulate mouse left button press
        bool* key_states = GetKeyStates();
        if (key_states) {
            key_states[MOUSE_LEFT] = true;
        }
    }

    return EM_TRUE; // Consume event to prevent scrolling/zooming
}

static EM_BOOL OnTouchMove(int event_type, const EmscriptenTouchEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    if (event->numTouches > 0) {
        const EmscriptenTouchPoint& touch = event->touches[0];
        double dpr = emscripten_get_device_pixel_ratio();
        g_web.mouse_position = {
            static_cast<f32>(touch.targetX * dpr),
            static_cast<f32>(touch.targetY * dpr)
        };
    }

    return EM_TRUE; // Consume event
}

static EM_BOOL OnTouchEnd(int event_type, const EmscriptenTouchEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    // Simulate mouse left button release
    bool* key_states = GetKeyStates();
    if (key_states) {
        key_states[MOUSE_LEFT] = false;
    }

    // If all touches ended, mouse is no longer on screen
    if (event->numTouches == 0) {
        g_web.mouse_on_screen = false;
    }

    return EM_TRUE; // Consume event
}

static EM_BOOL OnTouchCancel(int event_type, const EmscriptenTouchEvent* event, void* user_data) {
    (void)event_type;
    (void)event;
    (void)user_data;

    // Simulate mouse left button release
    bool* key_states = GetKeyStates();
    if (key_states) {
        key_states[MOUSE_LEFT] = false;
    }

    g_web.mouse_on_screen = false;
    return EM_TRUE;
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

    // Get CSS size and device pixel ratio for proper high-DPI support
    double css_width, css_height;
    emscripten_get_element_css_size(g_web.canvas_id, &css_width, &css_height);

    double dpr = emscripten_get_device_pixel_ratio();

    // Canvas buffer size should be CSS size * devicePixelRatio for sharp rendering
    Vec2Int new_size = {
        static_cast<i32>(css_width * dpr),
        static_cast<i32>(css_height * dpr)
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

    // Detect if we're on a mobile device
    g_web.is_mobile = DetectMobile();
    g_web.is_portrait = IsPortrait();
    g_web.fullscreen_requested = false;

    // Get initial canvas CSS size and device pixel ratio
    double css_width, css_height;
    emscripten_get_element_css_size(g_web.canvas_id, &css_width, &css_height);

    double dpr = emscripten_get_device_pixel_ratio();

    // Canvas buffer size should be CSS size * devicePixelRatio for sharp rendering
    g_web.screen_size = {
        static_cast<i32>(css_width * dpr),
        static_cast<i32>(css_height * dpr)
    };

    // Set canvas drawing buffer size (physical pixels)
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

    // Touch events for mobile support
    emscripten_set_touchstart_callback(g_web.canvas_id, nullptr, EM_TRUE, OnTouchStart);
    emscripten_set_touchmove_callback(g_web.canvas_id, nullptr, EM_TRUE, OnTouchMove);
    emscripten_set_touchend_callback(g_web.canvas_id, nullptr, EM_TRUE, OnTouchEnd);
    emscripten_set_touchcancel_callback(g_web.canvas_id, nullptr, EM_TRUE, OnTouchCancel);
}

void PlatformShutdown() {
    ShutdownWebGL();
}

bool PlatformUpdate() {
    // Reset per-frame state
    g_web.mouse_scroll = {0, 0};

    // Update portrait state
    g_web.is_portrait = IsPortrait();

    // Check for canvas size changes (including DPI changes)
    double css_width, css_height;
    emscripten_get_element_css_size(g_web.canvas_id, &css_width, &css_height);

    double dpr = emscripten_get_device_pixel_ratio();

    Vec2Int new_size = {
        static_cast<i32>(css_width * dpr),
        static_cast<i32>(css_height * dpr)
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

bool PlatformSavePersistentData(const char* name, const void* data, u32 size) {
    // Convert binary data to base64 and store in localStorage
    EM_ASM({
        var name = UTF8ToString($0);
        var dataPtr = $1;
        var size = $2;

        // Read bytes into array
        var bytes = new Uint8Array(size);
        for (var i = 0; i < size; i++) {
            bytes[i] = HEAPU8[dataPtr + i];
        }

        // Convert to base64
        var binary = "";
        for (var i = 0; i < bytes.length; i++) {
            binary += String.fromCharCode(bytes[i]);
        }
        var base64 = btoa(binary);

        // Store in localStorage with prefix
        localStorage.setItem('save_' + name, base64);
    }, name, data, size);

    return true;
}

u8* PlatformLoadPersistentData(Allocator* allocator, const char* name, u32* out_size) {
    // Get size first
    int size = EM_ASM_INT({
        var name = UTF8ToString($0);
        var base64 = localStorage.getItem('save_' + name);
        if (!base64) return 0;

        var binary = atob(base64);
        return binary.length;
    }, name);

    if (size == 0) {
        *out_size = 0;
        return nullptr;
    }

    u8* data = static_cast<u8*>(Alloc(allocator, size));

    // Load the data
    EM_ASM({
        var name = UTF8ToString($0);
        var dataPtr = $1;

        var base64 = localStorage.getItem('save_' + name);
        var binary = atob(base64);

        for (var i = 0; i < binary.length; i++) {
            HEAPU8[dataPtr + i] = binary.charCodeAt(i);
        }
    }, name, data);

    *out_size = static_cast<u32>(size);
    return data;
}

std::filesystem::path PlatformGetBinaryPath() {
    return std::filesystem::path("/");
}

std::filesystem::path PatformGetCurrentPath() {
    return std::filesystem::path("/");
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

bool PlatformIsMobile() {
    return g_web.is_mobile;
}

bool PlatformIsPortrait() {
    return g_web.is_portrait;
}

void PlatformRequestLandscape() {
    if (!g_web.fullscreen_requested) {
        g_web.fullscreen_requested = true;
        RequestLandscapeFullscreen();
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Main();
    emscripten_set_main_loop(EmscriptenMainLoop, 0, 1);

    return 0;
}
