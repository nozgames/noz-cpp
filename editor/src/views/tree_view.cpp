//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "tree_view.h"
#include "../tui/terminal.h"
#include "../tokenizer.h"
#include <algorithm>
#include <string>

static int CountLeadingTabs(const std::string& line)
{
    int count = 0;
    for (char c : line)
    {
        if (c == '\t')
            count++;
        else
            break;
    }
    return count;
}

static std::string RemoveLeadingTabs(const std::string& line)
{
    size_t first_non_tab = 0;
    while (first_non_tab < line.length() && line[first_non_tab] == '\t')
        first_non_tab++;
    return line.substr(first_non_tab);
}

static void FormatValue(TStringBuilder& builder, const std::string& value)
{
    if (value.empty())
    {
        builder.Add(value);
        return;
    }
    
    Tokenizer tok;
    Token token;
    
    // Check for color patterns using tokenizer
    color_t color_result;
    Init(tok, value.c_str());
    if (ExpectColor(tok, &token, &color_result))
    {
        builder.Add(color_result);
        return;
    }
    
    // Check for vector patterns using tokenizer: (x,y), (x,y,z), or (x,y,z,w)
    if (value.size() >= 5 && value.front() == '(' && value.back() == ')')
    {
        vec2 vec2_result;
        vec3 vec3_result;
        vec4 vec4_result;
        
        // Try parsing as vec2 first
        Init(tok, value.c_str());
        if (ExpectVec2(tok, &token, &vec2_result))
        {
            builder.Add(vec2_result);
            return;
        }
        
        // Reset tokenizer and try vec3
        Init(tok, value.c_str());
        if (ExpectVec3(tok, &token, &vec3_result))
        {
            builder.Add(vec3_result);
            return;
        }
        
        // Reset tokenizer and try vec4
        Init(tok, value.c_str());
        if (ExpectVec4(tok, &token, &vec4_result))
        {
            builder.Add(vec4_result);
            return;
        }
    }
    
    // Check for boolean values
    if (value == "true" || value == "false")
    {
        builder.Add(value == "true");
        return;
    }
    
    // Check for number (integer or float)
    static const std::regex number_regex("^[-+]?([0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)$");
    if (std::regex_match(value, number_regex))
    {
        // Try parsing as int first, then float
        char* end;
        long int_val = strtol(value.c_str(), &end, 10);
        if (*end == '\0')
            builder.Add(static_cast<int>(int_val));
        else
            builder.Add(static_cast<float>(strtof(value.c_str(), nullptr)));
        return;
    }
    
    // Everything else is treated as a string
    if (!value.empty() && value.front() == '"' && value.back() == '"')
        builder.Add(value, TCOLOR_GREEN); // Already has quotes
    else
        builder.Add("\"" + value + "\"", TCOLOR_GREEN); // Add quotes
}


static std::string GetArraySizeIndicator(const TreeNode* node)
{
    if (node && node->has_children())
    {
        size_t child_count = node->children.size();
        return "\033[97m[" + std::to_string(child_count) + "]\033[0m"; // Bright white
    }
    return "";
}

void TreeView::AddLine(const std::string& line)
{
    int indent_level = CountLeadingTabs(line);
    std::string content = RemoveLeadingTabs(line);
    
    // Adjust stack to match indent level
    while (_node_stack.size() > static_cast<size_t>(indent_level))
    {
        _node_stack.pop_back();
    }
    
    TreeNode* new_node = nullptr;
    if (_node_stack.empty())
    {
        // Root node
        auto root = std::make_unique<TreeNode>(content, 0, true, true);
        root->path = content;
        new_node = root.get();
        _root_nodes.push_back(std::move(root));
    }
    else
    {
        // Child node
        TreeNode* parent = _node_stack.back();
        new_node = parent->AddChild(content, true);
    }
    
    _node_stack.push_back(new_node);
    RebuildVisibleList();
    
    // Auto-scroll to bottom
    if (!_visible_nodes.empty())
    {
        _cursor_row = static_cast<int>(_visible_nodes.size()) - 1;
    }
}

void TreeView::AddObject(const std::string& name)
{
    TreeNode* new_node = nullptr;
    if (_node_stack.empty())
    {
        // Root node
        auto root = std::make_unique<TreeNode>(name, 0, false, true);
        root->path = name;
        new_node = root.get();
        _root_nodes.push_back(std::move(root));
    }
    else
    {
        // Child node
        TreeNode* parent = _node_stack.back();
        new_node = parent->AddChild(name, true);
    }
    
    RebuildVisibleList();
}

