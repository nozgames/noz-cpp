//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once
#include <cmath>

typedef float f32;
typedef double f64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

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


}

float RandomFloat();
float RandomFloat(float min, float max);
int RandomInt(int min, int max);
bool RandomBool();
bool RandomBool(float probability);

struct Vec3;
struct Vec4;

struct Mat4
{
    f32 m[16];

    Mat4 operator*(const Mat4& m) const;
    Vec3 operator*(const Vec3& v) const;
    Vec4 operator*(const Vec4& v) const;
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
    Vec2 operator-() const { return { -x, -y }; }
};

struct Vec3
{
    f32 x;
    f32 y;
    f32 z;

    Vec3 operator+(const Vec3& v) const { return { x + v.x, y + v.y, z + v.z }; }
    Vec3 operator-(const Vec3& v) const { return { x - v.x, y - v.y, z - v.z }; }
    Vec3& operator+=(const Vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vec3 operator*(f32 scalar) const { return { x * scalar, y * scalar, z * scalar }; }
    Vec3 operator*=(f32 scalar) const { return { x * scalar, y * scalar, z * scalar }; }
    Vec3 operator-() const { return { -x, -y, -z }; }
};

struct Vec4
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;

    Vec4 operator/=(f32 scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
};

struct Vec2Int
{
    i32 x;
    i32 y;

    Vec2Int operator+(const Vec2Int& v) const { return { x + v.x, y + v.y }; }
    Vec2Int operator+(i32 v) const { return { x + v, y + v }; }
    Vec2Int operator-(const Vec2Int& v) const { return { x - v.x, y - v.y }; }
    Vec2Int operator-(i32 v) const { return { x - v, y - v }; }
    bool operator==(const Vec2Int& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2Int& o) const { return x != o.x || y != o.y; }
};

struct Vec2Double
{
    f64 x;
    f64 y;

    Vec2Double operator*(f64 scalar) const { return { x * scalar, y * scalar }; }
    Vec2Double operator/(f64 scalar) const { return { x / scalar, y / scalar }; }
    Vec2Double operator*(const Vec2Double& v) const { return { x * v.x, y * v.y }; }
    Vec2Double operator/(const Vec2Double& v) const { return { x / v.x, y / v.y }; }
    Vec2Double operator*=(f64 scalar) const { return { x * scalar, y * scalar }; }
    Vec2Double operator+(const Vec2Double& v) const { return { x + v.x, y + v.y }; }
    Vec2Double operator-(const Vec2Double& v) const { return { x - v.x, y - v.y }; }

