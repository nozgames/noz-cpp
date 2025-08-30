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
    bool matches_search; // true if this node matches the current search
    bool is_search_parent; // true if this node is a parent of a matching node
    
    TreeNode* parent;
    std::vector<std::unique_ptr<TreeNode>> children;
    void* user_data = nullptr;  // User data pointer for attaching custom data
    
    TreeNode(const std::string& text, int indent = 0, bool expanded = false)
        : formatted_content(text, text.length()) // Default: plain text with no formatting
        , raw_content(text)
        , indent_level(indent)
        , is_expanded(expanded)
        , matches_search(false)
        , is_search_parent(false)
        , parent(nullptr)
    {}
    
    bool has_children() const { return !children.empty(); }
    bool has_user_data() const { return user_data != nullptr; }
    
    TreeNode* AddChild(const std::string& text)
    {
        auto child = std::make_unique<TreeNode>(text, indent_level + 1, false);
        child->parent = this;
        child->path = path.empty() ? text : path + "/" + text;
        TreeNode* ptr = child.get();
        children.push_back(std::move(child));
        return ptr;
    }
    
    void SetUserData(void* data)
    {
        user_data = data;
    }
    
    void* GetUserData() const { return user_data; }
};

class TreeView : public IView
{
protected:
    std::vector<std::unique_ptr<TreeNode>> _root_nodes;
    std::vector<TreeNode*> _visible_nodes;  // Pointers to currently visible nodes
    size_t _max_entries = 1000;
    int _cursor_row = 0;     // Current cursor position in visible list
    int _previous_cursor_row = -1; // Track cursor changes
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
    void Add(const std::string& name, int indent_level = 0, void* user_data = nullptr); // Main API
    
    // Legacy methods (for backward compatibility)
    void AddLine(const std::string& line); 
    void AddLine(const std::string& line, void* user_data);
    void AddObject(const std::string& name);
    void AddObject(const std::string& name, void* user_data);
    void Clear();
    size_t NodeCount() const;
    size_t VisibleCount() const;
    void SetMaxEntries(size_t max_entries);
    
    // User data management for nodes
    void SetCurrentNodeUserData(void* data);
    void SetNodeUserData(const std::string& node_path, void* data);
    
    // Cursor and selection
    TreeNode* GetCurrentNode() const;
    bool HasCursorChanged() const;
    void MarkCursorProcessed();
    
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