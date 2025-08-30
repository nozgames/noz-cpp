//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "log_view.h"

LogView::LogView()
{
    _tree_view = std::make_unique<TreeView>();
}

void LogView::Clear()
{
    _tree_view->Clear();
}

void LogView::Add(const std::string& message)
{
    _tree_view->Add(TStringBuilder().Add(message).ToString(), 0, nullptr);
}

size_t LogView::Count() const
{
    return _tree_view->NodeCount();
}

void LogView::SetMax(size_t max_messages)
{
    _tree_view->SetMaxEntries(max_messages);
}

void LogView::Render(const irect_t& rect)
{
    _tree_view->Render(rect);
}