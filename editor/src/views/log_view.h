//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once
#include "view_interface.h"

class LogView : public IView
{
    std::vector<std::string> messages_;
    size_t max_messages_ = 1000;
    int cursor_row_ = 0;     // Current cursor position in entire message list
    bool show_cursor_ = false;
    
public:

    void AddMessage(const std::string& message);
    void Clear();
    size_t MessageCount() const;
    void SetMaxMessages(size_t max_messages);
    
    // IView interface
    void Render(int width, int height) override;
    bool HandleKey(int key) override;
    void SetCursorVisible(bool visible) override;
    bool CanPopFromStack() const override { return false; }  // Log view cannot be popped
    
    // Additional scrolling support
    void ScrollUp(int lines = 1);
    void ScrollDown(int lines = 1);
    void ScrollToTop();
    void ScrollToBottom();
    
    // Cursor support
    void SetCursorPosition(int row, int col);
};