void TreeView::AddProperty(const std::string& name, const std::string& value)
{
    std::string raw_content = name;
    TString formatted_content;
    
    if (!value.empty())
    {
        raw_content += ": " + value;
        
        // Format the content immediately with proper colors
        auto builder = TStringBuilder::Build();
        builder.Add(name + ": ");
        FormatValue(builder, value);
        formatted_content = builder.ToString();
    }
    else
    {
        // No value, just plain name
        formatted_content = TString(name, name.length());
    }
    
    TreeNode* new_node = nullptr;
    if (!_node_stack.empty())
    {
        TreeNode* parent = _node_stack.back();
        new_node = parent->AddChild(raw_content, false);
    }
    
    // Update the node with formatted content
    if (new_node)
    {
        new_node->formatted_content = formatted_content;
        new_node->raw_content = raw_content;
    }
    
    RebuildVisibleList();
}

void TreeView::BeginObject(const std::string& name)
{
    TreeNode* new_node = nullptr;
    if (_node_stack.empty())
    {
        // Root node
        auto root = std::make_unique<TreeNode>(name, 0, false, true);
        root->path = name;
        new_node = root.get();
        _root_nodes.push_back(std::move(root));
    }
    else
    {
        // Child node
        TreeNode* parent = _node_stack.back();
        new_node = parent->AddChild(name, true);
    }
    
    _node_stack.push_back(new_node);
    RebuildVisibleList();
}

void TreeView::EndObject()
{
    if (!_node_stack.empty())
    {
        _node_stack.pop_back();
    }
}

void TreeView::RebuildVisibleList()
{
    // Remember current cursor node if any
    TreeNode* current_cursor_node = nullptr;
    if (!_visible_nodes.empty() && _cursor_row >= 0 && _cursor_row < static_cast<int>(_visible_nodes.size()))
    {
        current_cursor_node = _visible_nodes[_cursor_row];
    }
    
    _visible_nodes.clear();
    
    for (auto& root : _root_nodes)
    {
        if (_search_active && _search_regex_valid)
        {
            CollectSearchResults(root.get(), _visible_nodes);
        }
        else
        {
            CollectVisibleNodes(root.get(), _visible_nodes);
        }
    }
    
    // Try to restore cursor position to the same node, or closest available
    if (current_cursor_node && !_visible_nodes.empty())
    {
        // First try to find the exact same node
        for (size_t i = 0; i < _visible_nodes.size(); i++)
        {
            if (_visible_nodes[i] == current_cursor_node)
            {
                _cursor_row = static_cast<int>(i);
                return;
            }
        }
        
        // If exact node not found, try to find a close ancestor or descendant
        int best_distance = INT_MAX;
        int best_position = 0;
        
        for (size_t i = 0; i < _visible_nodes.size(); i++)
        {
            TreeNode* node = _visible_nodes[i];
            int distance = CalculateNodeDistance(current_cursor_node, node);
            if (distance < best_distance)
            {
                best_distance = distance;
                best_position = static_cast<int>(i);
            }
        }
        
        _cursor_row = best_position;
    }
    else
    {
        // No previous cursor or empty list - reset to top
        _cursor_row = 0;
    }
    
    // Ensure cursor is in valid range
    if (!_visible_nodes.empty())
    {
        _cursor_row = std::max(0, std::min(_cursor_row, static_cast<int>(_visible_nodes.size()) - 1));
    }
}

void TreeView::CollectVisibleNodes(TreeNode* node, std::vector<TreeNode*>& visible)
{
    if (!node) return;
    
    visible.push_back(node);
    
    if (node->is_expanded && node->has_children())
    {
        for (auto& child : node->children)
        {
            CollectVisibleNodes(child.get(), visible);
        }
    }
}

void TreeView::CollectSearchResults(TreeNode* node, std::vector<TreeNode*>& visible)
{
    if (!node) return;
    
    if (node->matches_search)
    {
        // Add this matching node
        visible.push_back(node);
        
        // Add children only if this node is expanded (respecting expansion state even in search)
        if (node->is_expanded && node->has_children())
        {
            for (auto& child : node->children)
            {
                visible.push_back(child.get());
                
                // Recursively add children of expanded nodes
                std::function<void(TreeNode*)> AddChildrenOfExpanded = [&](TreeNode* n) {
                    if (n->is_expanded && n->has_children())
                    {
                        for (auto& grandchild : n->children)
                        {
                            visible.push_back(grandchild.get());
                            AddChildrenOfExpanded(grandchild.get());
                        }
                    }
                };
                AddChildrenOfExpanded(child.get());
            }
        }
    }
    else if (node->is_search_parent)
    {
        // Add this parent node (will be rendered in darker color)
        visible.push_back(node);
        
        // Only add children that are either matches or search parents
        for (auto& child : node->children)
        {
            CollectSearchResults(child.get(), visible);
        }
    }
}

