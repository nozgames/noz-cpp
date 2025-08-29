//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "tree_view.h"
#include "../tui/terminal.h"
#include <algorithm>

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

void TreeView::AddLine(const std::string& line)
{
    int indent_level = CountLeadingTabs(line);
    std::string content = RemoveLeadingTabs(line);
    
    // Create new node - only expand root level nodes by default
    TreeNode node(content, indent_level, indent_level == 0);
    
    // Find parent and update hierarchy
    if (!nodes_.empty() && indent_level > 0)
    {
        // Find the most recent node with lower indent level (potential parent)
        for (int i = static_cast<int>(nodes_.size()) - 1; i >= 0; i--)
        {
            if (nodes_[i].indent_level < indent_level)
            {
                // This is the parent
                if (!nodes_[i].has_children)
                {
                    nodes_[i].has_children = true;
                    nodes_[i].first_child_index = nodes_.size();
                }
                nodes_[i].child_count++;
                break;
            }
        }
    }
    
    nodes_.push_back(node);
    
    // Keep only the most recent entries
    while (nodes_.size() > max_entries_)
    {
        nodes_.erase(nodes_.begin());
        // Rebuild hierarchy after removal
        RebuildVisibleList();
    }
    
    // Rebuild visible list and auto-scroll to bottom
    RebuildVisibleList();
    if (!visible_indices_.empty())
    {
        cursor_row_ = static_cast<int>(visible_indices_.size()) - 1;
    }
}

void TreeView::RebuildVisibleList()
{
    visible_indices_.clear();
    
    for (size_t i = 0; i < nodes_.size(); i++)
    {
        const TreeNode& node = nodes_[i];
        
        // Always add root level nodes
        if (node.indent_level == 0)
        {
            visible_indices_.push_back(i);
            continue;
        }
        
        // For child nodes, check if all parents are expanded
        bool should_be_visible = true;
        for (int j = static_cast<int>(i) - 1; j >= 0; j--)
        {
            if (nodes_[j].indent_level < node.indent_level)
            {
                // This is a parent node
                if (nodes_[j].has_children && !nodes_[j].is_expanded)
                {
                    should_be_visible = false;
                    break;
                }
                // Continue checking higher level parents
                if (nodes_[j].indent_level == 0)
                    break;
            }
        }
        
        if (should_be_visible)
        {
            visible_indices_.push_back(i);
        }
    }
}

void TreeView::ToggleExpansion(size_t node_index)
{
    if (node_index >= nodes_.size())
        return;
        
    TreeNode& node = nodes_[node_index];
    if (node.has_children)
    {
        node.is_expanded = !node.is_expanded;
        RebuildVisibleList();
    }
}

int TreeView::CountVisibleChildren(size_t node_index) const
{
    if (node_index >= nodes_.size() || !nodes_[node_index].has_children)
        return 0;
    
    const TreeNode& parent = nodes_[node_index];
    int count = 0;
    
    // Count immediate children that are visible
    for (size_t i = node_index + 1; i < nodes_.size(); i++)
    {
        if (nodes_[i].indent_level <= parent.indent_level)
            break; // No more children
            
        if (nodes_[i].indent_level == parent.indent_level + 1)
            count++;
    }
    
    return count;
}

void TreeView::Clear()
{
    nodes_.clear();
    visible_indices_.clear();
    cursor_row_ = 0;
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
    if (!visible_indices_.empty())
    {
        size_t total_visible = visible_indices_.size();
        size_t max_display_count = std::min(static_cast<size_t>(tree_height), total_visible);
        
        // Ensure cursor is within valid range
        cursor_row_ = std::max(0, std::min(cursor_row_, static_cast<int>(total_visible) - 1));
        
        // Calculate window to show cursor
        size_t start_idx = 0;
        if (total_visible > max_display_count)
        {
            int cursor_pos = cursor_row_;
            int ideal_start = cursor_pos - static_cast<int>(max_display_count) / 2;
            int max_start = static_cast<int>(total_visible) - static_cast<int>(max_display_count);
            
            start_idx = std::max(0, std::min(ideal_start, max_start));
        }
        
        size_t display_count = std::min(max_display_count, total_visible - start_idx);
        int cursor_in_window = cursor_row_ - static_cast<int>(start_idx);

        for (size_t i = 0; i < display_count; i++)
        {
            size_t node_index = visible_indices_[start_idx + i];
            const TreeNode& node = nodes_[node_index];
            
            MoveCursor(static_cast<int>(i), 0);

            // Build display string with indentation and expansion indicators
            std::string display_line;
            
            // Add indentation
            for (int indent = 0; indent < node.indent_level; indent++)
            {
                display_line += "  "; // 2 spaces per indent level
            }
            
            // Add expansion indicator for nodes with children
            if (node.has_children)
            {
                if (node.is_expanded)
                    display_line += "- ";
                else
                    display_line += "+ ";
            }
            else if (node.indent_level > 0)
            {
                display_line += "  ";
            }
            
            // Add content
            display_line += node.content;
            
            // Truncate if too long
            if (display_line.length() > static_cast<size_t>(width))
            {
                display_line = display_line.substr(0, width);
            }

            // Render with optional cursor highlighting
            int cursor_pos = (show_cursor_ && static_cast<int>(i) == cursor_in_window) ? 0 : -1;
            AddStringWithCursor(display_line, cursor_pos);
        }
    }
}

