//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <noz/color.h>
#include <string>
#include <vector>

// Color constants
extern const color24_t TCOLOR_ORANGE;   // Numbers
extern const color24_t TCOLOR_GREEN;    // Strings
extern const color24_t TCOLOR_PURPLE;   // Booleans
extern const color24_t TCOLOR_GREY;     // Vector punctuation
extern const color24_t TCOLOR_WHITE;    // White text

// Final string product with visual length
struct TString
{
    std::string text;
    size_t visual_length;
    
    TString() : text(""), visual_length(0) {}
    TString(std::string str, size_t vis_len) : text(std::move(str)), visual_length(vis_len) {}
};

// Terminal String Builder for clean color management
class TStringBuilder
{
private:
    std::string _buffer;
    size_t _visual_length;
    std::vector<color24_t> _color_stack; // Color stack

public:
    TStringBuilder() : _visual_length(0) {}
    
    // Builder pattern methods - all return *this for chaining  
    TStringBuilder& Add(const std::string& text);     // Add text in current color from stack
    TStringBuilder& Add(const std::string& text, const color24_t& color);  // Add text with specific color
    TStringBuilder& Add(const std::string& text, int r, int g, int b);  // Add text with RGB color
    
    // Type-specific overloads for common NoZ types
    TStringBuilder& Add(const vec2& v);               // Format as "(x, y)" 
    TStringBuilder& Add(const vec3& v);               // Format as "(x, y, z)"
    TStringBuilder& Add(const vec4& v);               // Format as "(x, y, z, w)"
    TStringBuilder& Add(const color24_t& color);      // Format as hex color
    TStringBuilder& Add(const color_t& color);        // Format as hex color (from float color)
    TStringBuilder& Add(bool value);                  // Format as "true"/"false"
    TStringBuilder& Add(int value);                   // Format as integer
    TStringBuilder& Add(float value);                 // Format as float
    
    // Color stack management
    TStringBuilder& PushColor(const color24_t& color);
    TStringBuilder& PushColor(int r, int g, int b);
    TStringBuilder& PopColor();
    
    // Utility methods
    TStringBuilder& Clear();
    TStringBuilder& TruncateToWidth(size_t max_width);
    
    // Query methods (const)
    size_t VisualLength() const { return _visual_length; }
    bool Empty() const { return _buffer.empty(); }
    
    // Final build method
    TString ToString() const { return TString(_buffer, _visual_length); }
    
    // Static factory method
    static TStringBuilder Build() { return TStringBuilder(); }
};