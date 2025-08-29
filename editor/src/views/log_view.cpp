//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "log_view.h"
#include "../window.h"

void LogView::AddMessage(const std::string& message)
{
    messages_.push_back(message);

    // Keep only the most recent messages
    while (messages_.size() > max_messages_)
    {
        messages_.erase(messages_.begin());
    }
}

void LogView::Clear()
{
    messages_.clear();
}

void LogView::Render(Window* window, int width, int height)
{
    int log_height = height - 2; // Leave 2 rows for status and command

    for (int row = 0; row < log_height; row++)
    {
        window->MoveCursor(row, 0);
        for (int col = 0; col < width; col++)
        {
            window->AddChar(' ');
        }
    }

    // Calculate which messages to show (show most recent)
    if (!messages_.empty())
    {
        size_t total_messages = messages_.size();
        size_t display_count = std::min(static_cast<size_t>(log_height), total_messages);
        size_t start_idx = total_messages >= display_count ? total_messages - display_count : 0;

        for (size_t i = 0; i < display_count; i++)
        {
            const std::string& message = messages_[start_idx + i];
            window->MoveCursor(static_cast<int>(i), 0);

            std::string display_msg = message;
            if (display_msg.length() > static_cast<size_t>(width))
            {
                display_msg = display_msg.substr(0, width);
            }

            window->AddString(display_msg.c_str());
        }
    }
}

size_t LogView::MessageCount() const
{
    return messages_.size();
}

void LogView::SetMaxMessages(size_t max_messages)
{
    max_messages_ = max_messages;

    // Trim existing messages if needed
    while (messages_.size() > max_messages_)
    {
        messages_.erase(messages_.begin());
    }
}