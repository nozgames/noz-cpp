//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef _WIN32

#include "terminal.h"
#include <noz/log.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <sstream>
#include <algorithm>

static HANDLE g_console_input = INVALID_HANDLE_VALUE;
static HANDLE g_console_output = INVALID_HANDLE_VALUE;

static int g_screen_width = 0;
static int g_screen_height = 0;
static int g_cursor_x = 0;
static int g_cursor_y = 0;

// Output buffer for batched VT sequences
static std::string g_output_buffer;

static TerminalRenderCallback g_render_callback = nullptr;
static TerminalResizeCallback g_resize_callback = nullptr;
static std::atomic<bool> g_needs_redraw = false;

// VT sequence color codes for different terminal colors
static const char* g_color_sequences[] = {
    "\033[0m",           // Default (reset)
    "\033[30;47m",       // STATUS_BAR (black on white)
    "\033[37;40m",       // COMMAND_LINE (white on black)  
    "\033[92m",          // SUCCESS (bright green)
    "\033[91m",          // ERROR (bright red)
    "\033[93m"           // WARNING (bright yellow)
};

static int g_current_color = 0;

void SetRenderCallback(TerminalRenderCallback callback)
{
    g_render_callback = callback;
}

void SetResizeCallback(TerminalResizeCallback callback)
{
    g_resize_callback = callback;
}

void UpdateScreenSize()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi = {};
    if (!GetConsoleScreenBufferInfo(g_console_output, &csbi))
        return;

    int new_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int new_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    static int debug_counter = 0;
    debug_counter++;

    // Debug every 100 calls to see if we're polling
    if (debug_counter % 100 == 0)
    {
        LogDebug("UpdateScreenSize poll #%d: current=%dx%d, detected=%dx%d",
                 debug_counter, g_screen_width, g_screen_height, new_width, new_height);
    }

    if (new_width != g_screen_width || new_height != g_screen_height)
    {
        LogDebug("Console size changed: %dx%d -> %dx%d", g_screen_width, g_screen_height, new_width, new_height);

        // Update to new dimensions
        g_screen_width = new_width;
        g_screen_height = new_height;

        // Set buffer size to match window size (removes scrollbar)
        COORD buffer_size = {(SHORT)new_width, (SHORT)new_height};
        SetConsoleScreenBufferSize(g_console_output, buffer_size);
        LogDebug("Set buffer size to %dx%d", new_width, new_height);

        if (g_resize_callback)
        {
            g_resize_callback(g_screen_width, g_screen_height);
        }

        // Force immediate redraw after resize
        RequestRender();
        RenderTerminal();
    }
}

void RenderTerminal()
{
    LogDebug("RenderTerminal() called");
    if (!g_render_callback)
        return;

    // Clear the output buffer
    g_output_buffer.clear();
    
    // Reset cursor position for rendering
    g_cursor_x = 0;
    g_cursor_y = 0;

    // Always call the render callback to populate the output buffer
    g_render_callback(g_screen_width, g_screen_height);
        
    // Write the entire output buffer at once for maximum performance
    if (!g_output_buffer.empty())
    {
        DWORD written;
        WriteConsoleA(g_console_output, g_output_buffer.c_str(), 
                      (DWORD)g_output_buffer.length(), &written, nullptr);
        FlushFileBuffers(g_console_output);  // Force immediate output
    }
    else
    {
        LogDebug("Output buffer is empty!");
    }
    
    // Clear the redraw flag
    g_needs_redraw = false;
}

void RequestRender()
{
    LogDebug("RequestRender() called");
    g_needs_redraw = true;
}

