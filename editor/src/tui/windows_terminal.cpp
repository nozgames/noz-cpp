#include "windows_terminal.h"

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
    int new_height, new_width;

    LogDebug("WindowsTerminal HandleResize called");

    // Method 1: Try resize_term first
    resize_term(0, 0);
    getmaxyx(stdscr, new_height, new_width);
    
    LogDebug("After resize_term: %dx%d", new_width, new_height);
    
    // Method 2: If dimensions are invalid or unchanged, force full reinit
    if (new_height <= 0 || new_width <= 0 || 
        (new_height == _screen_height && new_width == _screen_width))
    {
        LogDebug("resize_term failed or no change, trying full reinit");
        
        endwin();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        _main_window = initscr();
        if (_main_window)
        {
            InitializeTerminal();
            InitializeColors();
            
            getmaxyx(stdscr, new_height, new_width);
            LogDebug("After full reinit: %dx%d", new_width, new_height);
        }
    }
    
    // Method 3: Last resort
    if (new_height <= 0 || new_width <= 0)
    {
        LogDebug("Dimensions still invalid, trying refresh approach");
        clear();
        refresh();
        getmaxyx(stdscr, new_height, new_width);
        LogDebug("After refresh: %dx%d", new_width, new_height);
    }

    // Apply resize if dimensions changed and are valid
    if ((new_height != _screen_height || new_width != _screen_width) && new_height > 0 && new_width > 0)
    {
        LogDebug("Applying resize: %dx%d -> %dx%d", 
                 _screen_width, _screen_height, new_width, new_height);

        _screen_height = new_height;
        _screen_width = new_width;

        erase();
        clear();
        clearok(stdscr, TRUE);
        wrefresh(stdscr);

        if (_on_resize_callback)
        {
            _on_resize_callback(_screen_width, _screen_height);
        }

        RequestRedraw();
        
        if (_render_callback)
        {
            _render_callback(_screen_width, _screen_height);
        }
        refresh();
    }
    else
    {
        LogDebug("No resize needed or invalid dimensions: %dx%d", new_width, new_height);
    }
}

void WindowsTerminal::CheckResize()
{
    // Windows resize detection handled by background thread
    return;
}

bool WindowsTerminal::ShouldResize()
{
    return g_window_resized.exchange(false);
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