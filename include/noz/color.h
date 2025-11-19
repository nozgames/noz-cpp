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

inline Color ToLinear(Color srgb) {
    return Color {
        .r = powf(srgb.r, 2.2f),
        .g = powf(srgb.g, 2.2f),
        .b = powf(srgb.b, 2.2f),
        .a = srgb.a
    };
}

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

constexpr f32 COLOR_UV_ATLAS_WIDTH = 128;
constexpr f32 COLOR_UV_ATLAS_HEIGHT = 128;
constexpr f32 COLOR_UV_SIZE = 8;

constexpr Vec2 ColorUV(int col, int row)
{
    return {
        ((f32)col * COLOR_UV_SIZE + COLOR_UV_SIZE * 0.5f) / COLOR_UV_ATLAS_WIDTH,
        ((f32)row * COLOR_UV_SIZE + COLOR_UV_SIZE * 0.5f) / COLOR_UV_ATLAS_HEIGHT
    };
}

constexpr Color Color32ToColor(u8 r, u8 g, u8 b, u8 a) {
    return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
}

constexpr Color Color24ToColor(u8 r, u8 g, u8 b) {
    return { r / 255.0f, g / 255.0f, b / 255.0f, 1.0f };
}

constexpr Color Color24ToColor(u32 rgb) {
    return Color32ToColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 255);
}

constexpr Color Color32ToColor(u32 rgba) {
    return Color32ToColor((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, rgba & 0xFF);
}

constexpr Color COLOR_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
constexpr Color COLOR_WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr Color COLOR_RED = {1.0f, 0.0f, 0.0f, 1.0f};
constexpr Color COLOR_YELLOW = {1.0f, 1.0f, 0.0f, 1.0f};
constexpr Color COLOR_PURPLE = {1.0f, 0.0f, 1.0f, 1.0f};
constexpr Color COLOR_GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
constexpr Color COLOR_BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
constexpr Color COLOR_TRANSPARENT = {0.0f, 0.0f, 0.0f, 0.0f};
