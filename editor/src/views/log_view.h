//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <string>
#include <vector>

class log_view {
private:
    std::vector<std::string> messages_;
    size_t max_messages_ = 1000;
    
public:
    void add_message(const std::string& message);
    void clear();
    const std::vector<std::string>& get_messages() const;
    size_t message_count() const;
    
    // For future FTXUI integration
    void set_max_messages(size_t max_messages);
};