void TreeView::Clear()
{
    _root_nodes.clear();
    _visible_nodes.clear();
    _node_stack.clear();
    _cursor_row = 0;
}

void TreeView::Render(int width, int height)
{
    int tree_height = height - 2; // Leave 2 rows for status and command

    // Clear the tree area
    for (int row = 0; row < tree_height; row++)
    {
        MoveCursor(row, 0);
        for (int col = 0; col < width; col++)
            AddChar(' ');
    }

    // Calculate which nodes to show based on cursor position
    if (!_visible_nodes.empty())
    {
        size_t total_visible = _visible_nodes.size();
        size_t max_display_count = std::min(static_cast<size_t>(tree_height), total_visible);
        
        // Ensure cursor is within valid range
        _cursor_row = std::max(0, std::min(_cursor_row, static_cast<int>(total_visible) - 1));
        
        // Calculate window to show cursor
        size_t start_idx = 0;
        if (total_visible > max_display_count)
        {
            int cursor_pos = _cursor_row;
            int ideal_start = cursor_pos - static_cast<int>(max_display_count) / 2;
            int max_start = static_cast<int>(total_visible) - static_cast<int>(max_display_count);
            
            start_idx = std::max(0, std::min(ideal_start, max_start));
        }
        
        size_t display_count = std::min(max_display_count, total_visible - start_idx);
        int cursor_in_window = _cursor_row - static_cast<int>(start_idx);

        for (size_t i = 0; i < display_count; i++)
        {
            TreeNode* node = _visible_nodes[start_idx + i];
            
            MoveCursor(static_cast<int>(i), 0);

            // Build display line using TStringBuilder
            auto line_builder = TStringBuilder::Build();
            
            // Add indentation
            for (int indent = 0; indent < node->indent_level; indent++)
            {
                line_builder.Add("  "); // 2 spaces per indent level
            }
            
            // Add expansion indicator for nodes with children
            if (node->has_children())
            {
                if (node->is_expanded)
                    line_builder.Add("- ");
                else
                    line_builder.Add("+ ");
            }
            else if (node->indent_level > 0)
            {
                line_builder.Add("  ");
            }
            
            // Add content
            if (_search_active && node->is_search_parent && !node->matches_search)
            {
                // Search parent nodes in darker/subdued color
                line_builder.Add(node->raw_content, 128, 128, 128);
            }
            else
            {
                line_builder.Add(node->formatted_content);
            }
            
            // Add array size indicator for objects with children
            std::string size_indicator = GetArraySizeIndicator(node);
            if (!size_indicator.empty())
            {
                line_builder.Add(" " + size_indicator);
            }
            
            // Truncate if needed and build final TString
            line_builder.TruncateToWidth(width);
            TString display_line = line_builder.ToString();

            // Render with optional cursor highlighting
            int cursor_pos = (_show_cursor && static_cast<int>(i) == cursor_in_window) ? 0 : -1;
            AddStringWithCursor(display_line, cursor_pos);
        }
    }
}

size_t TreeView::NodeCount() const
{
    size_t count = 0;
    std::function<void(TreeNode*)> count_nodes = [&](TreeNode* node) {
        if (!node) return;
        count++;
        for (auto& child : node->children)
        {
            count_nodes(child.get());
        }
    };
    
    for (auto& root : _root_nodes)
    {
        count_nodes(root.get());
    }
    
    return count;
}

size_t TreeView::VisibleCount() const
{
    return _visible_nodes.size();
}

void TreeView::SetMaxEntries(size_t max_entries)
{
    _max_entries = max_entries;
    RebuildVisibleList();
}

