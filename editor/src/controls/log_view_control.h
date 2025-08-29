//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#ifdef _WIN32
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include <vector>
#include <string>

class log_view_control {
private:
    std::vector<std::string> messages_;
    int x_, y_, width_, height_;
    size_t scroll_offset_;
    size_t max_messages_;
    
public:
    log_view_control(int x, int y, int width, int height);
    
    void add_message(const std::string& message);
    void clear();
    void draw(WINDOW* win);
    
    void scroll_up();
    void scroll_down();
    void scroll_to_bottom();
    
    size_t message_count() const { return messages_.size(); }
};