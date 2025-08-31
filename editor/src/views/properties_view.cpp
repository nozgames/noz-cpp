//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "properties_view.h"
#include "../tui/terminal.h"
#include "../tokenizer.h"
#include <algorithm>

void PropertiesView::AddProperty(const std::string& name, const TString& value)
{
    _properties.push_back(std::move(std::make_unique<Item>(name, value)));
    RebuildVisibleList();
}

void PropertiesView::Clear()
{
    _properties.clear();
    _visible_properties.clear();
    _cursor_row = 0;
}

void PropertiesView::RebuildVisibleList()
{
    _visible_properties.clear();
    for (auto& property : _properties)
        _visible_properties.push_back(property.get());

    if (!_visible_properties.empty())
        _cursor_row = std::max(0, std::min(_cursor_row, static_cast<int>(_visible_properties.size()) - 1));
    else
        _cursor_row = 0;
}

size_t PropertiesView::PropertyCount() const
{
    return _properties.size();
}

void PropertiesView::Render(const RectInt& rect)
{
#if 0
    for (int row = 0; row < height; row++)
    {
        MoveCursor(row, 0);
        for (int col = 0; col < width; col++)
            AddChar(' ');
    }

    // Calculate which properties to show based on cursor position
    if (!_visible_properties.empty())
    {
        size_t total_visible = _visible_properties.size();
        size_t max_display_count = std::min(static_cast<size_t>(properties_height), total_visible);
        
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
            Item* property = _visible_properties[start_idx + i];
            MoveCursor(static_cast<int>(i), 0);
            int cursor_pos = (_show_cursor && static_cast<int>(i) == cursor_in_window) ? 0 : -1;
            AddString(property->value, cursor_pos, width);
        }
    }
#endif
}

bool PropertiesView::HandleKey(int key)
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
            if (!_visible_properties.empty() && _cursor_row < static_cast<int>(_visible_properties.size()) - 1)
            {
                _cursor_row++;
            }
            return true;
            
        case KEY_PPAGE:  // Page Up
            _cursor_row = std::max(0, _cursor_row - 10);
            return true;
            
        case KEY_NPAGE:  // Page Down
            if (!_visible_properties.empty())
            {
                _cursor_row = std::min(_cursor_row + 10, static_cast<int>(_visible_properties.size()) - 1);
            }
            return true;
            
        case KEY_HOME:
            _cursor_row = 0;
            return true;
            
        case KEY_END:
            if (!_visible_properties.empty())
            {
                _cursor_row = static_cast<int>(_visible_properties.size()) - 1;
            }
            return true;
            
        default:
            return false;
    }
}

void PropertiesView::ScrollUp(int lines)
{
    _cursor_row = std::max(0, _cursor_row - lines);
}

void PropertiesView::ScrollDown(int lines)
{
    if (!_visible_properties.empty())
    {
        _cursor_row = std::min(_cursor_row + lines, static_cast<int>(_visible_properties.size()) - 1);
    }
}

void PropertiesView::ScrollToTop()
{
    _cursor_row = 0;
}

void PropertiesView::ScrollToBottom()
{
    if (!_visible_properties.empty())
    {
        _cursor_row = static_cast<int>(_visible_properties.size()) - 1;
    }
}

void PropertiesView::SetCursorVisible(bool visible)
{
    _show_cursor = visible;
}

void PropertiesView::SetCursorPosition(int row, int col)
{
    _cursor_row = row;
}