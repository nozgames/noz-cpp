//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

class LogView
{
    std::vector<std::string> messages_;
    size_t max_messages_ = 1000;
    
public:

    void AddMessage(const std::string& message);
    void Clear();
    void Render(int width, int height);
    size_t MessageCount() const;
    void SetMaxMessages(size_t max_messages);
};
