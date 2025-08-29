//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "text_input.h"
#include "terminal.h"
#include <cassert>
#include <noz/log.h>

struct TextInputImpl : TextInput
{
    std::string buffer;
    size_t cursor_pos = 0;
    int x, y;
    int width;
    bool active = false;
    
    TextInputImpl(int x_, int y_, int width_) 
        : x(x_), y(y_), width(width_) {}
};

TextInput* CreateTextInput(int x, int y, int width)
{
    return new TextInputImpl(x, y, width);
}

void Destroy(TextInput* input)
{
    if (input)
    {
        TextInputImpl* impl = static_cast<TextInputImpl*>(input);
        delete impl;
    }
}

void Draw(TextInput* input)
{
    assert(input);
    TextInputImpl* impl = static_cast<TextInputImpl*>(input);
    
    if (impl->active) {
        // Render text with cursor at correct position
        for (size_t i = 0; i <= impl->buffer.length(); ++i) {
            if (i == impl->cursor_pos) {
                // Show cursor at this position
                if (i < impl->buffer.length()) {
                    // Cursor is over a character - show character with inverse video
                    AddString("\033[7m");  // Enable reverse video (inverse)
                    AddChar(impl->buffer[i]);
                    AddString("\033[27m"); // Disable reverse video
                } else {
                    // Cursor is after the last character - show cursor block
                    AddChar('\xDB');  // Solid block character (█)
                }
            } else if (i < impl->buffer.length()) {
                // Regular character
                AddChar(impl->buffer[i]);
            }
        }
    } else {
        // Just render the text without cursor when inactive
        AddString(impl->buffer.c_str());
    }
}

void SetActive(TextInput* input, bool active)
{
    assert(input);
    auto impl = static_cast<TextInputImpl*>(input);
    impl->active = active;
}

bool HandleKey(TextInput* input, int key)
{
    assert(input);
    auto* impl = static_cast<TextInputImpl*>(input);

    if (!impl->active)
        return false;
    
    switch (key) {
        case 127: // Delete/Backspace
        case 8:
            if (impl->cursor_pos > 0) {
                impl->buffer.erase(impl->cursor_pos - 1, 1);
                impl->cursor_pos--;
            }
            return true;
            
        case KEY_LEFT:
            if (impl->cursor_pos > 0) {
                impl->cursor_pos--;
            }
            return true;
            
        case KEY_RIGHT:
            if (impl->cursor_pos < impl->buffer.length()) {
                impl->cursor_pos++;
            }
            return true;
            
        case KEY_HOME:
            impl->cursor_pos = 0;
            return true;
            
        case KEY_END:
            impl->cursor_pos = impl->buffer.length();
            return true;
            
        default:
            // Handle regular characters
            if (key >= 32 && key <= 126) { // Printable ASCII
                // Ensure cursor_pos is valid
                if (impl->cursor_pos > impl->buffer.length())
                    impl->cursor_pos = impl->buffer.length();
                    
                impl->buffer.insert(impl->cursor_pos, 1, static_cast<char>(key));
                impl->cursor_pos++;
                return true;
            }
            break;
    }
    
    return false;
}

void Clear(TextInput* input)
{
    assert(input);
    TextInputImpl* impl = static_cast<TextInputImpl*>(input);
    impl->buffer.clear();
    impl->cursor_pos = 0;
}

const std::string& GetText(TextInput* input)
{
    assert(input);
    TextInputImpl* impl = static_cast<TextInputImpl*>(input);
    return impl->buffer;
}

void SetText(TextInput* input, const std::string& text)
{
    assert(input);
    TextInputImpl* impl = static_cast<TextInputImpl*>(input);
    impl->buffer = text;
    impl->cursor_pos = impl->buffer.length();
}

size_t GetCursorPos(TextInput* input)
{
    assert(input);
    TextInputImpl* impl = static_cast<TextInputImpl*>(input);
    return impl->cursor_pos;
}

bool IsActive(TextInput* input)
{
    assert(input);
    TextInputImpl* impl = static_cast<TextInputImpl*>(input);
    return impl->active;
}