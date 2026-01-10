//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz {

    struct Color32 {
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
    extern Color32 ToColor32(const Color& color);
    Color32 color32_from_color24(Color24* color, u8 alpha);
    bool color32_is_transparent(Color32* color);
    bool color32_is_opaque(Color32* color);
    bool color32_equals(Color32* a, Color32* b);

    inline Color ToLinear(Color color) {
        return color;
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

    inline Color SetAlpha(const Color& color, float alpha){
        return { color.r, color.g, color.b, alpha };
    }

    inline Color32 SetAlpha(const Color32& color, u8 alpha){
        return { color.r, color.g, color.b, alpha };
    }

    inline Color32 SetAlpha(const Color32& color, float alpha){
        return { color.r, color.g, color.b, static_cast<u8>(alpha * 255.0f) };
    }

    inline Color MultiplyAlpha(const Color& color, float multiply) {
        return { color.r, color.g, color.b, multiply * color.a };
    }

    inline Color32 MultiplyAlpha(const Color32& color, float multiply){
        return { color.r, color.g, color.b, static_cast<u8>(multiply * color.a) };
    }

    inline Color32 MultiplyAlpha(const Color32& color, u8 multiply){
        return { color.r, color.g, color.b, static_cast<u8>((multiply / 255.0f) * (color.a / 255.0f) * 255.0f) };
    }

    inline Color32 Blend(const Color32& a, const Color32& b) {
        float alpha = b.a / 255.0f;
        return {
            static_cast<u8>(a.r + (b.r - a.r) * alpha),
            static_cast<u8>(a.g + (b.g - a.g) * alpha),
            static_cast<u8>(a.b + (b.b - a.b) * alpha),
            static_cast<u8>(a.a + (b.a - a.a) * alpha)
        };
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

    constexpr f32 COLOR_UV_ATLAS_WIDTH = 512;
    constexpr f32 COLOR_UV_ATLAS_HEIGHT = 512;
    constexpr f32 COLOR_UV_SIZE = 8;
    constexpr i32 COLOR_COUNT = 64;
    constexpr int COLOR_PALETTE_COUNT = 64;

    constexpr Vec2 ColorUV(int col, int row) {
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

    constexpr Color Color8ToColor(u8 r) {
        return { r / 255.0f, r / 255.0f, r / 255.0f, 1.0f };
    }

    constexpr Color Color24ToColor(u32 rgb) {
        return Color32ToColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 255);
    }

    constexpr Color Color32ToColor(u32 rgba) {
        return Color32ToColor((rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF, rgba & 0xFF);
    }

    constexpr Color Color32ToColor(u32 rgb, float alpha) {
        return Color32ToColor((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, (u8)(alpha * 255.0f));
    }

    constexpr Color32 ColorToColor32(float r, float g, float b, float a) {
        return Color32 {
            static_cast<u8>(r * 255.0f),
            static_cast<u8>(g * 255.0f),
            static_cast<u8>(b * 255.0f),
            static_cast<u8>(a * 255.0f)
        };
    }

    constexpr Color32 ColorToColor32(const Color& color) {
        return ColorToColor32(color.r, color.g, color.b, color.a);
    }

    constexpr Color COLOR_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
    constexpr Color COLOR_WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
    constexpr Color COLOR_RED = {1.0f, 0.0f, 0.0f, 1.0f};
    constexpr Color COLOR_YELLOW = {1.0f, 1.0f, 0.0f, 1.0f};
    constexpr Color COLOR_PURPLE = {1.0f, 0.0f, 1.0f, 1.0f};
    constexpr Color COLOR_GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
    constexpr Color COLOR_BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
    constexpr Color COLOR_TRANSPARENT = {0.0f, 0.0f, 0.0f, 0.0f};

    constexpr Color32 COLOR32_TRANSPARENT = {0,0,0,0};


    constexpr Color COLOR_BLACK_2PCT = {0,0,0,0.02f};
    constexpr Color COLOR_BLACK_5PCT = {0,0,0,0.05f};
    constexpr Color COLOR_BLACK_10PCT = {0,0,0,0.1f};

    constexpr Color COLOR_WHITE_1PCT = {1,1,1,0.01f};
    constexpr Color COLOR_WHITE_2PCT = {1,1,1,0.02f};
    constexpr Color COLOR_WHITE_5PCT = {1,1,1,0.05f};
    constexpr Color COLOR_WHITE_10PCT = {1,1,1,0.1f};
}
