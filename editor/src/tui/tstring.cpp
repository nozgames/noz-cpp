//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "tstring.h"
#include <algorithm>
#include <cstdio>

// Color constants
const color24_t TCOLOR_ORANGE = {255, 165, 0};
const color24_t TCOLOR_GREEN = {0, 128, 0};
const color24_t TCOLOR_PURPLE = {128, 0, 128};
const color24_t TCOLOR_GREY = {128, 128, 128};
const color24_t TCOLOR_WHITE = {255, 255, 255};
const color24_t TCOLOR_DISABLED = {64, 64, 64};

// TStringBuilder implementation
TStringBuilder& TStringBuilder::Add(const std::string& text)
{
    if (!_color_stack.empty())
    {
        // Use current color from stack
        const color24_t& color = _color_stack.back();
        return Add(text, color.r, color.g, color.b);
    }

    _raw += text;
    _formatted += text;
    return *this;
}

TStringBuilder& TStringBuilder::Add(const char* text)
{
    Add(std::string(text));
    return *this;
}

TStringBuilder& TStringBuilder::Add(const std::string& text, const color24_t& color)
{
    return Add(text, color.r, color.g, color.b);
}

TStringBuilder& TStringBuilder::Add(const std::string& text, int r, int g, int b)
{
    // Clamp RGB values
    r = std::max(0, std::min(255, r));
    g = std::max(0, std::min(255, g));
    b = std::max(0, std::min(255, b));
    
    _formatted += "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
    _formatted += text;
    _formatted += "\033[0m";
    _raw += text;
    return *this;
}

TStringBuilder& TStringBuilder::Add(const std::string& text, int tcolor)
{
    if (tcolor <= 0)
        return Add(text);

    _formatted += "\033[" + std::to_string(tcolor) + "m";
    _formatted += text;
    _formatted += "\033[0m";
    _raw += text;
    return *this;
}

TStringBuilder& TStringBuilder::PushColor(const color24_t& color)
{
    _color_stack.push_back(color);
    return *this;
}

TStringBuilder& TStringBuilder::PushColor(int r, int g, int b)
{
    _color_stack.push_back(color24_t{static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b)});
    return *this;
}

TStringBuilder& TStringBuilder::PopColor()
{
    if (!_color_stack.empty())
    {
        _color_stack.pop_back();
    }
    return *this;
}

TStringBuilder& TStringBuilder::Clear()
{
    _raw.clear();
    _formatted.clear();
    _color_stack.clear();
    return *this;
}

#if 0
TStringBuilder& TStringBuilder::TruncateToWidth(size_t max_width)
{
    if (_raw.size() <= max_width)
        return *this;
        
    // Need to truncate while preserving ANSI sequences
    size_t visual_len = 0;
    size_t truncate_pos = 0;
    
    for (size_t i = 0; i < _formatted.length(); i++)
    {
        if (_formatted[i] == '\033' && i + 1 < _formatted.length() && _formatted[i + 1] == '[')
        {
            // Skip ANSI escape sequence
            i++;
            while (i < _formatted.length() && _formatted[i] != 'm')
                i++;
            // Don't increment visual_len for ANSI codes
        }
        else
        {
            visual_len++;
            if (visual_len >= max_width)
            {
                truncate_pos = i + 1;
                break;
            }
        }
        truncate_pos = i + 1;
    }
    
    _formatted = _formatted.substr(0, truncate_pos);
    _formatted += "\033[0m"; // Ensure we end with a reset
    return *this;
}
#endif

// Type-specific Add overloads
TStringBuilder& TStringBuilder::Add(const TString& tstr)
{
    _raw += tstr.raw;
    _formatted += tstr.formatted;
    return *this;
}

TStringBuilder& TStringBuilder::Add(const vec2& v)
{
    Add("(", TCOLOR_GREY).Add(v.x)
        .Add(", ", TCOLOR_GREY).Add(v.y)
        .Add(")", TCOLOR_GREY);
    return *this;
}

TStringBuilder& TStringBuilder::Add(const vec3& v)
{
    Add("(", TCOLOR_GREY).Add(v.x)
        .Add(", ", TCOLOR_GREY).Add(v.y)
        .Add(", ", TCOLOR_GREY).Add(v.z)
        .Add(")", TCOLOR_GREY);
    return *this;
}

TStringBuilder& TStringBuilder::Add(const vec4& v)
{
    Add("(", TCOLOR_GREY).Add(v.x)
        .Add(", ", TCOLOR_GREY).Add(v.y)
        .Add(", ", TCOLOR_GREY).Add(v.z)
        .Add(", ", TCOLOR_GREY).Add(v.w)
        .Add(")", TCOLOR_GREY);
    return *this;
}

TStringBuilder& TStringBuilder::Add(const color24_t& color)
{
    char hex[8];
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", color.r, color.g, color.b);

    auto hex_str = std::string(hex);
    
    // Add color block with background color followed by hex text
    _formatted += "\033[48;2;" + std::to_string(color.r) + ";" + std::to_string(color.g) + ";" + std::to_string(color.b) + "m \033[0m";
    _formatted += " " + hex_str;
    _raw += hex_str;
    return *this;
}

TStringBuilder& TStringBuilder::Add(const color_t& color)
{
    // Convert color_t to color24_t
    color24_t color24;
    color24.r = static_cast<uint8_t>(color.r * 255);
    color24.g = static_cast<uint8_t>(color.g * 255);
    color24.b = static_cast<uint8_t>(color.b * 255);
    
    return Add(color24);
}

TStringBuilder& TStringBuilder::Add(bool value)
{
    Add(value ? "true" : "false", TCOLOR_PURPLE);
    return *this;
}

TStringBuilder& TStringBuilder::Add(int value)
{
    Add(std::to_string(value), TCOLOR_ORANGE);
    return *this;
}

TStringBuilder& TStringBuilder::Add(float value)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%g", value);
    Add(std::string(buffer), TCOLOR_ORANGE);
    return *this;
}