//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#ifdef _WIN32
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include <string>

class text_input {
private:
    std::string buffer_;
    size_t cursor_pos_;
    int x_, y_;
    int width_;
    bool active_;
    
public:
    text_input(int x, int y, int width);
    
    void draw(WINDOW* win);
    void set_active(bool active);
    bool handle_key(int key);
    void clear();
    
    const std::string& get_text() const;
    void set_text(const std::string& text);
    size_t get_cursor_pos() const { return cursor_pos_; }
    
    bool is_active() const { return active_; }
};