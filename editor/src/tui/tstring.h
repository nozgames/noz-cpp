//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "screen.h"

struct tcolor2_t
{
    uint8_t code;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct tchar_t
{
    char value;
    tcolor_t color;
    tcolor_t bg_color;
    tcolor2_t color1;
    tcolor2_t color2;
};

struct TString : Object {};
struct TStringBuilder : Object {};

TString* CreateTString(Allocator* allocator);
TStringBuilder* CreateTStringBuilder(Allocator* allocator, size_t capacity = 8192);

TStringBuilder* Append(TStringBuilder* builder, const char* text, tcolor_t color = TCOLOR_NONE, tcolor_t bg_color = TCOLOR_NONE);

void CStringToTChar(const char* str, tchar_t* buffer, u32 buffer_size);

#if 0

class TStringBuilder
{
    std::string _formatted;
    std::string _raw;
    std::vector<color24_t> _color_stack;

public:

    // Builder pattern methods - all return *this for chaining
    TStringBuilder& Add(const char* text);
    TStringBuilder& Add(const std::string& text);     // Add text in current color from stack
    TStringBuilder& Add(const std::string& text, const color24_t& color);  // Add text with specific color
    TStringBuilder& Add(const std::string& text, int r, int g, int b);  // Add text with RGB color
    TStringBuilder& Add(const std::string& text, int tcolor);  // Add text with RGB color

    // Type-specific overloads for common NoZ types
    TStringBuilder& Add(const TString& tstr);         // Add existing TString (text + visual length)
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
    //TStringBuilder& TruncateToWidth(size_t max_width);
    
    // Query methods (const)
    size_t VisualLength() const { return _raw.length(); }
    bool Empty() const { return _formatted.empty(); }
    
    // Final build method
    TString ToTString() const;
    
    // Static factory method
    static TStringBuilder Build() { return TStringBuilder(); }
};

#endif
