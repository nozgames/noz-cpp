#include "window.h"
#include <csignal>
#ifndef _WIN32
#include <cstdlib> // for setenv
#endif

// Global pointer to active window for signal handling
static Window* g_active_window = nullptr;

// Global flag for window resize
static std::atomic<bool> g_window_resized = false;

#ifndef _WIN32
// Signal handler for window resize (Unix/Linux only)
void sigwinch_handler(int sig)
{
    if (sig == SIGWINCH)
    {
        g_window_resized = true;
    }
}
#endif

Window::Window()
    : _main_window(nullptr)
    , _screen_width(0)
    , _screen_height(0)
    , _render_callback(nullptr)
    , _running(false)
{
}

Window::~Window()
{
    Cleanup();
}

bool Window::Initialize()
{
    _main_window = initscr();
    if (!_main_window)
    {
        return false;
    }

    getmaxyx(stdscr, _screen_height, _screen_width);

    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);
    timeout(50);

#ifndef _WIN32
    signal(SIGWINCH, sigwinch_handler);
    keypad(stdscr, TRUE);
    setenv("LINES", "", 1);
    setenv("COLUMNS", "", 1);
#endif

    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_RED, COLOR_BLACK);
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    }

    g_active_window = this;

    return true;
}

void Window::Cleanup()
{
    if (_main_window)
    {
        endwin();
        _main_window = nullptr;
    }

    if (g_active_window == this)
    {
        g_active_window = nullptr;
    }
}

void Window::SetRenderCallback(RenderCallback callback)
{
    _render_callback = callback;
}

void Window::HandleResize()
{
    int new_height, new_width;

#ifdef _WIN32
    // Windows PDCurses approach
    resize_term(0, 0);
    getmaxyx(stdscr, new_height, new_width);
#else
    // Unix/Linux approach
    endwin();
    refresh();
    getmaxyx(stdscr, new_height, new_width);

    // If dimensions are still invalid after refresh, try once more
    if (new_height <= 0 || new_width <= 0)
    {
        endwin();
        refresh();
        getmaxyx(stdscr, new_height, new_width);
    }
#endif

    // Only proceed if dimensions actually changed and are valid
    if ((new_height != _screen_height || new_width != _screen_width) && new_height > 0 && new_width > 0)
    {

        // Update stored dimensions
        _screen_height = new_height;
        _screen_width = new_width;

        // Clear the entire screen buffer
        clear();

        if (_on_resize_callback)
        {
            _on_resize_callback(_screen_width, _screen_height);
        }

        RequestRedraw();
        Render();
    }
}

void Window::CheckResize()
{
    // Only check resize occasionally to avoid performance impact
    static int resize_check_counter = 0;
    resize_check_counter++;

    if (resize_check_counter % 20 != 0)
    {
        return;
    }

#ifdef _WIN32
    // On Windows, poll for resize since no SIGWINCH
    int new_height, new_width;
    getmaxyx(stdscr, new_height, new_width);
    if (new_height != _screen_height || new_width != _screen_width)
    {
        g_window_resized = true;
    }
#else
    // On Unix/Linux, rely on signal handler but still check occasionally
    int new_height, new_width;
    getmaxyx(stdscr, new_height, new_width);
    if (new_height != _screen_height || new_width != _screen_width)
    {
        g_window_resized = true;
    }
#endif
}

bool Window::ShouldResize()
{
    return g_window_resized.exchange(false);
}

void Window::Render()
{
    if (_render_callback)
    {
        _render_callback(_screen_width, _screen_height);
    }
    refresh();
}

void Window::RequestRedraw()
{
    _needs_redraw = true;
}

bool Window::NeedsRedraw()
{
    return _needs_redraw.exchange(false);
}

int Window::GetKey()
{
    return getch();
}

int Window::GetWidth() const
{
    return _screen_width;
}

int Window::GetHeight() const
{
    return _screen_height;
}

WINDOW* Window::GetWindow() const
{
    return _main_window;
}

void Window::SetResizeCallback(ResizeCallback callback)
{
    _on_resize_callback = callback;
}

void Window::ClearScreen()
{
    erase();
}

void Window::MoveCursor(int y, int x)
{
    move(y, x);
}

void Window::AddChar(char ch)
{
    addch(ch);
}

void Window::AddString(const char* str)
{
    addstr(str);
}

void Window::SetColor(int pair)
{
    if (has_colors())
    {
        attron(COLOR_PAIR(pair));
    }
}

void Window::UnsetColor(int pair)
{
    if (has_colors())
    {
        attroff(COLOR_PAIR(pair));
    }
}

void Window::SetCursorVisible(bool visible)
{
    curs_set(visible ? 1 : 0);
}

bool Window::HasColorSupport() const
{
    return has_colors();
}