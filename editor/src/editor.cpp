#ifdef _WIN32
#include <curses.h>
#else
#include <ncurses.h>
#endif

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "controls/log_view_control.h"
#include "controls/text_input.h"

int InitImporter();

class noz_editor {
private:
    std::unique_ptr<log_view_control> log_view_;
    std::unique_ptr<text_input> command_input_;
    bool command_mode_ = false;
    std::atomic<bool> running_ = true;
    WINDOW* main_window_;
    int screen_width_, screen_height_;
    
public:
    noz_editor() : main_window_(nullptr) {}
    
    ~noz_editor() {
        cleanup();
    }
    
    bool initialize() {
        // Initialize ncurses
        main_window_ = initscr();
        if (!main_window_) {
            return false;
        }
        
        // Get screen dimensions
        getmaxyx(stdscr, screen_height_, screen_width_);
        
        // Setup ncurses options
        cbreak();        // Disable line buffering
        noecho();        // Don't echo keys
        keypad(stdscr, TRUE);  // Enable function keys
        curs_set(0);     // Hide cursor initially
        
        // Initialize colors if available
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_BLACK, COLOR_WHITE);  // For status bar
            init_pair(2, COLOR_WHITE, COLOR_BLACK);  // For command line
        }
        
        // Create controls
        // Log view takes most of the screen, leaving 2 rows at bottom
        log_view_ = std::make_unique<log_view_control>(0, 0, screen_width_, screen_height_ - 2);
        
        // Command input on the bottom row
        command_input_ = std::make_unique<text_input>(1, screen_height_ - 1, screen_width_ - 1);
        
        return true;
    }
    
    void cleanup() {
        if (main_window_) {
            endwin();
            main_window_ = nullptr;
        }
    }
    
    void add_log_message(const std::string& message) {
        if (log_view_) {
            log_view_->add_message(message);
        }
    }
    
    void draw() {
        // Save current cursor position
        int cur_y, cur_x;
        getyx(stdscr, cur_y, cur_x);
        
        // Draw everything without refreshing
        erase(); // Clear screen buffer
        
        // Draw log view
        if (log_view_) {
            log_view_->draw(stdscr);
        }
        
        // Draw status bar (second to last row)
        if (has_colors()) {
            attron(COLOR_PAIR(1));
        }
        
        std::string status = "NoZ Editor";
        if (command_mode_) {
            status += " - Command Mode";
        }
        
        // Fill entire status bar width
        move(screen_height_ - 2, 0);
        for (int i = 0; i < screen_width_; i++) {
            char ch = (i < static_cast<int>(status.length())) ? status[i] : ' ';
            addch(ch);
        }
        
        if (has_colors()) {
            attroff(COLOR_PAIR(1));
        }
        
        // Draw command line (bottom row)
        if (has_colors()) {
            attron(COLOR_PAIR(2));
        }
        
        move(screen_height_ - 1, 0);
        if (command_mode_) {
            addch(':');
            
            // Draw command input text manually
            std::string input_text = command_input_->get_text();
            size_t cursor_pos = command_input_->get_cursor_pos();
            int available_width = screen_width_ - 1;
            
            // Handle scrolling for long text
            std::string display_text = input_text;
            size_t display_cursor_pos = cursor_pos;
            
            if (input_text.length() > static_cast<size_t>(available_width)) {
                if (cursor_pos >= static_cast<size_t>(available_width)) {
                    // Scroll text to keep cursor visible
                    size_t start = cursor_pos - available_width + 1;
                    display_text = input_text.substr(start);
                    display_cursor_pos = available_width - 1;
                } else {
                    display_text = input_text.substr(0, available_width);
                }
            }
            
            // Draw the text
            addstr(display_text.c_str());
            
            // Fill remaining space
            int remaining = available_width - static_cast<int>(display_text.length());
            for (int i = 0; i < remaining; i++) {
                addch(' ');
            }
            
            // Set final cursor position
            int final_cursor_x = 1 + static_cast<int>(display_cursor_pos);
            move(screen_height_ - 1, final_cursor_x);
            
        } else {
            // Clear command line
            for (int i = 0; i < screen_width_; i++) {
                addch(' ');
            }
            // Move cursor off screen when not in command mode
            move(0, 0);
        }
        
        if (has_colors()) {
            attroff(COLOR_PAIR(2));
        }
        
        // Only now refresh everything at once
        refresh();
    }
    
    void handle_command(const std::string& command) {
        if (command == "q" || command == "quit") {
            running_ = false;
        } else if (command == "import") {
            // Run importer in background thread
            std::thread([this]() {
                add_log_message("Starting asset importer...");
                int result = InitImporter();
                add_log_message("Importer finished with code: " + std::to_string(result));
            }).detach();
        } else if (command == "clear") {
            if (log_view_) {
                log_view_->clear();
            }
        } else if (!command.empty()) {
            add_log_message("Unknown command: " + command);
        }
    }
    
    void run() {
        if (!initialize()) {
            std::cerr << "Failed to initialize ncurses\n";
            return;
        }
        
        add_log_message("NoZ Editor starting...");
        add_log_message("Type ':import' to run asset importer");
        add_log_message("Type ':q' to quit");
        
        while (running_) {
            draw();
            
            int key = getch();
            
            if (command_mode_) {
                // In command mode
                if (key == '\n' || key == '\r') {
                    // Execute command
                    std::string command = command_input_->get_text();
                    handle_command(command);
                    command_mode_ = false;
                    command_input_->set_active(false);
                    command_input_->clear();
                    curs_set(0);  // Hide cursor when leaving command mode
                } else if (key == 27) { // Escape
                    command_mode_ = false;
                    command_input_->set_active(false);
                    command_input_->clear();
                    curs_set(0);  // Hide cursor when leaving command mode
                } else {
                    // Forward to command input
                    command_input_->handle_key(key);
                }
            } else {
                // Not in command mode
                if (key == ':') {
                    command_mode_ = true;
                    command_input_->set_active(true);
                    command_input_->clear();
                    curs_set(1);  // Show cursor in command mode
                } else if (key == 'q') {
                    // Quick quit with 'q' key
                    running_ = false;
                }
            }
        }
    }
};

int main(int argc, char* argv[])
{
    noz_editor editor;
    editor.run();
    return 0;
}
