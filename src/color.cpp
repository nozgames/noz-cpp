//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "noz/color.h"
#include <math.h>

// Helper function to clamp float values
static float clamp_float(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Helper function to clamp and convert float to uint8_t
static uint8_t float_to_uint8(float value)
{
    return (uint8_t)clamp_float(value * 255.0f, 0.0f, 255.0f);
}

// Color32 functions
color32_t color32_create(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    color32_t color = {r, g, b, a};
    return color;
}

color32_t color32_from_color(const Color* color)
{
    if (!color) 
    {
        return color32_create(0, 0, 0, 255);
    }
    
    color32_t result = 
    {
        float_to_uint8(color->r),
        float_to_uint8(color->g),
        float_to_uint8(color->b),
        float_to_uint8(color->a)
    };
    return result;
}

color32_t color32_from_color24(const color24_t* color, uint8_t alpha)
{
    if (!color) 
    {
        return color32_create(0, 0, 0, alpha);
    }
    
    return color32_create(color->r, color->g, color->b, alpha);
}

bool color32_is_transparent(const color32_t* color)
{
    return color && color->a == 0;
}

bool color32_is_opaque(const color32_t* color)
{
    return color && color->a == 255;
}

bool color32_equals(const color32_t* a, const color32_t* b)
{
    if (!a || !b) return a == b;
    return a->r == b->r && a->g == b->g && a->b == b->b && a->a == b->a;
}

// Color24 functions
color24_t color24_create(uint8_t r, uint8_t g, uint8_t b)
{
    color24_t color = {r, g, b};
    return color;
}

color24_t color24_from_color(const Color* color)
{
    if (!color) 
    {
        return color24_create(0, 0, 0);
    }
    
    color24_t result = 
    {
        float_to_uint8(color->r),
        float_to_uint8(color->g),
        float_to_uint8(color->b)
    };
    return result;
}

color24_t color24_from_color32(const color32_t* color)
{
    if (!color) 
    {
        return color24_create(0, 0, 0);
    }
    
    return color24_create(color->r, color->g, color->b);
}

bool color24_equals(const color24_t* a, const color24_t* b)
{
    if (!a || !b) return a == b;
    return a->r == b->r && a->g == b->g && a->b == b->b;
}

// Color functions
Color color_create(float r, float g, float b, float a)
{
    Color color = {r, g, b, a};
    return color;
}

Color color_from_color32(const color32_t* color)
{
    if (!color) 
    {
        return color_create(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    Color result =
    {
        color->r / 255.0f,
        color->g / 255.0f,
        color->b / 255.0f,
        color->a / 255.0f
    };
    return result;
}

Color color_from_color24(const color24_t* color, float alpha)
{
    if (!color) 
    {
        return color_create(0.0f, 0.0f, 0.0f, alpha);
    }
    
    Color result =
    {
        color->r / 255.0f,
        color->g / 255.0f,
        color->b / 255.0f,
        alpha
    };
    return result;
}

bool color_is_transparent(const Color* color)
{
    return color && color->a <= 0.0f;
}

bool color_is_opaque(const Color* color)
{
    return color && color->a >= 1.0f;
}

Color color_clamped(const Color* color)
{
    if (!color) 
    {
        return color_create(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    return color_create(
        clamp_float(color->r, 0.0f, 1.0f),
        clamp_float(color->g, 0.0f, 1.0f),
        clamp_float(color->b, 0.0f, 1.0f),
        clamp_float(color->a, 0.0f, 1.0f)
    );
}

bool color_equals(const Color* a, const Color* b)
{
    if (!a || !b) return a == b;
    
    const float epsilon = 1e-6f;
    return fabsf(a->r - b->r) < epsilon && 
           fabsf(a->g - b->g) < epsilon && 
           fabsf(a->b - b->b) < epsilon &&
           fabsf(a->a - b->a) < epsilon;
}

Color color_add(const Color* a, const Color* b)
{
    if (!a || !b) 
    {
        return color_create(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    return color_create(a->r + b->r, a->g + b->g, a->b + b->b, a->a + b->a);
}

Color color_subtract(const Color* a, const Color* b)
{
    if (!a || !b) 
    {
        return color_create(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    return color_create(a->r - b->r, a->g - b->g, a->b - b->b, a->a - b->a);
}

Color color_multiply_scalar(const Color* color, float scalar)
{
    if (!color) 
    {
        return color_create(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    return color_create(color->r * scalar, color->g * scalar, color->b * scalar, color->a * scalar);
}

Color color_multiply(const Color* a, const Color* b)
{
    if (!a || !b) 
    {
        return color_create(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    return color_create(a->r * b->r, a->g * b->g, a->b * b->b, a->a * b->a);
}

Color color_lerp(const Color* a, const Color* b, float t)
{
    if (!a || !b) 
    {
        return color_create(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    return color_create(
        a->r + (b->r - a->r) * t,
        a->g + (b->g - a->g) * t,
        a->b + (b->b - a->b) * t,
        a->a + (b->a - a->a) * t
    );
}

// @color32
color32_t color32_black = {0, 0, 0, 255};
color32_t color32_white = {255, 255, 255, 255};
color32_t color32_red = {255, 0, 0, 255};
color32_t color32_green = {0, 255, 0, 255};
color32_t color32_blue = {0, 0, 255, 255};
color32_t color32_transparent = {0, 0, 0, 0};

// @color24
color24_t color24_black = {0, 0, 0};
color24_t color24_white = {255, 255, 255};
color24_t color24_red = {255, 0, 0};
color24_t color24_green = {0, 255, 0};
color24_t color24_blue = {0, 0, 255};

// @color
Color COLOR_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
Color COLOR_WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
Color COLOR_RED = {1.0f, 0.0f, 0.0f, 1.0f};
Color COLOR_GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
Color COLOR_BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
Color COLOR_TRANSPARENT = {0.0f, 0.0f, 0.0f, 0.0f};