int GetTerminalKey()
{
    INPUT_RECORD input_record;
    DWORD events_read;
    
    // Check if input is available without blocking
    DWORD events_available;
    if (!GetNumberOfConsoleInputEvents(g_console_input, &events_available) || events_available == 0)
    {
        return ERR; // No input available
    }
    
    while (ReadConsoleInput(g_console_input, &input_record, 1, &events_read) && events_read > 0)
    {
        if (input_record.EventType == KEY_EVENT && input_record.Event.KeyEvent.bKeyDown)
        {
            // Handle special keys
            if (input_record.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
            {
                return 27; // ESC
            }
            
            // Return the ASCII character
            char ch = input_record.Event.KeyEvent.uChar.AsciiChar;
            if (ch != 0)
            {
                return ch;
            }
        }
        else if (input_record.EventType == MOUSE_EVENT)
        {
            // Handle mouse events (including scroll wheel)
            MOUSE_EVENT_RECORD& mouse = input_record.Event.MouseEvent;
            if (mouse.dwEventFlags == MOUSE_WHEELED)
            {
                LogDebug("Mouse wheel event intercepted");
                return KEY_MOUSE; // Return special mouse key
            }
        }
        else if (input_record.EventType == WINDOW_BUFFER_SIZE_EVENT)
        {
            // Handle resize events immediately
            LogDebug("WINDOW_BUFFER_SIZE_EVENT received");
            UpdateScreenSize();
        }
    }
    
    return ERR; // No input available
}

int GetTerminalWidth()
{
    return g_screen_width;
}

int GetTerminalHeight()
{
    return g_screen_height;
}

void ClearScreen()
{
    // Add VT sequence to clear entire screen and move cursor to home
    // Also reset any color attributes to prevent color bleeding
    g_output_buffer += "\033[0m\033[2J\033[H";
    g_cursor_x = 0;
    g_cursor_y = 0;
    g_current_color = 0;  // Reset current color tracking
}

void MoveCursor(int y, int x)
{
    // Always add VT sequence - disable optimization for now to avoid sync issues
    // VT sequence: ESC[row;columnH (1-based coordinates)
    g_output_buffer += "\033[" + std::to_string(y + 1) + ";" + std::to_string(x + 1) + "H";
    g_cursor_x = x;
    g_cursor_y = y;
}

void AddChar(char ch)
{
    // Only add character if within screen bounds
    if (g_cursor_y >= 0 && g_cursor_y < g_screen_height && 
        g_cursor_x >= 0 && g_cursor_x < g_screen_width)
    {
        g_output_buffer += ch;
    }
    
    // Advance cursor position tracking
    g_cursor_x++;
    if (g_cursor_x >= g_screen_width)
    {
        g_cursor_x = 0;
        g_cursor_y++;
    }
}

void AddString(const char* str)
{
    // More efficient string addition - add entire string at once
    if (str && *str && g_cursor_y >= 0 && g_cursor_y < g_screen_height)
    {
        // Calculate how much of the string fits on current line
        int remaining_on_line = g_screen_width - g_cursor_x;
        int str_len = static_cast<int>(strlen(str));
        
        if (remaining_on_line > 0)
        {
            int chars_to_add = std::min(str_len, remaining_on_line);
            g_output_buffer.append(str, chars_to_add);
            
            // Update cursor position
            g_cursor_x += chars_to_add;
            if (g_cursor_x >= g_screen_width)
            {
                g_cursor_x = 0;
                g_cursor_y++;
            }
        }
    }
}

void SetColor(int pair)
{
    if (pair >= 0 && pair < (int)(sizeof(g_color_sequences) / sizeof(g_color_sequences[0])))
    {
        if (pair != g_current_color)
        {
            g_output_buffer += g_color_sequences[pair];
            g_current_color = pair;
        }
    }
}

void UnsetColor(int pair)
{
    // Reset to default color (index 0)
    if (g_current_color != 0)
    {
        g_output_buffer += g_color_sequences[0];
        g_current_color = 0;
    }
}

void SetColor256(int fg, int bg)
{
    std::string color_seq = "\033[";
    
    if (fg >= 0 && fg <= 255)
    {
        color_seq += "38;5;" + std::to_string(fg);
        if (bg >= 0 && bg <= 255)
        {
            color_seq += ";48;5;" + std::to_string(bg);
        }
    }
    else if (bg >= 0 && bg <= 255)
    {
        color_seq += "48;5;" + std::to_string(bg);
    }
    
    color_seq += "m";
    g_output_buffer += color_seq;
}

void SetColorRGB(int r, int g, int b, int bg_r, int bg_g, int bg_b)
{
    std::string color_seq = "\033[";
    
    // Clamp RGB values to 0-255
    r = std::max(0, std::min(255, r));
    g = std::max(0, std::min(255, g));
    b = std::max(0, std::min(255, b));
    
    // Set foreground RGB color
    color_seq += "38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b);
    
    // Set background RGB color if provided
    if (bg_r >= 0 && bg_g >= 0 && bg_b >= 0)
    {
        bg_r = std::max(0, std::min(255, bg_r));
        bg_g = std::max(0, std::min(255, bg_g));
        bg_b = std::max(0, std::min(255, bg_b));
        color_seq += ";48;2;" + std::to_string(bg_r) + ";" + std::to_string(bg_g) + ";" + std::to_string(bg_b);
    }
    
    color_seq += "m";
    g_output_buffer += color_seq;
}

void SetBold(bool enabled)
{
    if (enabled)
    {
        g_output_buffer += "\033[1m";
    }
    else
    {
        g_output_buffer += "\033[22m";  // Reset bold/dim
    }
}

void SetUnderline(bool enabled)
{
    if (enabled)
    {
        g_output_buffer += "\033[4m";
    }
    else
    {
        g_output_buffer += "\033[24m";  // Reset underline
    }
}

void SetItalic(bool enabled)
{
    if (enabled)
    {
        g_output_buffer += "\033[3m";
    }
    else
    {
        g_output_buffer += "\033[23m";  // Reset italic
    }
}

void SetScrollRegion(int top, int bottom)
{
    // VT sequence: ESC[top;bottomr sets scroll region (1-based)
    // Clamp values to valid screen range
    top = std::max(1, std::min(g_screen_height, top));
    bottom = std::max(top, std::min(g_screen_height, bottom));
    
    g_output_buffer += "\033[" + std::to_string(top) + ";" + std::to_string(bottom) + "r";
}

void ResetScrollRegion()
{
    // VT sequence: ESC[r resets scroll region to full screen
    g_output_buffer += "\033[r";
}

void SetCursorVisible(bool visible)
{
    LogDebug("SetCursorVisible: %s", visible ? "true" : "false");
    // Use VT sequence to show/hide cursor: \033[?25h (show) or \033[?25l (hide)
    if (visible)
    {
        g_output_buffer += "\033[?25h";
    }
    else
    {
        g_output_buffer += "\033[?25l";
    }
}

bool HasColorSupport()
{
    return true; // Windows console always supports colors
}

void UpdateTerminal()
{
    // Check for resize events by polling window size
    UpdateScreenSize();
}

void InitTerminal()
{
    // Get console handles
    g_console_input = GetStdHandle(STD_INPUT_HANDLE);
    g_console_output = GetStdHandle(STD_OUTPUT_HANDLE);
    
    if (g_console_input == INVALID_HANDLE_VALUE || g_console_output == INVALID_HANDLE_VALUE)
        return;

    // Set input mode to capture mouse and window events
    DWORD input_mode;
    GetConsoleMode(g_console_input, &input_mode);
    input_mode &= ~ENABLE_LINE_INPUT;
    input_mode &= ~ENABLE_ECHO_INPUT;
    input_mode &= ~ENABLE_PROCESSED_INPUT;
    input_mode &= ~ENABLE_AUTO_POSITION;
    input_mode |= ENABLE_MOUSE_INPUT;
    input_mode |= ENABLE_WINDOW_INPUT;
    input_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    input_mode |= ENABLE_QUICK_EDIT_MODE;
    input_mode |= ENABLE_EXTENDED_FLAGS;
    SetConsoleMode(g_console_input, input_mode);
    
    // Set output mode - this is the key change for VT sequences
    DWORD output_mode;
    GetConsoleMode(g_console_output, &output_mode);
    LogDebug("Original console output mode: 0x%08x", output_mode);
    
    output_mode |= ENABLE_PROCESSED_OUTPUT;
    output_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;  // Enable VT sequence processing
    output_mode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
    
    BOOL result = SetConsoleMode(g_console_output, output_mode);
    LogDebug("SetConsoleMode result: %s, new mode: 0x%08x", result ? "SUCCESS" : "FAILED", output_mode);
    
    if (!result) {
        LogDebug("Failed to enable VT processing, error: %d", GetLastError());
        // Fall back to legacy console API if VT sequences aren't supported
    }
    
    // Verify VT processing is actually enabled
    DWORD final_mode;
    GetConsoleMode(g_console_output, &final_mode);
    LogDebug("Final console output mode: 0x%08x", final_mode);
    LogDebug("VT processing enabled: %s", (final_mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? "YES" : "NO");
    
    // Initialize screen size
    UpdateScreenSize();
    
    // Force remove scrollbar by setting buffer size aggressively
    COORD buffer_size = {(SHORT)g_screen_width, (SHORT)g_screen_height};
    SetConsoleScreenBufferSize(g_console_output, buffer_size);

    // Initialize output buffer and cursor position
    g_output_buffer.reserve(g_screen_width * g_screen_height * 2);  // Reserve space for efficiency
    g_cursor_x = 0;
    g_cursor_y = 0;
    g_current_color = 0;

    // Switch to alternate screen buffer and hide cursor
    g_output_buffer.clear();
    g_output_buffer += "\033[?1049h";    // Enable alternate screen buffer
    g_output_buffer += "\033[2J";        // Clear screen
    g_output_buffer += "\033[H";         // Move to home position
    g_output_buffer += "\033[?25l";      // Hide cursor
    
    // Write initial setup sequences
    DWORD written;
    WriteConsoleA(g_console_output, g_output_buffer.c_str(), 
                  (DWORD)g_output_buffer.length(), &written, nullptr);
    
    g_output_buffer.clear();
}

void ShutdownTerminal()
{
    // Reset to default colors, show cursor, and exit alternate screen
    g_output_buffer.clear();
    g_output_buffer += "\033[0m";        // Reset all attributes
    g_output_buffer += "\033[?25h";      // Show cursor
    g_output_buffer += "\033[?1049l";    // Disable alternate screen buffer (restore original screen)
    
    // Write final reset sequences
    if (!g_output_buffer.empty())
    {
        DWORD written;
        WriteConsoleA(g_console_output, g_output_buffer.c_str(), 
                      (DWORD)g_output_buffer.length(), &written, nullptr);
    }
    
    // Reset console modes to default
    if (g_console_input != INVALID_HANDLE_VALUE)
    {
        DWORD input_mode;
        GetConsoleMode(g_console_input, &input_mode);
        input_mode |= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;
        input_mode &= ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
        SetConsoleMode(g_console_input, input_mode);
    }
}

#endif // _WIN32