bool TreeView::HandleKey(int key)
{
    switch (key)
    {
        case KEY_UP:
            if (_cursor_row > 0)
            {
                _cursor_row--;
            }
            return true;
            
        case KEY_DOWN:
            if (!_visible_nodes.empty() && _cursor_row < static_cast<int>(_visible_nodes.size()) - 1)
            {
                _cursor_row++;
            }
            return true;
            
        case KEY_PPAGE:  // Page Up
            _cursor_row = std::max(0, _cursor_row - 10);
            return true;
            
        case KEY_NPAGE:  // Page Down
            if (!_visible_nodes.empty())
            {
                _cursor_row = std::min(_cursor_row + 10, static_cast<int>(_visible_nodes.size()) - 1);
            }
            return true;
            
        case KEY_HOME:
            _cursor_row = 0;
            return true;
            
        case KEY_END:
            if (!_visible_nodes.empty())
            {
                _cursor_row = static_cast<int>(_visible_nodes.size()) - 1;
            }
            return true;
            
        case KEY_RIGHT:
        case ' ':  // Space also expands
            // Expand current node
            if (!_visible_nodes.empty() && _cursor_row >= 0 && _cursor_row < static_cast<int>(_visible_nodes.size()))
            {
                TreeNode* node = _visible_nodes[_cursor_row];
                ToggleExpansion(node);
            }
            return true;
            
        case KEY_LEFT:
            // Collapse current node or move to parent
            if (!_visible_nodes.empty() && _cursor_row >= 0 && _cursor_row < static_cast<int>(_visible_nodes.size()))
            {
                TreeNode* node = _visible_nodes[_cursor_row];
                
                if (node->has_children() && node->is_expanded)
                {
                    // Collapse current node
                    ToggleExpansion(node);
                }
                else if (node->parent)
                {
                    // Move to parent
                    auto it = std::find(_visible_nodes.begin(), _visible_nodes.end(), node->parent);
                    if (it != _visible_nodes.end())
                    {
                        _cursor_row = static_cast<int>(std::distance(_visible_nodes.begin(), it));
                    }
                }
            }
            return true;
            
        default:
            return false;
    }
}

void TreeView::ScrollUp(int lines)
{
    _cursor_row = std::max(0, _cursor_row - lines);
}

void TreeView::ScrollDown(int lines)
{
    if (!_visible_nodes.empty())
    {
        _cursor_row = std::min(_cursor_row + lines, static_cast<int>(_visible_nodes.size()) - 1);
    }
}

void TreeView::ScrollToTop()
{
    _cursor_row = 0;
}

void TreeView::ScrollToBottom()
{
    if (!_visible_nodes.empty())
    {
        _cursor_row = static_cast<int>(_visible_nodes.size()) - 1;
    }
}

void TreeView::SetCursorVisible(bool visible)
{
    _show_cursor = visible;
}

void TreeView::SetCursorPosition(int row, int col)
{
    _cursor_row = row;
}

void TreeView::ExpandAll()
{
    std::function<void(TreeNode*)> expand_recursive = [&](TreeNode* node) {
        if (!node) return;
        if (node->has_children())
        {
            node->is_expanded = true;
        }
        for (auto& child : node->children)
        {
            expand_recursive(child.get());
        }
    };
    
    for (auto& root : _root_nodes)
    {
        expand_recursive(root.get());
    }
    RebuildVisibleList();
}

void TreeView::CollapseAll()
{
    std::function<void(TreeNode*)> collapse_recursive = [&](TreeNode* node) {
        if (!node) return;
        if (node->has_children())
        {
            node->is_expanded = false;
        }
        for (auto& child : node->children)
        {
            collapse_recursive(child.get());
        }
    };
    
    for (auto& root : _root_nodes)
    {
        collapse_recursive(root.get());
    }
    RebuildVisibleList();
}

void TreeView::ExpandCurrent()
{
    if (!_visible_nodes.empty() && _cursor_row >= 0 && _cursor_row < static_cast<int>(_visible_nodes.size()))
    {
        TreeNode* node = _visible_nodes[_cursor_row];
        if (node->has_children())
        {
            node->is_expanded = true;
            RebuildVisibleList();
        }
    }
}

void TreeView::CollapseCurrent()
{
    if (!_visible_nodes.empty() && _cursor_row >= 0 && _cursor_row < static_cast<int>(_visible_nodes.size()))
    {
        TreeNode* node = _visible_nodes[_cursor_row];
        if (node->has_children())
        {
            node->is_expanded = false;
            RebuildVisibleList();
        }
    }
}

void TreeView::ToggleExpansion(TreeNode* node)
{
    if (node && node->has_children())
    {
        node->is_expanded = !node->is_expanded;
        RebuildVisibleList();
    }
}

