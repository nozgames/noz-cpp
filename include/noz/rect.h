//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Rect
{
    f32 x;
    f32 y;
    f32 width;
    f32 height;
};

struct RectInt
{
    i32 x;
    i32 y;
    i32 width;
    i32 height;
};

inline f32 GetLeft(const Rect& rect) { return rect.x; }
inline f32 GetTop(const Rect& rect) { return rect.y; }
inline f32 GetBottom(const Rect& rect) { return rect.y + rect.height; }
inline f32 GetRight(const Rect& rect) { return rect.x + rect.width; }

inline void SetLeft(Rect& rect, f32 value) { rect.x = value; }
inline void SetTop(Rect& rect, f32 value) { rect.y = value; }
inline void SetBottom(Rect& rect, f32 value) { rect.height = value - rect.y; }
inline void SetRight(Rect& rect, f32 value) { rect.width = value - rect.x; }

inline bool Contains(const Rect& rect, f32 x, f32 y);
inline bool Intersects(const Rect& rect, const Rect& other);
inline Rect Intersection(const Rect& rect, const Rect& other);

inline i32 GetRight(const RectInt& rect) { return rect.x + rect.width; }
inline i32 GetBottom(const RectInt& rect) { return rect.y + rect.height; }