    bool operator==(const Vec2Double& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2Double& o) const { return x != o.x || y != o.y; }
};

inline Vec2Double operator*(f64 scalar, const Vec2Double& v) { return { v.x * scalar, v.y * scalar }; }

struct Bounds2
{
    Vec2 min;
    Vec2 max;
};

struct Bounds3
{
    Vec3 min;
    Vec3 max;
};

struct quat
{
    float x;
    float y;
    float z;
    float w;
};

constexpr Mat3 MAT3_IDENTITY = { 1,0,0, 0,1,0, 0,0,1 };
constexpr Mat4 MAT4_IDENTITY = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

constexpr Vec3 VEC3_FORWARD = { 0, 0, 1 };
constexpr Vec3 VEC3_BACKWARD = { 0, 0, -1 };
constexpr Vec3 VEC3_UP = { 0, 1, 0 };
constexpr Vec3 VEC3_DOWN = { 0, -1, 0 };
constexpr Vec3 VEC3_RIGHT = { 1, 0, 0 };
constexpr Vec3 VEC3_LEFT = { -1, 0, 0 };
constexpr Vec3 VEC3_ZERO = { 0,0,0 };
constexpr Vec3 VEC3_ONE = { 1,1,1 };

constexpr Vec4 VEC4_ZERO = { 0,0,0,0 };
constexpr Vec4 VEC4_ONE = { 1,1,1,1 };

constexpr Vec2 VEC2_ZERO = { 0,0 };
constexpr Vec2 VEC2_ONE = { 1, 1 };
constexpr Vec2 VEC2_UP = { 0, -1 };
constexpr Vec2 VEC2_DOWN = { 0, 1 };
constexpr Vec2 VEC2_RIGHT = { 1, 0 };
constexpr Vec2 VEC2_LEFT = { -1, 0 };

constexpr Vec2Int VEC2INT_ZERO = { 0,0 };
constexpr Vec2Int VEC2INT_ONE = { 1,1 };

constexpr f32 F32_MAX = 3.402823466e+38F;
constexpr f32 F32_MIN = -3.402823466e+38F;

// @bounds3
Bounds3 ToBounds(const Vec3* positions, u32 count);
bool Contains(const Bounds3& bounds, const Vec3& point);
bool Intersects(const Bounds3& bounds, const Bounds3& point);
Bounds3 Expand(const Bounds3& bounds, const Vec3& point);
Bounds3 Expand(const Bounds3& bounds, const Bounds3& other);

// @bounds2
Bounds2 ToBounds(const Vec2* positions, u32 count);

// @mat4
extern Mat4 Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
extern Mat4 Ortho(f32 top, f32 bottom, f32 near, f32 far);
extern Mat4 Inverse(const Mat4& m);

// @mat3
extern Mat3 TRS(const Vec2& translation, f32 rotation, const Vec2& scale);
extern Mat3 Translate(const Vec2& translation);
extern Mat3 Inverse(const Mat3& m);

// @vec2
extern f32 Length(const Vec2& v);
extern Vec2 Reflect(const Vec2& v, const Vec2& normal);
extern Vec2 Normalize(const Vec2& v);
inline Vec2 Cross(const Vec2& a, const Vec2& b) { return Vec2{ -a.y, a.x }; }

// @vec2d
extern f64 Length(const Vec2Double& v);
extern Vec2Double Normalize(const Vec2Double& v);

// @vec3
extern f32 Length(const Vec3& v);
extern Vec3 Normalize(const Vec3& v);
extern Vec3 Cross(const Vec3& a, const Vec3& b);

inline f32 Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline f32 Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline f64 Dot(const Vec2Double& a, const Vec2Double& b) { return a.x * b.x + a.y * b.y; }

inline f32 Mix(f32 v1, f32 v2, f32 t) { return v1 + (v2 - v1) * t; }
inline f64 Mix(f64 v1, f64 v2, f64 t) { return v1 + (v2 - v1) * t; }
inline Vec2 Mix(const Vec2& v1, const Vec2& v2, f32 t) { return v1 + (v2 - v1) * t; }
inline Vec2Double Mix(const Vec2Double& v1, const Vec2Double& v2, f64 t) { return v1 + (v2 - v1) * t; }

inline i32 Clamp(i32 v, i32 min, i32 max) { return v < min ? min : v > max ? max : v; }
inline u32 Clamp(u32 v, u32 min, u32 max) { return v < min ? min : v > max ? max : v; }
inline f32 Clamp(f32 v, f32 min, f32 max) { return v < min ? min : v > max ? max : v; }
inline f64 Clamp(f64 v, f64 min, f64 max) { return v < min ? min : v > max ? max : v; }

inline float Radians(float degrees) { return degrees * noz::PI / 180.0f; }
inline float Degrees(float radians) { return radians * 180.0f / noz::PI; }

inline i32 Min(i32 v1, i32 v2) { return v1 < v2 ? v1 : v2; }
inline u32 Min(u32 v1, u32 v2) { return v1 < v2 ? v1 : v2; }
inline f32 Min(f32 v1, f32 v2) { return v1 < v2 ? v1 : v2; }
inline u64 Min(u64 v1, u64 v2) { return v1 < v2 ? v1 : v2; }
inline Vec2 Min(const Vec2& m1, const Vec2& m2) { return { Min(m1.x, m2.x), Min(m1.y, m2.y) }; }
inline Vec3 Min(const Vec3& m1, const Vec3& m2) { return { Min(m1.x, m2.x), Min(m1.y, m2.y), Min(m1.z, m2.z) }; }

inline i32 Max(i32 v1, i32 v2) { return v1 > v2 ? v1 : v2; }
inline f32 Max(f32 v1, f32 v2) { return v1 > v2 ? v1 : v2; }
inline u32 Max(u32 v1, u32 v2) { return v1 > v2 ? v1 : v2; }
inline u64 Max(u64 v1, u64 v2) { return v1 > v2 ? v1 : v2; }
inline Vec2 Max(const Vec2& m1, const Vec2& m2) { return { Max(m1.x, m2.x), Max(m1.y, m2.y) }; }
inline Vec3 Max(const Vec3& m1, const Vec3& m2) { return { Max(m1.x, m2.x), Max(m1.y, m2.y), Max(m1.z, m2.z) }; }

inline i32 Abs(i32 v) { return v < 0 ? -v : v; }
inline f32 Abs(f32 v) { return v < 0 ? -v : v; }
inline f64 Abs(f64 v) { return v < 0 ? -v : v; }

inline f32 Sqrt(f32 v) { return sqrtf(v); }
inline f32 Cos(f32 v) { return cosf(v); }
inline f32 Sin(f32 v) { return sinf(v); }

constexpr Vec2Int RoundToNearest(const Vec2Int& v)
{
    return { (i32)(v.x + 0.5f), (i32)(v.y + 0.5f) };
}

constexpr Vec2Int RoundToNearest(const Vec2Double& v)
{
    return { (i32)(v.x + 0.5f), (i32)(v.y + 0.5f) };
}


