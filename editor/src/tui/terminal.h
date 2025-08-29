#pragma once

#include <functional>
#include <atomic>
#include <memory>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#undef MOUSE_MOVED
#include <windows.h>
#undef MOUSE_MOVED
#include <curses.h>
#else
#include <ncurses.h>
#endif

/**
 * Terminal class wraps ncurses/PDCurses and handles platform-specific behavior
 */
class Terminal {
public:
    using RenderCallback = std::function<void(int width, int height)>;
    using ResizeCallback = std::function<void(int new_width, int new_height)>;

    Terminal();
    virtual ~Terminal();

    virtual bool Initialize() = 0;
    virtual void Cleanup() = 0;

    void SetRenderCallback(RenderCallback callback);
    void SetResizeCallback(ResizeCallback callback);

    virtual void Update() = 0;

    void Render();
    void RequestRedraw();
    bool NeedsRedraw();
    int GetKey();

    int GetWidth() const;
    int GetHeight() const;
    WINDOW* GetWindow() const;

    void ClearScreen();
    void MoveCursor(int y, int x);
    void AddChar(char ch);
    void AddString(const char* str);
    void SetColor(int pair);
    void UnsetColor(int pair);
    void SetCursorVisible(bool visible);
    bool HasColorSupport() const;

    static constexpr int COLOR_STATUS_BAR = 1;
    static constexpr int COLOR_COMMAND_LINE = 2;
    static constexpr int COLOR_SUCCESS = 3;
    static constexpr int COLOR_ERROR = 4;
    static constexpr int COLOR_WARNING = 5;

protected:
    WINDOW* _main_window;
    int _screen_width;
    int _screen_height;
    
    RenderCallback _render_callback;
    ResizeCallback _on_resize_callback;
    
    std::atomic<bool> _needs_redraw = false;

    void InitializeColors();
    void InitializeTerminal();
};

std::unique_ptr<Terminal> CreateTerminal();