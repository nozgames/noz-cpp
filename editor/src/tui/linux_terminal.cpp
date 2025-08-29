#ifndef _WIN32
#include "linux_terminal.h"
#include <noz/log.h>
#include <csignal>
#include <cstdlib>

static LinuxTerminal* g_active_terminal = nullptr;
static std::atomic<bool> g_window_resized = false;

void sigwinch_handler(int sig)
{
    if (sig == SIGWINCH)
    {
        g_window_resized = true;
    }
}

LinuxTerminal::LinuxTerminal()
{
}

LinuxTerminal::~LinuxTerminal()
{
    Cleanup();
}

bool LinuxTerminal::Initialize()
{
    _main_window = initscr();
    if (!_main_window)
    {
        return false;
    }

    getmaxyx(stdscr, _screen_height, _screen_width);
    LogDebug("LinuxTerminal initialization - Initial size: %dx%d", _screen_width, _screen_height);

    InitializeTerminal();
    InitializeColors();

    // Setup signal handler for resize
    signal(SIGWINCH, sigwinch_handler);
    keypad(stdscr, TRUE);
    setenv("LINES", "", 1);
    setenv("COLUMNS", "", 1);

    g_active_terminal = this;

    return true;
}

void LinuxTerminal::Cleanup()
{
    if (_main_window)
    {
        endwin();
        _main_window = nullptr;
    }

    if (g_active_terminal == this)
    {
        g_active_terminal = nullptr;
    }

    LogDebug("LinuxTerminal cleanup completed");
}

void LinuxTerminal::HandleResize()
{
    int new_height, new_width;

    LogDebug("LinuxTerminal HandleResize called");

    endwin();
    refresh();
    getmaxyx(stdscr, new_height, new_width);

    if (new_height <= 0 || new_width <= 0)
    {
        endwin();
        refresh();
        getmaxyx(stdscr, new_height, new_width);
    }

    if ((new_height != _screen_height || new_width != _screen_width) && new_height > 0 && new_width > 0)
    {
        LogDebug("Applying resize: %dx%d -> %dx%d", 
                 _screen_width, _screen_height, new_width, new_height);

        _screen_height = new_height;
        _screen_width = new_width;

        clear();

        if (_on_resize_callback)
        {
            _on_resize_callback(_screen_width, _screen_height);
        }

        RequestRedraw();
        Render();
    }
    else
    {
        LogDebug("No resize needed or invalid dimensions: %dx%d", new_width, new_height);
    }
}

void LinuxTerminal::CheckResize()
{
    static int counter = 0;
    if (++counter % 20 != 0) return;
    
    int new_height, new_width;
    getmaxyx(stdscr, new_height, new_width);
    if (new_height != _screen_height || new_width != _screen_width)
    {
        g_window_resized = true;
    }
}

bool LinuxTerminal::ShouldResize()
{
    return g_window_resized.exchange(false);
}

#endif // !_WIN32