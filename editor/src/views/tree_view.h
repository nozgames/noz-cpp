//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "view_interface.h"
#include "../tui/tstring.h"

struct TreeNode
{
    TString formatted_content;  // Pre-formatted content with colors and visual length
    std::string raw_content;    // Raw text for searching
    std::string path; // Full path from root, e.g., "Game Systems/Player System/Movement Component"
    int indent_level;
    bool is_expanded;
    bool is_object; // true for objects (searchable), false for properties (display only)
    bool matches_search; // true if this node matches the current search
    bool is_search_parent; // true if this node is a parent of a matching node
    
    TreeNode* parent;
    std::vector<std::unique_ptr<TreeNode>> children;
    
    TreeNode(const std::string& text, int indent = 0, bool expanded = false, bool object = true)
        : formatted_content(text, text.length()) // Default: plain text with no formatting
        , raw_content(text)
        , indent_level(indent)
        , is_expanded(expanded)
        , is_object(object)
        , matches_search(false)
        , is_search_parent(false)
        , parent(nullptr)
    {}
    
    bool has_children() const { return !children.empty(); }
    
    TreeNode* AddChild(const std::string& text, bool object = true)
    {
        auto child = std::make_unique<TreeNode>(text, indent_level + 1, false, object);
        child->parent = this;
        child->path = path.empty() ? text : path + "/" + text;
        TreeNode* ptr = child.get();
        children.push_back(std::move(child));
        return ptr;
    }
};

class TreeView : public IView
{
protected:
    std::vector<std::unique_ptr<TreeNode>> _root_nodes;
    std::vector<TreeNode*> _visible_nodes;  // Pointers to currently visible nodes
    std::vector<TreeNode*> _node_stack;     // Stack for tracking current parent during construction
    size_t _max_entries = 1000;
    int _cursor_row = 0;     // Current cursor position in visible list
    bool _show_cursor = false;
    
    // Search functionality
    bool _search_active = false;
    std::string _search_pattern;
    std::regex _search_regex;
    bool _search_regex_valid = false;

    void RebuildVisibleList();
    void CollectVisibleNodes(TreeNode* node, std::vector<TreeNode*>& visible);
    void CollectSearchResults(TreeNode* node, std::vector<TreeNode*>& visible);
    void ToggleExpansion(TreeNode* node);
    bool MatchesSearch(TreeNode* node) const;
    void UpdateSearchFlags();
    void UpdateSearchFlagsRecursive(TreeNode* node);
    int CalculateNodeDistance(TreeNode* from, TreeNode* to) const;
    
public:
    void AddLine(const std::string& line); // Generic method (for backward compatibility)
    void AddObject(const std::string& name); // Add searchable object at current indent
    void AddProperty(const std::string& name, const std::string& value = ""); // Add non-searchable property
    void BeginObject(const std::string& name); // Add object and increase indent level
    void EndObject(); // Decrease indent level
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
    
    // IView search interface
    void SetSearchPattern(const std::string& pattern) override;
    void ClearSearch() override;
    bool SupportsSearch() const override;
};