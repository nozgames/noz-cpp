//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "properties_view.h"
#include "../tui/terminal.h"
#include "../tokenizer.h"
#include <algorithm>

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

void PropertiesView::FormatProperty(Item* property)
{
    if (!property->value.empty())
    {
        auto builder = TStringBuilder::Build();
        builder.Add(property->name + ": ");
        FormatValue(builder, property->value);
        property->formatted_content = builder.ToString();
    }
    else
    {
        property->formatted_content = TString(property->name, property->name.length());
    }
}

void PropertiesView::AddProperty(const std::string& name, const std::string& value, int indent_level)
{
    auto property = std::make_unique<Item>(name, value, indent_level);
    FormatProperty(property.get());
    _properties.push_back(std::move(property));
    RebuildVisibleList();
}

void PropertiesView::AddProperty(const std::string& name, const TString& value, int indent_level)
{
    auto property = std::make_unique<Item>(name, value, indent_level);
    FormatProperty(property.get());
    _properties.push_back(std::move(property));
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
    {
        _visible_properties.push_back(property.get());
    }
    
    // Ensure cursor is in valid range
    if (!_visible_properties.empty())
    {
        _cursor_row = std::max(0, std::min(_cursor_row, static_cast<int>(_visible_properties.size()) - 1));
    }
    else
    {
        _cursor_row = 0;
    }
}

size_t PropertiesView::PropertyCount() const
{
    return _properties.size();
}

void PropertiesView::Render(int width, int height)
{
    int properties_height = height - 2; // Leave 2 rows for status and command

    // Clear the properties area
    for (int row = 0; row < properties_height; row++)
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

            // Build display line using TStringBuilder
            auto line_builder = TStringBuilder::Build();
            
            // Add indentation
            for (int indent = 0; indent < property->indent_level; indent++)
            {
                line_builder.Add("  "); // 2 spaces per indent level
            }
            
            // Add content
            line_builder.Add(property->formatted_content);
            
            // Truncate if needed and build final TString
            line_builder.TruncateToWidth(width);
            TString display_line = line_builder.ToString();

            // Render with optional cursor highlighting
            int cursor_pos = (_show_cursor && static_cast<int>(i) == cursor_in_window) ? 0 : -1;
            AddStringWithCursor(display_line, cursor_pos);
        }
    }
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