size_t TreeView::NodeCount() const
{
    return nodes_.size();
}

size_t TreeView::VisibleCount() const
{
    return visible_indices_.size();
}

void TreeView::SetMaxEntries(size_t max_entries)
{
    max_entries_ = max_entries;

    // Trim existing nodes if needed
    while (nodes_.size() > max_entries_)
    {
        nodes_.erase(nodes_.begin());
    }
    
    RebuildVisibleList();
}

bool TreeView::HandleKey(int key)
{
    switch (key)
    {
        case KEY_UP:
            if (cursor_row_ > 0)
            {
                cursor_row_--;
            }
            return true;
            
        case KEY_DOWN:
            if (!visible_indices_.empty() && cursor_row_ < static_cast<int>(visible_indices_.size()) - 1)
            {
                cursor_row_++;
            }
            return true;
            
        case KEY_PPAGE:  // Page Up
            cursor_row_ = std::max(0, cursor_row_ - 10);
            return true;
            
        case KEY_NPAGE:  // Page Down
            if (!visible_indices_.empty())
            {
                cursor_row_ = std::min(cursor_row_ + 10, static_cast<int>(visible_indices_.size()) - 1);
            }
            return true;
            
        case KEY_HOME:
            cursor_row_ = 0;
            return true;
            
        case KEY_END:
            if (!visible_indices_.empty())
            {
                cursor_row_ = static_cast<int>(visible_indices_.size()) - 1;
            }
            return true;
            
        case KEY_RIGHT:
        case ' ':  // Space also expands
            // Expand current node
            if (!visible_indices_.empty() && cursor_row_ >= 0 && cursor_row_ < static_cast<int>(visible_indices_.size()))
            {
                size_t node_index = visible_indices_[cursor_row_];
                ToggleExpansion(node_index);
            }
            return true;
            
        case KEY_LEFT:
            // Collapse current node or move to parent
            if (!visible_indices_.empty() && cursor_row_ >= 0 && cursor_row_ < static_cast<int>(visible_indices_.size()))
            {
                size_t node_index = visible_indices_[cursor_row_];
                const TreeNode& node = nodes_[node_index];
                
                if (node.has_children && node.is_expanded)
                {
                    // Collapse current node
                    ToggleExpansion(node_index);
                }
                else if (node.indent_level > 0)
                {
                    // Move to parent
                    for (int i = cursor_row_ - 1; i >= 0; i--)
                    {
                        size_t parent_index = visible_indices_[i];
                        if (nodes_[parent_index].indent_level < node.indent_level)
                        {
                            cursor_row_ = i;
                            break;
                        }
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
    cursor_row_ = std::max(0, cursor_row_ - lines);
}

void TreeView::ScrollDown(int lines)
{
    if (!visible_indices_.empty())
    {
        cursor_row_ = std::min(cursor_row_ + lines, static_cast<int>(visible_indices_.size()) - 1);
    }
}

void TreeView::ScrollToTop()
{
    cursor_row_ = 0;
}

void TreeView::ScrollToBottom()
{
    if (!visible_indices_.empty())
    {
        cursor_row_ = static_cast<int>(visible_indices_.size()) - 1;
    }
}

void TreeView::SetCursorVisible(bool visible)
{
    show_cursor_ = visible;
}

void TreeView::SetCursorPosition(int row, int col)
{
    cursor_row_ = row;
}

void TreeView::ExpandAll()
{
    for (TreeNode& node : nodes_)
    {
        if (node.has_children)
        {
            node.is_expanded = true;
        }
    }
    RebuildVisibleList();
}

void TreeView::CollapseAll()
{
    for (TreeNode& node : nodes_)
    {
        if (node.has_children)
        {
            node.is_expanded = false;
        }
    }
    RebuildVisibleList();
}

void TreeView::ExpandCurrent()
{
    if (!visible_indices_.empty() && cursor_row_ >= 0 && cursor_row_ < static_cast<int>(visible_indices_.size()))
    {
        size_t node_index = visible_indices_[cursor_row_];
        TreeNode& node = nodes_[node_index];
        if (node.has_children)
        {
            node.is_expanded = true;
            RebuildVisibleList();
        }
    }
}

void TreeView::CollapseCurrent()
{
    if (!visible_indices_.empty() && cursor_row_ >= 0 && cursor_row_ < static_cast<int>(visible_indices_.size()))
    {
        size_t node_index = visible_indices_[cursor_row_];
        TreeNode& node = nodes_[node_index];
        if (node.has_children)
        {
            node.is_expanded = false;
            RebuildVisibleList();
        }
    }
}