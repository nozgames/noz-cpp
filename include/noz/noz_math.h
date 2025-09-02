//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

namespace noz
{
    constexpr float PI = 3.14159265359f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = PI * 0.5f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    constexpr int MB = 1024 * 1024;

    // @power
    constexpr uint32_t NextPowerOf2(uint32_t n)
    {
        if (n <= 1) return 1;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        return n + 1;
    }

    constexpr size_t NextPowerOf2(size_t n)
    {
        if (n <= 1) return 2;
        n--;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

    // @rounding
    constexpr int RoundToNearest(float v)
    {
        return (int)(v + 0.5f);
    }

    constexpr glm::ivec2 RoundToNearest(const glm::vec2& v)
    {
        return glm::ivec2((int)(v.x + 0.5f), (int)(v.y + 0.5f));
    }

    constexpr glm::ivec2 RoundToNearest(const glm::dvec2& v)
    {
        return glm::ivec2((int)(v.x + 0.5f), (int)(v.y + 0.5f));
    }
}

float RandomFloat();
float RandomFloat(float min, float max);
int RandomInt(int min, int max);
bool RandomBool();
bool RandomBool(float probability);




struct Mat4
{
    f32 m[16];
    
    operator glm::mat4() const;
};

struct Mat3
{
    f32 m[9];
    
    operator Mat4() const;
};

struct Vec2
{
    f32 x;
    f32 y;

    Vec2 operator+(const Vec2& v) const { return Vec2{ x + v.x, y + v.y }; }
    Vec2 operator-(const Vec2& v) const { return Vec2{ x - v.x, y - v.y }; }
    Vec2& operator+=(const Vec2& v) { x += v.x; y += v.y; return *this; }
    Vec2 operator*(f32 scalar) const { return Vec2{ x * scalar, y * scalar }; }
    Vec2 operator*=(f32 scalar) const { return Vec2{ x * scalar, y * scalar }; }

    operator glm::vec2() const { return { x, y }; }
};

struct Vec2Int
{
    i32 x;
    i32 y;
};

constexpr Mat3 MAT3_IDENTITY = { 1,0,0, 0,1,0, 0,0,1 };
constexpr Mat4 MAT4_IDENTITY = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

Mat4 Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
Mat4 Ortho(f32 top, f32 bottom, f32 near, f32 far);

Mat3 TRS(const Vec2& translation, f32 rotation, const Vec2& scale);

// @vec2
Vec2 Reflect(const Vec2& v, const Vec2& normal);
Vec2 Normalize(const Vec2& v);

inline Vec2 Lerp(const Vec2& v1, const Vec2& v2, float t) { return v1 + (v2 - v1) * t; }
inline float Lerp(const float& v1, const float& v2, float t) { return v1 + (v2 - v1) * t; }

inline float Radians(float degrees) { return degrees * noz::PI / 180.0f; }
inline float Degrees(float radians) { return radians * 180.0f / noz::PI; }


inline i32 Min(i32 v1, i32 v2) { return v1 < v2 ? v1 : v2; }