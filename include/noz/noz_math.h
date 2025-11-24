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
    constexpr int GB = 1024 * 1024 * 1024;

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

struct Vec3;
struct Vec4;
struct Vec2;

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
    Vec2 operator*(const Vec2& v) const;
    Vec3 operator*(const Vec3& v) const;
    Mat3 operator*(const Mat3& o) const;
};

struct Vec2 {
    f32 x;
    f32 y;
};

inline Vec2 operator*(f32 scalar, const Vec2& v) { return Vec2{ v.x * scalar, v.y * scalar }; }

struct Vec3 {
    f32 x;
    f32 y;
    f32 z;
};

struct Vec4 {
    f32 x;
    f32 y;
    f32 z;
    f32 w;

    float& operator [] (int index) { return *(reinterpret_cast<float*>(this) + index); }
    float operator [] (int index) const { return *((float*)this + index); }
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

struct Bounds2 {
    Vec2 min;
    Vec2 max;
};

struct Bounds3 {
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

struct Transform {
    Vec2 position;
    Vec2 scale;
    float rotation;
    Mat3 local_to_world;
    Mat3 world_to_local;
    u32 flags;
};

constexpr u32 TRANSFORM_FLAG_DIRTY = 1 << 0;

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
constexpr Vec2 VEC2_NEGATIVE_ONE = { -1, -1 };
constexpr Vec2 VEC2_UP = { 0, -1 };
constexpr Vec2 VEC2_DOWN = { 0, 1 };
constexpr Vec2 VEC2_RIGHT = { 1, 0 };
constexpr Vec2 VEC2_LEFT = { -1, 0 };

constexpr Vec2Int VEC2INT_ZERO = { 0,0 };
constexpr Vec2Int VEC2INT_ONE = { 1,1 };

constexpr f32 F32_MAX = 3.402823466e+38F;
constexpr f32 F32_MIN = -3.402823466e+38F;
constexpr f32 F32_EPSILON = 1.192092896e-07F;

constexpr i32 I32_MAX = 2147483647;
constexpr i32 I32_MIN = -2147483648;
constexpr u32 U32_MAX = 0xFFFFFFFF;
constexpr u32 U32_MIN = 0;

constexpr Bounds2 BOUNDS2_ZERO = { VEC2_ZERO, VEC2_ZERO };

// @random
float RandomFloat();
float RandomFloat(float min, float max);
int RandomInt(int min, int max);
bool RandomBool();
bool RandomBool(float probability);
Vec2 RandomVec2(const Vec2& min, const Vec2& max);

// @float
extern float Repeat(float t, float length);
inline f32 Floor(f32 v2) { return floorf(v2); }
inline i32 FloorToInt(f32 v2) { return static_cast<int>(floorf(v2)); }
inline f32 Ceil(f32 v2) { return ceilf(v2); }
inline i32 CeilToInt(f32 v2) { return static_cast<int>(ceilf(v2)); }
inline f32 Min(f32 v1, f32 v2) { return v1 < v2 ? v1 : v2; }
inline f32 Max(f32 v1, f32 v2) { return v1 > v2 ? v1 : v2; }
inline f32 Mix(f32 v1, f32 v2, f32 t) { return v1 + (v2 - v1) * t; }
inline f64 Mix(f64 v1, f64 v2, f64 t) { return v1 + (v2 - v1) * t; }
inline bool ApproxEqual(f32 a, f32 b, f32 epsilon = 1e-6f) { return fabsf(a - b) <= epsilon; }
inline f32 Sqrt(f32 v) { return sqrtf(v); }
inline float Sqr(float x) { return x * x; }
inline f32 Cos(f32 v) { return cosf(v); }
inline f32 Sin(f32 v) { return sinf(v); }
inline f32 Atan2(f32 y, f32 x) { return atan2f(y, x); }

// @min
inline i32 Min(i32 v1, i32 v2) { return v1 < v2 ? v1 : v2; }
inline u32 Min(u32 v1, u32 v2) { return v1 < v2 ? v1 : v2; }
inline u64 Min(u64 v1, u64 v2) { return v1 < v2 ? v1 : v2; }
inline Vec2 Min(const Vec2& m1, const Vec2& m2) { return { Min(m1.x, m2.x), Min(m1.y, m2.y) }; }
inline Vec3 Min(const Vec3& m1, const Vec3& m2) { return { Min(m1.x, m2.x), Min(m1.y, m2.y), Min(m1.z, m2.z) }; }

// @max
inline i32 Max(i32 v1, i32 v2) { return v1 > v2 ? v1 : v2; }
inline u32 Max(u32 v1, u32 v2) { return v1 > v2 ? v1 : v2; }
inline u64 Max(u64 v1, u64 v2) { return v1 > v2 ? v1 : v2; }
inline Vec2 Max(const Vec2& m1, const Vec2& m2) { return { Max(m1.x, m2.x), Max(m1.y, m2.y) }; }
inline Vec3 Max(const Vec3& m1, const Vec3& m2) { return { Max(m1.x, m2.x), Max(m1.y, m2.y), Max(m1.z, m2.z) }; }

// @abs
inline i32 Abs(i32 v) { return v < 0 ? -v : v; }
inline f32 Abs(f32 v) { return v < 0 ? -v : v; }
inline f64 Abs(f64 v) { return v < 0 ? -v : v; }

// @bounds3
Bounds3 ToBounds(const Vec3* positions, u32 count);
bool Contains(const Bounds3& bounds, const Vec3& point);
bool Intersects(const Bounds3& bounds, const Bounds3& point);
Bounds3 Expand(const Bounds3& bounds, const Vec3& point);
Bounds3 Expand(const Bounds3& bounds, const Bounds3& other);

inline Bounds2 ToBounds2(const Bounds3& bounds) {
    return Bounds2{ Vec2{ bounds.min.x, bounds.min.y }, Vec2{ bounds.max.x, bounds.max.y } };
}

// @mat4
extern Mat4 TRS(const Vec3& translation, const Vec4& rotation, const Vec3& scale);
extern Mat4 Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
extern Mat4 Ortho(f32 top, f32 bottom, f32 near, f32 far);
extern Mat4 Inverse(const Mat4& m);
extern Mat3 ToMat3(const Mat4& m);

// @mat3
extern Mat3 TRS(const Vec2& translation, f32 rotation, const Vec2& scale);
extern Mat3 TRS(const Vec2& translation, const Vec2& direction, const Vec2& scale);
extern Mat3 Translate(const Vec2& translation);
extern Mat3 Rotate(float rotation);
extern Mat3 Rotate(const Vec2& direction);
extern Mat3 Scale(float scale);
extern Mat3 Scale(const Vec2& scale);
extern Mat3 Inverse(const Mat3& m);
extern Vec2 TransformPoint(const Mat3& m, const Vec2& point);
inline Vec2 TransformPoint(const Mat3& m) { return TransformPoint(m, VEC2_ZERO); }
extern Vec2 TransformVector(const Mat3& m, const Vec2& vector);
extern Vec2 GetForward(const Mat3& m);
extern Vec2 GetRight(const Mat3& m);

// @vec2
inline Vec3 XZ(const Vec2& v) { return {v.x, 0.0f, v.y}; }
inline Vec3 XY(const Vec2& v) { return {v.x, v.y, 0.0f}; }
inline Vec2 operator+(const Vec2& v1, const Vec2& v2) { return Vec2{ v1.x + v2.x, v1.y + v2.y }; }

inline Vec2 operator-(const Vec2& v1, const Vec2& v2) { return Vec2{ v1.x - v2.x, v1.y - v2.y }; }
inline Vec2 operator*(const Vec2& v, f32 s) { return Vec2{ v.x * s, v.y * s }; }
inline Vec2 operator*(const Vec2& v1, const Vec2& v2) { return Vec2{ v1.x * v2.x, v1.y * v2.y }; }
inline Vec2 operator/(const Vec2& v, f32 s) { return Vec2{ v.x / s, v.y / s }; }
inline Vec2 operator+=(Vec2& v1, const Vec2& v2) { v1.x += v2.x; v1.y += v2.y; return v1; }
inline Vec2 operator-=(Vec2& v1, const Vec2& v2) { v1.x -= v2.x; v1.y -= v2.y; return v1; }
inline Vec2 operator*=(Vec2& v, f32 s)  { v.x *= s; v.y *= s; return v; }
inline Vec2 operator*=(Vec2& v1, const Vec2& v2) { v1.x *= v2.x; v1.y *= v2.y; return v1; }
inline Vec2 operator/=(Vec2& v, f32 s)  { v.x /= s; v.y /= s; return v; }
inline Vec2 operator/=(Vec2& v1, const Vec2& v2) { v1.x /= v2.x; v1.y /= v2.y; return v1; }
inline Vec2 operator-(const Vec2& v) { return { -v.x, -v.y }; }
inline bool operator==(const Vec2& v1, const Vec2& v2) { return v1.x == v2.x && v1.y == v2.y; }
inline bool operator!=(const Vec2& v1, const Vec2& v2) { return v1.x != v2.x || v1.y != v2.y; }

extern f32 Length(const Vec2& v);
extern Vec2 Reflect(const Vec2& v, const Vec2& normal);
extern Vec2 Normalize(const Vec2& v);
extern Vec2 Rotate(const Vec2& v, f32 degrees);
extern Vec2 Rotate(const Vec2& v, const Vec2& direction);
extern float DistanceFromLine(const Vec2& v0, const Vec2& v1, const Vec2& point);

inline f32 LengthSqr(const Vec2& v) { return v.x * v.x + v.y * v.y; }
inline f32 Distance(const Vec2& v1, const Vec2& v2) { return Length(v2 - v1); }
inline f32 DistanceSqr(const Vec2& v1, const Vec2& v2) { return LengthSqr(v2 - v1); }
inline Vec2 Perpendicular(const Vec2& v) { return Vec2{ -v.y, v.x }; }
inline f32 Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline Vec2 Mix(const Vec2& v1, const Vec2& v2, f32 t) { return v1 + (v2 - v1) * t; }
inline Vec2 Abs(const Vec2& v) { return Vec2{ Abs(v.x), Abs(v.y) }; }
inline bool ApproxEqual(const Vec2& a, const Vec2& b, f32 epsilon = 1e-6f) {
    return ApproxEqual(a.x, b.x, epsilon) && ApproxEqual(a.y, b.y, epsilon);
}

// @vec4
inline Vec4 operator+(const Vec4& v1, const Vec4& v2) { return Vec4{ v1.x + v2.x, v1.y + v2.y, v1.z + v2.y, v1.w + v2.w }; }
inline Vec4 operator-(const Vec4& v1, const Vec4& v2) { return Vec4{ v1.x - v2.x, v1.y - v2.y, v1.z - v2.y, v1.w - v2.w }; }
inline Vec4 operator-(const Vec4& v1, f32 scalar) { return Vec4{ v1.x - scalar, v1.y - scalar, v1.z - scalar, v1.w - scalar }; }
inline Vec4 operator*(const Vec4& v, f32 scalar) { return Vec4{ v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar }; }
inline Vec4 operator*(const Vec4& v1, const Vec4& v2) { return Vec4{ v1.x * v2.x, v1.y * v2.y, v1.z * v2.z, v1.w * v2.w }; }
inline Vec4 operator/=(Vec4& v, f32 scalar) { v.x /= scalar; v.y /= scalar; v.z /= scalar; v.w /= scalar; return v; }

inline Vec4 Mix(const Vec4& v1, const Vec4& v2, f32 t) { return v1 + (v2 - v1) * t; }
inline Vec4 Floor(const Vec4& v) { return { Floor(v.x), Floor(v.y), Floor(v.z), Floor(v.w) }; }


// @vec2d
extern f64 Length(const Vec2Double& v);
extern Vec2Double Normalize(const Vec2Double& v);
inline f64 Dot(const Vec2Double& a, const Vec2Double& b) { return a.x * b.x + a.y * b.y; }
inline Vec2Double Mix(const Vec2Double& v1, const Vec2Double& v2, f64 t) { return v1 + (v2 - v1) * t; }

// @vec2int
inline Vec2 ToVec2(const Vec2Int& v) { return { (f32)v.x, (f32)v.y }; }
inline Vec2 ToVec2(const Vec3& v) { return { (f32)v.x, (f32)v.y }; }

// @vec3
inline Vec3 operator+(const Vec3& v1, const Vec3& v2) { return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z }; }
inline Vec3 operator-(const Vec3& v1, const Vec3& v2) { return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z }; }
inline Vec3 operator*(const Vec3& v, f32 s) { return { v.x * s, v.y * s, v.z * s }; }
inline Vec3 operator/(const Vec3& v, f32 s) { return { v.x / s, v.y / s, v.z / s }; }
inline Vec3 operator-(const Vec3& v) { return { -v.x, -v.y, -v.z }; }
inline Vec3 operator+=(Vec3& v, const Vec3& v2) { v.x += v2.x; v.y += v2.y; v.z += v2.z; return v; }
inline Vec3 operator*=(Vec3& v, f32 s) { v.x *= s; v.y *= s; v.z *= s; return v; }
inline Vec3 operator-=(Vec3& v, const Vec3& v2) { v.x -= v2.x; v.y -= v2.y; v.z -= v2.z; return v; }
extern f32 Length(const Vec3& v);
extern Vec3 Normalize(const Vec3& v);
extern Vec3 Cross(const Vec3& a, const Vec3& b);
inline f32 LengthSqr(const Vec3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
inline Vec3 ToVec3(const Vec2& a) { return Vec3{ a.x, a.y, 0.0f }; }
inline Vec3 ToVec3(const Vec2& a, float z) { return Vec3{ a.x, a.y, z }; }
inline f32 DistanceSqr(const Vec3& v1, const Vec3& v2) { return LengthSqr(v2 - v1); }
inline f32 Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec2 XY(const Vec3& v) { return {v.x, v.y}; }
inline Vec2 XZ(const Vec3& v) { return {v.x, v.z}; }

// @bounds2
extern Bounds2 ToBounds(const Vec2* positions, u32 count);
extern Bounds2 Intersection(const Bounds2& a, const Bounds2& b);
extern bool Contains(const Bounds2& bounds, const Vec2& point);
extern bool Intersects(const Bounds2& bounds, const Bounds2& other);
extern bool Intersects(const Bounds2& bounds, const Vec2& line_start, const Vec2& line_end);
extern bool Intersects(const Bounds2& bounds, const Vec2& tri_pt0, const Vec2& tri_pt1, const Vec2& tri_pt2);
extern Bounds2 Union(const Bounds2& a, const Bounds2& b);
inline Bounds2 Union(const Bounds2& a, const Vec2& b) { return Bounds2{ Min(a.min, b), Max(a.max, b) }; }
inline Vec2 GetCenter(const Bounds2& b) { return Vec2{ (b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f }; }
inline Vec2 GetSize(const Bounds2& b) { return Vec2{ b.max.x - b.min.x, b.max.y - b.min.y }; }
inline Bounds2 operator+ (const Bounds2& b, const Vec2& offset) { return { b.min + offset, b.max + offset }; }
inline Bounds2 operator- (const Bounds2& b, const Vec2& offset) { return { b.min - offset, b.max - offset }; }
inline Bounds2 Expand(const Bounds2& b, float size) { return Bounds2{ b.min - Vec2{ size, size }, b.max + Vec2{ size, size } }; }
inline Bounds2 Translate(const Bounds2& b, const Vec2& translation) { return Bounds2{ b.min + translation, b.max + translation }; }

// @angle
extern float MixAngle(float a, float b, float t);
extern float AngleDelta(const Vec2& a, const Vec2&b);
extern float AngleDelta(float a, float b);
extern float NormalizeAngle(float angle);
extern float NormalizeAngle180(float angle);
inline float Radians(float degrees) { return degrees * noz::PI / 180.0f; }
inline float Degrees(float radians) { return radians * 180.0f / noz::PI; }
inline float Angle(const Vec2& direction) { return Degrees(Atan2(direction.y, direction.x)); }

// @transform
extern void SetIdentity(Transform& transform);
extern void Update(Transform& transform);
extern void SetPosition(Transform& transform, const Vec2& position);
extern void SetPosition(Transform& transform, float x, float y);
extern void SetRotation(Transform& transform, f32 rotation);
extern void SetRotation(Transform& transform, const Vec2& direction);
extern void SetScale(Transform& transform, const Vec2& scale);
extern void SetScale(Transform& transform, float scale);
extern const Mat3& GetLocalToWorld(Transform& transform);
extern const Mat3& GetWorldToLocal(Transform& transform);
extern Vec2 TransformPoint(Transform& transform, const Vec2& point);
extern Vec2 TransformPoint(Transform& transform);
extern Vec2 InverseTransformPoint(Transform& transform, const Vec2& point);
extern Vec2 TransformVector(Transform& transform, const Vec2& vector);
extern Vec2 InverseTransformVector(Transform& transform, const Vec2& vector);
extern Transform Mix(const Transform& t1, const Transform& t2, f32 t);

inline i32 Clamp(i32 v, i32 min, i32 max) { return v < min ? min : v > max ? max : v; }
inline u32 Clamp(u32 v, u32 min, u32 max) { return v < min ? min : v > max ? max : v; }
inline f32 Clamp(f32 v, f32 min, f32 max) { return v < min ? min : v > max ? max : v; }
inline f64 Clamp(f64 v, f64 min, f64 max) { return v < min ? min : v > max ? max : v; }
inline f32 Clamp01(f32 v) { return Clamp(v, 0.0f, 1.0f); }

constexpr Vec2Int RoundToNearest(const Vec2Int& v) {
    return { (i32)(v.x + 0.5f), (i32)(v.y + 0.5f) };
}

constexpr Vec2Int RoundToNearest(const Vec2Double& v) {
    return { (i32)(v.x + 0.5f), (i32)(v.y + 0.5f) };
}

constexpr u32 FourCC(u8 a, u8 b, u8 c, u8 d) {
    return (u32)(d) | ((u32)(c) << 8) | ((u32)(b) << 16) | ((u32)(a) << 24);
}

// @easing
inline float EaseQuadratic(float t) { return t * t; }
inline float EaseCubic(float t) { return t * t * t; }
inline float EaseOut(float t, const std::function<float (float)>& func) { return 1.0f - func(1.0f - t); }
inline float EaseOutQuadratic(float t) { return EaseOut(t, EaseQuadratic); }

extern float PerlinNoise(const Vec2& Position);


// @smooth
extern float SmoothDamp(float current, float target, float& current_velocity, float smooth_time, float max_speed, float delta_time);
extern Vec2 SmoothDamp(const Vec2& current, const Vec2& target, Vec2& current_velocity, float smooth_time, float max_speed, float delta_time);