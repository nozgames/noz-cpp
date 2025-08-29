//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once
#include "tree_view.h"

class LogView : public TreeView
{
public:
    void AddMessage(const std::string& message);
    size_t MessageCount() const;
    void SetMaxMessages(size_t max_messages);
};
