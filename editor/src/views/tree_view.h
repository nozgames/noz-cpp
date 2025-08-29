//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once
#include "view_interface.h"
#include <vector>
#include <string>

struct TreeNode
{
    std::string content;
    int indent_level;
    bool is_expanded;
    bool has_children;
    size_t first_child_index;
    size_t child_count;
    
    TreeNode(const std::string& text, int indent = 0, bool expanded = false)
        : content(text)
        , indent_level(indent)
        , is_expanded(expanded)
        , has_children(false)
        , first_child_index(0)
        , child_count(0)
    {}
};

class TreeView : public IView
{
protected:
    std::vector<TreeNode> nodes_;
    std::vector<size_t> visible_indices_;  // Indices of currently visible nodes
    size_t max_entries_ = 1000;
    int cursor_row_ = 0;     // Current cursor position in visible list
    bool show_cursor_ = false;
    
    void RebuildVisibleList();
    void ToggleExpansion(size_t node_index);
    int CountVisibleChildren(size_t node_index) const;
    
public:
    void AddLine(const std::string& line);
    void Clear();
    size_t NodeCount() const;
    size_t VisibleCount() const;
    void SetMaxEntries(size_t max_entries);
    
    // IView interface
    void Render(int width, int height) override;
    bool HandleKey(int key) override;
    void SetCursorVisible(bool visible) override;
    bool CanPopFromStack() const override { return false; }
    
    // Navigation support
    void ScrollUp(int lines = 1);
    void ScrollDown(int lines = 1);
    void ScrollToTop();
    void ScrollToBottom();
    
    // Cursor support
    void SetCursorPosition(int row, int col);
    
    // Tree-specific operations
    void ExpandAll();
    void CollapseAll();
    void ExpandCurrent();
    void CollapseCurrent();
};