//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Color32
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
};

struct Color24
{
    u8 r;
    u8 g;
    u8 b;
};

struct Color
{
    float r;
    float g;
    float b;
    float a;
};

// Color32 functions
Color32 color32_create(u8 r, u8 g, u8 b, u8 a);
Color32 color32_from_color(Color* color);
Color32 color32_from_color24(Color24* color, u8 alpha);
bool color32_is_transparent(Color32* color);
bool color32_is_opaque(Color32* color);
bool color32_equals(Color32* a, Color32* b);

// Color24 functions
Color24 color24_create(u8 r, u8 g, u8 b);
Color24 color24_from_color(Color* color);
Color24 color24_from_color32(Color32* color);
bool color24_equals(Color24* a, Color24* b);

// Color functions
Color color_create(float r, float g, float b, float a);
Color color_from_color32(Color32* color);
Color color_from_color24(Color24* color, float alpha);
bool color_is_transparent(Color* color);
bool color_is_opaque(Color* color);
Color color_clamped(Color* color);
bool color_equals(Color* a, Color* b);
Color color_add(Color* a, Color* b);
Color color_subtract(Color* a, Color* b);
Color color_multiply_scalar(Color* color, float scalar);
Color color_multiply(Color* a, Color* b);

inline Color SetAlpha(const Color& color, float alpha)
{
    return { color.r, color.g, color.b, alpha };
}

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
extern Color32 color32_black;
extern Color32 color32_white;
extern Color32 color32_red;
extern Color32 color32_green;
extern Color32 color32_blue;
extern Color32 color32_transparent;

extern Color24 color24_black;
extern Color24 color24_white;
extern Color24 color24_red;
extern Color24 color24_green;
extern Color24 color24_blue;

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
