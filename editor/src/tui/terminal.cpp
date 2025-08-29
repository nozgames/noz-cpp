#include "terminal.h"

#ifdef _WIN32
#include "windows_terminal.h"
#else
#include "linux_terminal.h"
#endif

Terminal::Terminal()
    : _main_window(nullptr)
    , _screen_width(0)
    , _screen_height(0)
    , _render_callback(nullptr)
{
}

Terminal::~Terminal()
{
    // Cleanup is handled by derived classes in their destructors
}

void Terminal::SetRenderCallback(RenderCallback callback)
{
    _render_callback = callback;
}

void Terminal::SetResizeCallback(ResizeCallback callback)
{
    _on_resize_callback = callback;
}

void Terminal::Render()
{
    if (_render_callback)
    {
        _render_callback(_screen_width, _screen_height);
    }
    refresh();
}

void Terminal::RequestRedraw()
{
    _needs_redraw = true;
}

bool Terminal::NeedsRedraw()
{
    return _needs_redraw.exchange(false);
}

int Terminal::GetKey()
{
    return getch();
}

int Terminal::GetWidth() const
{
    return _screen_width;
}

int Terminal::GetHeight() const
{
    return _screen_height;
}

WINDOW* Terminal::GetWindow() const
{
    return _main_window;
}

void Terminal::ClearScreen()
{
    erase();
}

void Terminal::MoveCursor(int y, int x)
{
    move(y, x);
}

void Terminal::AddChar(char ch)
{
    addch(ch);
}

void Terminal::AddString(const char* str)
{
    addstr(str);
}

void Terminal::SetColor(int pair)
{
    if (has_colors())
    {
        attron(COLOR_PAIR(pair));
    }
}

void Terminal::UnsetColor(int pair)
{
    if (has_colors())
    {
        attroff(COLOR_PAIR(pair));
    }
}

void Terminal::SetCursorVisible(bool visible)
{
    curs_set(visible ? 1 : 0);
}

bool Terminal::HasColorSupport() const
{
    return has_colors();
}

void Terminal::InitializeColors()
{
    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_BLACK);
        init_pair(3, COLOR_GREEN, COLOR_BLACK);
        init_pair(4, COLOR_RED, COLOR_BLACK);
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    }
}

void Terminal::InitializeTerminal()
{
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);
    timeout(50);
}

std::unique_ptr<Terminal> CreateTerminal()
{
#ifdef _WIN32
    return std::make_unique<WindowsTerminal>();
#else
    return std::make_unique<LinuxTerminal>();
#endif
}