//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "log_view_control.h"
#include <algorithm>

log_view_control::log_view_control(int x, int y, int width, int height)
    : x_(x), y_(y), width_(width), height_(height), scroll_offset_(0), max_messages_(1000) {
}

void log_view_control::add_message(const std::string& message) {
    messages_.push_back(message);
    
    // Keep only the most recent messages
    while (messages_.size() > max_messages_) {
        messages_.erase(messages_.begin());
    }
    
    // Auto-scroll to bottom when new message arrives
    scroll_to_bottom();
}

void log_view_control::clear() {
    messages_.clear();
    scroll_offset_ = 0;
}

void log_view_control::draw(WINDOW* win) {
    // Clear the log area
    for (int row = 0; row < height_; row++) {
        wmove(win, y_ + row, x_);
        for (int col = 0; col < width_; col++) {
            waddch(win, ' ');
        }
    }
    
    // Calculate which messages to show
    size_t total_messages = messages_.size();
    if (total_messages == 0) {
        return;
    }
    
    // Determine the range of messages to display
    size_t display_count = std::min(static_cast<size_t>(height_), total_messages);
    size_t start_idx = 0;
    
    if (total_messages > static_cast<size_t>(height_)) {
        start_idx = total_messages - display_count - scroll_offset_;
        if (scroll_offset_ > total_messages - display_count) {
            scroll_offset_ = total_messages - display_count;
            start_idx = 0;
        }
    }
    
    // Draw messages
    for (size_t i = 0; i < display_count && (start_idx + i) < total_messages; i++) {
        const std::string& message = messages_[start_idx + i];
        wmove(win, y_ + static_cast<int>(i), x_);
        
        // Truncate message if too long
        std::string display_msg = message;
        if (display_msg.length() > static_cast<size_t>(width_)) {
            display_msg = display_msg.substr(0, width_);
        }
        
        waddstr(win, display_msg.c_str());
    }
}

void log_view_control::scroll_up() {
    if (messages_.size() > static_cast<size_t>(height_)) {
        size_t max_scroll = messages_.size() - height_;
        if (scroll_offset_ < max_scroll) {
            scroll_offset_++;
        }
    }
}

void log_view_control::scroll_down() {
    if (scroll_offset_ > 0) {
        scroll_offset_--;
    }
}

void log_view_control::scroll_to_bottom() {
    scroll_offset_ = 0;
}