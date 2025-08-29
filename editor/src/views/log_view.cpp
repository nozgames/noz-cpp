//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "log_view.h"
#include "../tui/terminal.h"
#include <algorithm>

// Utility function to add a string with optional cursor highlighting
// cursor_pos: -1 for no cursor, >= 0 for cursor at that visible character position
static void AddStringWithCursor(const std::string& str, int cursor_pos)
{
    if (cursor_pos < 0)
    {
        // No cursor, just add the string normally
        AddString(str.c_str());
        return;
    }
    
    size_t pos = 0;
    int visible_char_count = 0;
    std::string before_cursor, cursor_char, after_cursor;
    bool cursor_found = false;
    
    while (pos < str.length())
    {
        if (str[pos] == '\033')
        {
            // ANSI escape sequence - skip to end (find 'm')
            size_t escape_start = pos;
            while (pos < str.length() && str[pos] != 'm')
                pos++;
            if (pos < str.length())
                pos++; // Include the 'm'
            
            // Add escape sequence to appropriate part
            std::string escape_seq = str.substr(escape_start, pos - escape_start);
            if (!cursor_found)
            {
                before_cursor += escape_seq;
            }
            else
            {
                after_cursor += escape_seq;
            }
        }
        else
        {
            // Regular visible character
            if (visible_char_count == cursor_pos && !cursor_found)
            {
                // This is where the cursor should be
                cursor_char = str[pos];
                cursor_found = true;
                pos++;
            }
            else if (!cursor_found)
            {
                before_cursor += str[pos];
                pos++;
            }
            else
            {
                after_cursor += str.substr(pos);
                break;
            }
            visible_char_count++;
        }
    }
    
    // Render the parts
    AddString(before_cursor.c_str());
    
    if (cursor_found)
    {
        AddString("\033[7m");  // Enable reverse video
        AddChar(cursor_char[0]);
        AddString("\033[27m"); // Disable reverse video
        AddString(after_cursor.c_str());
    }
    else if (cursor_pos >= visible_char_count)
    {
        // Cursor position is beyond the string - show cursor at end
        AddString("\033[7m");  // Enable reverse video
        AddChar(' ');
        AddString("\033[27m"); // Disable reverse video
    }
}

void LogView::AddMessage(const std::string& message)
{
    messages_.push_back(message);

    // Keep only the most recent messages
    while (messages_.size() > max_messages_)
    {
        messages_.erase(messages_.begin());
    }
    
    // Auto-scroll to bottom: keep cursor at the newest (last) message
    if (!messages_.empty())
    {
        cursor_row_ = static_cast<int>(messages_.size()) - 1;  // Move cursor to newest message
    }
}

void LogView::Clear()
{
    messages_.clear();
    cursor_row_ = 0;
}

void LogView::Render(int width, int height)
{
    int log_height = height - 2; // Leave 2 rows for status and command

    // Clear the log area
    for (int row = 0; row < log_height; row++)
    {
        MoveCursor(row, 0);
        for (int col = 0; col < width; col++)
            AddChar(' ');
    }

    // Calculate which messages to show based on cursor position
    if (!messages_.empty())
    {
        size_t total_messages = messages_.size();
        size_t max_display_count = std::min(static_cast<size_t>(log_height), total_messages);
        
        // Ensure cursor is within valid message range
        cursor_row_ = std::max(0, std::min(cursor_row_, static_cast<int>(total_messages) - 1));
        
        // Calculate window to show cursor
        size_t start_idx = 0;
        if (total_messages > max_display_count)
        {
            // We have more messages than can fit on screen
            // Position window to show cursor
            int cursor_message_idx = cursor_row_;
            
            // Try to center cursor in the view, but adjust if at edges
            int ideal_start = cursor_message_idx - static_cast<int>(max_display_count) / 2;
            int max_start = static_cast<int>(total_messages) - static_cast<int>(max_display_count);
            
            start_idx = std::max(0, std::min(ideal_start, max_start));
        }
        
        size_t display_count = std::min(max_display_count, total_messages - start_idx);
        
        // Calculate cursor position within the visible window
        int cursor_in_window = cursor_row_ - static_cast<int>(start_idx);

        for (size_t i = 0; i < display_count; i++)
        {
            const std::string& message = messages_[start_idx + i];
            MoveCursor(static_cast<int>(i), 0);

            std::string display_msg = message;
            if (display_msg.length() > static_cast<size_t>(width))
            {
                display_msg = display_msg.substr(0, width);
            }

            // Render the message with optional cursor highlighting
            int cursor_pos = (show_cursor_ && static_cast<int>(i) == cursor_in_window) ? 0 : -1;
            AddStringWithCursor(display_msg, cursor_pos);
        }
    }
}

size_t LogView::MessageCount() const
{
    return messages_.size();
}

void LogView::SetMaxMessages(size_t max_messages)
{
    max_messages_ = max_messages;

    // Trim existing messages if needed
    while (messages_.size() > max_messages_)
    {
        messages_.erase(messages_.begin());
    }
}

bool LogView::HandleKey(int key)
{
    switch (key)
    {
        case KEY_UP:
            // Move cursor up through the entire message list
            if (cursor_row_ > 0)
            {
                cursor_row_--;
            }
            return true;
            
        case KEY_DOWN:
            // Move cursor down through the entire message list
            if (!messages_.empty() && cursor_row_ < static_cast<int>(messages_.size()) - 1)
            {
                cursor_row_++;
            }
            return true;
            
        case KEY_PPAGE:  // Page Up
            // Move cursor up by a page (10 lines)
            cursor_row_ = std::max(0, cursor_row_ - 10);
            return true;
            
        case KEY_NPAGE:  // Page Down
            // Move cursor down by a page (10 lines)
            if (!messages_.empty())
            {
                cursor_row_ = std::min(cursor_row_ + 10, static_cast<int>(messages_.size()) - 1);
            }
            return true;
            
        case KEY_HOME:
            // Go to first message
            cursor_row_ = 0;
            return true;
            
        case KEY_END:
            // Go to last message
            if (!messages_.empty())
            {
                cursor_row_ = static_cast<int>(messages_.size()) - 1;
            }
            return true;
            
        default:
            return false;  // Key not handled
    }
}

void LogView::ScrollUp(int lines)
{
    // Legacy method - now just moves cursor up
    cursor_row_ = std::max(0, cursor_row_ - lines);
}

void LogView::ScrollDown(int lines)
{
    // Legacy method - now just moves cursor down
    if (!messages_.empty())
    {
        cursor_row_ = std::min(cursor_row_ + lines, static_cast<int>(messages_.size()) - 1);
    }
}

void LogView::ScrollToTop()
{
    // Legacy method - now just moves cursor to top
    cursor_row_ = 0;
}

void LogView::ScrollToBottom()
{
    // Legacy method - now just moves cursor to bottom
    if (!messages_.empty())
    {
        cursor_row_ = static_cast<int>(messages_.size()) - 1;
    }
}

void LogView::SetCursorVisible(bool visible)
{
    show_cursor_ = visible;
}

void LogView::SetCursorPosition(int row, int col)
{
    cursor_row_ = row;
}