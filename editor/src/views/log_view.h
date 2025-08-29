//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <string>
#include <vector>

// Forward declaration
class Window;

class LogView {
private:
    std::vector<std::string> messages_;
    size_t max_messages_ = 1000;
    
public:
    void AddMessage(const std::string& message);
    void Clear();
    void Render(Window* window, int width, int height);
    size_t MessageCount() const;
    void SetMaxMessages(size_t max_messages);
};