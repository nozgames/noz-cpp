//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "text_input.h"

text_input::text_input(int x, int y, int width) 
    : cursor_pos_(0), x_(x), y_(y), width_(width), active_(false) {
}

void text_input::draw(WINDOW* win) {
    // Calculate display parameters
    std::string display_text = buffer_;
    size_t text_start = 0;
    size_t display_cursor_pos = cursor_pos_;
    
    // Handle text scrolling if it's too long
    if (display_text.length() > static_cast<size_t>(width_)) {
        if (cursor_pos_ >= static_cast<size_t>(width_)) {
            // Scroll text to keep cursor visible
            text_start = cursor_pos_ - width_ + 1;
            display_cursor_pos = width_ - 1;
        }
        display_text = display_text.substr(text_start, width_);
    }
    
    // Clear the input area and draw text using mvwaddnstr to avoid cursor movement
    for (int i = 0; i < width_; i++) {
        mvwaddch(win, y_, x_ + i, ' ');
    }
    
    // Draw the visible text without moving cursor
    if (!display_text.empty()) {
        mvwaddnstr(win, y_, x_, display_text.c_str(), width_);
    }
    
    // Position cursor correctly only at the very end
    if (active_) {
        int final_cursor_x = x_ + static_cast<int>(display_cursor_pos);
        wmove(win, y_, final_cursor_x);
    }
}

void text_input::set_active(bool active) {
    active_ = active;
}

bool text_input::handle_key(int key) {
    if (!active_) {
        return false;
    }
    
    switch (key) {
        case KEY_BACKSPACE:
        case 127: // Delete/Backspace
        case 8:
            if (cursor_pos_ > 0) {
                buffer_.erase(cursor_pos_ - 1, 1);
                cursor_pos_--;
            }
            return true;
            
        case KEY_LEFT:
            if (cursor_pos_ > 0) {
                cursor_pos_--;
            }
            return true;
            
        case KEY_RIGHT:
            if (cursor_pos_ < buffer_.length()) {
                cursor_pos_++;
            }
            return true;
            
        case KEY_HOME:
            cursor_pos_ = 0;
            return true;
            
        case KEY_END:
            cursor_pos_ = buffer_.length();
            return true;
            
        case KEY_DC: // Delete key
            if (cursor_pos_ < buffer_.length()) {
                buffer_.erase(cursor_pos_, 1);
            }
            return true;
            
        default:
            // Handle regular characters
            if (key >= 32 && key <= 126) { // Printable ASCII
                buffer_.insert(cursor_pos_, 1, static_cast<char>(key));
                cursor_pos_++;
                return true;
            }
            break;
    }
    
    return false;
}

void text_input::clear() {
    buffer_.clear();
    cursor_pos_ = 0;
}

const std::string& text_input::get_text() const {
    return buffer_;
}

void text_input::set_text(const std::string& text) {
    buffer_ = text;
    cursor_pos_ = buffer_.length();
}