bool TreeView::MatchesSearch(TreeNode* node) const
{
    if (!_search_regex_valid || !node)
        return false;
    
    // Only search objects, not properties
    if (!node->is_object)
        return false;
    
    try
    {
        // Search both content and full path
        return std::regex_search(node->raw_content, _search_regex) ||
               std::regex_search(node->path, _search_regex);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

void TreeView::SetSearchPattern(const std::string& pattern)
{
    _search_pattern = pattern;
    _search_regex_valid = false;
    
    if (!pattern.empty())
    {
        _search_active = true;
        try
        {
            _search_regex = std::regex(pattern, std::regex_constants::icase);
            _search_regex_valid = true;
        }
        catch (const std::exception&)
        {
            // Invalid regex, keep _search_regex_valid as false
        }
    }
    else
    {
        _search_active = false;
    }
    
    UpdateSearchFlags();
    RebuildVisibleList();
    
    // Position cursor on first matching result to ensure it's visible
    if (_search_active && _search_regex_valid && !_visible_nodes.empty())
    {
        // Find first matching node in visible list
        for (size_t i = 0; i < _visible_nodes.size(); i++)
        {
            if (_visible_nodes[i]->matches_search)
            {
                _cursor_row = static_cast<int>(i);
                break;
            }
        }
    }
    else
    {
        _cursor_row = 0; // Reset cursor to top when search cleared
    }
}

void TreeView::ClearSearch()
{
    _search_active = false;
    _search_pattern.clear();
    _search_regex_valid = false;
    UpdateSearchFlags();
    RebuildVisibleList(); // This will automatically preserve cursor position
}

bool TreeView::SupportsSearch() const
{
    return true;
}

void TreeView::UpdateSearchFlags()
{
    for (auto& root : _root_nodes)
    {
        UpdateSearchFlagsRecursive(root.get());
    }
}

void TreeView::UpdateSearchFlagsRecursive(TreeNode* node)
{
    if (!node) return;
    
    // Clear flags
    node->matches_search = false;
    node->is_search_parent = false;
    
    if (_search_active && _search_regex_valid)
    {
        try
        {
            // Check if this node matches the search pattern
            bool matches = std::regex_search(node->raw_content, _search_regex) ||
                          std::regex_search(node->path, _search_regex);
            
            if (matches)
            {
                if (node->is_object)
                {
                    node->matches_search = true;
                    // Mark all parents as search parents and expand them
                    TreeNode* parent = node->parent;
                    while (parent)
                    {
                        parent->is_search_parent = true;
                        parent->is_expanded = true;
                        parent = parent->parent;
                    }
                }
                else if (node->parent)
                {
                    // Property matches - mark parent object instead
                    node->parent->matches_search = true;
                    // Expand the parent object to show the matching property
                    node->parent->is_expanded = true;
                    // Mark all grandparents as search parents and expand them
                    TreeNode* parent = node->parent->parent;
                    while (parent)
                    {
                        parent->is_search_parent = true;
                        parent->is_expanded = true;
                        parent = parent->parent;
                    }
                }
            }
        }
        catch (const std::exception&)
        {
            // Regex error, skip this node
        }
    }
    
    // Recurse to children
    for (auto& child : node->children)
    {
        UpdateSearchFlagsRecursive(child.get());
    }
}

int TreeView::CalculateNodeDistance(TreeNode* from, TreeNode* to) const
{
    if (!from || !to) return INT_MAX;
    if (from == to) return 0;
    
    // Check if one is an ancestor of the other
    TreeNode* ancestor = from->parent;
    int distance = 1;
    while (ancestor)
    {
        if (ancestor == to) return distance;
        ancestor = ancestor->parent;
        distance++;
    }
    
    ancestor = to->parent;
    distance = 1;
    while (ancestor)
    {
        if (ancestor == from) return distance;
        ancestor = ancestor->parent;
        distance++;
    }
    
    // Find common ancestor and calculate distance through it
    std::vector<TreeNode*> from_path;
    std::vector<TreeNode*> to_path;
    
    TreeNode* node = from;
    while (node)
    {
        from_path.push_back(node);
        node = node->parent;
    }
    
    node = to;
    while (node)
    {
        to_path.push_back(node);
        node = node->parent;
    }
    
    // Find common ancestor
    int common_depth = 0;
    while (common_depth < static_cast<int>(from_path.size()) && 
           common_depth < static_cast<int>(to_path.size()) &&
           from_path[from_path.size() - 1 - common_depth] == to_path[to_path.size() - 1 - common_depth])
    {
        common_depth++;
    }
    
    if (common_depth == 0) return INT_MAX; // No common ancestor
    
    // Distance is sum of distances to common ancestor
    return static_cast<int>(from_path.size()) + static_cast<int>(to_path.size()) - 2 * common_depth;
}