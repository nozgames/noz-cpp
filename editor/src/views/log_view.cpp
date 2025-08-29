//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "log_view.h"

void LogView::AddMessage(const std::string& message)
{
    // Use the base TreeView's AddLine method, which handles tab-based nesting
    AddLine(message);
}

size_t LogView::MessageCount() const
{
    return NodeCount();
}

void LogView::SetMaxMessages(size_t max_messages)
{
    SetMaxEntries(max_messages);
}