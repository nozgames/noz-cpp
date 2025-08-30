//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once
#include "tree_view.h"

class LogView : public IView
{
    std::unique_ptr<TreeView> _tree_view;

public:

    LogView();

    void Clear();
    void Render(const irect_t& rect) override;
    void Add(const std::string& message);
    size_t Count() const;
    void SetMax(size_t max_messages);
};
