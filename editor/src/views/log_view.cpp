//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "log_view.h"

void log_view::add_message(const std::string& message) {
    messages_.push_back(message);
    
    // Keep only the most recent messages
    while (messages_.size() > max_messages_) {
        messages_.erase(messages_.begin());
    }
}

void log_view::clear() {
    messages_.clear();
}

const std::vector<std::string>& log_view::get_messages() const {
    return messages_;
}

size_t log_view::message_count() const {
    return messages_.size();
}

void log_view::set_max_messages(size_t max_messages) {
    max_messages_ = max_messages;
    
    // Trim existing messages if needed
    while (messages_.size() > max_messages_) {
        messages_.erase(messages_.begin());
    }
}