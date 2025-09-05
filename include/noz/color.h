//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <stdint.h>
#include <stdbool.h>

// 32-bit RGBA color (0-255 per component)
struct color32_t
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

// 24-bit RGB color (0-255 per component)
struct color24_t
{
    u8 r;
    u8 g;
    u8 b;
};

// Floating point RGBA color (0.0-1.0 per component)
struct Color
{
    float r;
    float g;
    float b;
    float a;
};

// Color32 functions
color32_t color32_create(u8 r, u8 g, u8 b, u8 a);
color32_t color32_from_color(Color* color);
color32_t color32_from_color24(color24_t* color, u8 alpha);
bool color32_is_transparent(color32_t* color);
bool color32_is_opaque(color32_t* color);
bool color32_equals(color32_t* a, color32_t* b);

// Color24 functions
color24_t color24_create(u8 r, u8 g, u8 b);
color24_t color24_from_color(Color* color);
color24_t color24_from_color32(color32_t* color);
bool color24_equals(color24_t* a, color24_t* b);

// Color functions
Color color_create(float r, float g, float b, float a);
Color color_from_color32(color32_t* color);
Color color_from_color24(color24_t* color, float alpha);
bool color_is_transparent(Color* color);
bool color_is_opaque(Color* color);
Color color_clamped(Color* color);
bool color_equals(Color* a, Color* b);
Color color_add(Color* a, Color* b);
Color color_subtract(Color* a, Color* b);
Color color_multiply_scalar(Color* color, float scalar);
Color color_multiply(Color* a, Color* b);
Color color_lerp(Color* a, Color* b, float t);

inline Color Mix(const Color& a, const Color& b, float t)
{
    return {
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t,
        a.a + (b.a - a.a) * t
    };
}

// Predefined colors
extern color32_t color32_black;
extern color32_t color32_white;
extern color32_t color32_red;
extern color32_t color32_green;
extern color32_t color32_blue;
extern color32_t color32_transparent;

extern color24_t color24_black;
extern color24_t color24_white;
extern color24_t color24_red;
extern color24_t color24_green;
extern color24_t color24_blue;

extern Color COLOR_BLACK;
extern Color COLOR_WHITE;
extern Color COLOR_RED;
extern Color COLOR_GREEN;
extern Color COLOR_BLUE;
extern Color COLOR_TRANSPARENT;

constexpr Vec2 ColorUV(int row, int col, int atlas_width = 1024, int atlas_height = 1024, int color_size=8)
{
    return {
        ((f32)col * (f32)color_size + (f32)color_size * 0.5f) / (f32)atlas_width,
        ((f32)row * (f32)color_size + (f32)color_size * 0.5f) / (f32)atlas_height
    };
}
