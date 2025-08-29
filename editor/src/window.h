#pragma once

#ifdef _WIN32
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include <functional>
#include <atomic>

/**
 * Window class handles ncurses initialization, resize events, and provides
 * a clean interface for rendering through callbacks.
 */
class Window {
public:
    // Callback function types
    using RenderCallback = std::function<void(int width, int height)>;
    using ResizeCallback = std::function<void(int new_width, int new_height)>;

    Window();
    ~Window();

    // Initialization and cleanup
    bool Initialize();
    void Cleanup();

    // Callback management
    void SetRenderCallback(RenderCallback callback);
    void SetResizeCallback(ResizeCallback callback);

    // Resize handling
    void HandleResize();
    void CheckResize();
    bool ShouldResize();

    // Rendering
    void Render();
    void RequestRedraw();
    bool NeedsRedraw();

    // Input handling
    int GetKey();

    // Window properties
    int GetWidth() const;
    int GetHeight() const;
    WINDOW* GetWindow() const;

    // Drawing utilities
    void ClearScreen();
    void MoveCursor(int y, int x);
    void AddChar(char ch);
    void AddString(const char* str);
    void SetColor(int pair);
    void UnsetColor(int pair);
    void SetCursorVisible(bool visible);
    bool HasColorSupport() const;

    // Color pairs (predefined)
    static constexpr int COLOR_STATUS_BAR = 1;     // Black on white
    static constexpr int COLOR_COMMAND_LINE = 2;   // White on black
    static constexpr int COLOR_SUCCESS = 3;        // Green on black
    static constexpr int COLOR_ERROR = 4;          // Red on black
    static constexpr int COLOR_WARNING = 5;        // Yellow on black

private:
    WINDOW* _main_window;
    int _screen_width;
    int _screen_height;
    
    RenderCallback _render_callback;
    ResizeCallback _on_resize_callback;
    
    std::atomic<bool> _needs_redraw = false;
    bool _running;
};