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
struct color_t
{
    float r;
    float g;
    float b;
    float a;
};

// Color32 functions
color32_t color32_create(u8 r, u8 g, u8 b, u8 a);
color32_t color32_from_color(color_t* color);
color32_t color32_from_color24(color24_t* color, u8 alpha);
bool color32_is_transparent(color32_t* color);
bool color32_is_opaque(color32_t* color);
bool color32_equals(color32_t* a, color32_t* b);

// Color24 functions
color24_t color24_create(u8 r, u8 g, u8 b);
color24_t color24_from_color(color_t* color);
color24_t color24_from_color32(color32_t* color);
bool color24_equals(color24_t* a, color24_t* b);

// Color functions
color_t color_create(float r, float g, float b, float a);
color_t color_from_color32(color32_t* color);
color_t color_from_color24(color24_t* color, float alpha);
bool color_is_transparent(color_t* color);
bool color_is_opaque(color_t* color);
color_t color_clamped(color_t* color);
bool color_equals(color_t* a, color_t* b);
color_t color_add(color_t* a, color_t* b);
color_t color_subtract(color_t* a, color_t* b);
color_t color_multiply_scalar(color_t* color, float scalar);
color_t color_multiply(color_t* a, color_t* b);
color_t color_lerp(color_t* a, color_t* b, float t);

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

extern color_t color_black;
extern color_t color_white;
extern color_t color_red;
extern color_t color_green;
extern color_t color_blue;
extern color_t color_transparent;