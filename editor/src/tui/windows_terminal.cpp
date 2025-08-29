#ifdef _WIN32
#include "windows_terminal.h"
#include <noz/log.h>

static std::atomic<bool> g_window_resized = false;

WindowsTerminal::WindowsTerminal()
{
}

WindowsTerminal::~WindowsTerminal()
{
    Cleanup();
}

bool WindowsTerminal::Initialize()
{
    _main_window = initscr();
    if (!_main_window)
    {
        return false;
    }

    getmaxyx(stdscr, _screen_height, _screen_width);
    LogDebug("WindowsTerminal initialization - Initial size: %dx%d", _screen_width, _screen_height);

    InitializeTerminal();
    InitializeColors();

    // Start resize monitoring thread
    _resize_thread_running = true;
    _resize_thread = std::thread(&WindowsTerminal::ResizeThreadLoop, this);

    return true;
}

void WindowsTerminal::Cleanup()
{
    // Stop resize thread
    if (_resize_thread_running)
    {
        _resize_thread_running = false;
        if (_resize_thread.joinable())
        {
            _resize_thread.join();
        }
    }

    if (_main_window)
    {
        endwin();
        _main_window = nullptr;
    }

    LogDebug("WindowsTerminal cleanup completed");
}

void WindowsTerminal::HandleResize()
{
    // PDCurses resize_term(0,0) handles the resize for us
    resize_term(0, 0);
    
    int new_width, new_height;
    getmaxyx(stdscr, new_height, new_width);
    
    // Only proceed if dimensions are valid and changed
    if (new_width > 0 && new_height > 0 && 
        (new_width != _screen_width || new_height != _screen_height))
    {
        LogDebug("Resize: %dx%d -> %dx%d", _screen_width, _screen_height, new_width, new_height);
        
        _screen_width = new_width;
        _screen_height = new_height;
        
        // Clear and redraw
        clear();
        
        if (_on_resize_callback)
        {
            _on_resize_callback(_screen_width, _screen_height);
        }
        
        RequestRedraw();
    }
}

void WindowsTerminal::Update()
{
    // Check if resize thread detected a resize
    if (g_window_resized.exchange(false))
    {
        HandleResize();
    }
}

void WindowsTerminal::ResizeThreadLoop()
{
    LogDebug("Windows resize monitoring thread started");
    
    while (_resize_thread_running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        if (!_resize_thread_running) break;
        
        if (is_termresized())
        {
            LogDebug("PDCurses resize detected by thread");
            g_window_resized = true;
        }
    }
    
    LogDebug("Windows resize monitoring thread stopped");
}

#endif // _WIN32