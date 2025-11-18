//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

namespace noz {

    struct Rect {
        f32 x;
        f32 y;
        f32 width;
        f32 height;
    };

    struct RectInt {
        i32 x;
        i32 y;
        i32 width;
        i32 height;
    };

    inline f32 GetLeft(const Rect& rect) { return rect.x; }
    inline f32 GetTop(const Rect& rect) { return rect.y; }
    inline f32 GetBottom(const Rect& rect) { return rect.y + rect.height; }
    inline f32 GetRight(const Rect& rect) { return rect.x + rect.width; }
    inline Vec2 GetTopLeft(const Rect& rect) { return { rect.x, rect.y }; }
    inline Vec2 GetCenter(const Rect& rect) { return { rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f }; }
    inline Vec2 GetSize(const Rect& rect) { return { rect.width, rect.height }; }
    inline Rect Expand(const Rect& rect, float s) { return { rect.x - s, rect.y - s, rect.width + s * 2, rect.height + s * 2 }; }

    inline void SetLeft(Rect& rect, f32 value) { rect.x = value; }
    inline void SetTop(Rect& rect, f32 value) { rect.y = value; }
    inline void SetBottom(Rect& rect, f32 value) { rect.height = value - rect.y; }
    inline void SetRight(Rect& rect, f32 value) { rect.width = value - rect.x; }

    inline bool Contains(const Rect& rect, f32 x, f32 y) { return x >= rect.x && x <= rect.x + rect.width && y >= rect.y && y <= rect.y + rect.height; }
    inline bool Contains(const Rect& rect, const Vec2& pos) { return Contains(rect, pos.x, pos.y); }
    inline Rect Intersection(const Rect& rect, const Rect& other);

    inline i32 GetLeft(const RectInt& rect) { return rect.x; }
    inline i32 GetRight(const RectInt& rect) { return rect.x + rect.width; }
    inline i32 GetTop(const RectInt& rect) { return rect.y; }
    inline i32 GetBottom(const RectInt& rect) { return rect.y + rect.height; }

    inline bool Intersects(const Rect& rect, const Rect& other) {
        return !(other.x > rect.x + rect.width || other.x + other.width < rect.x ||
                 other.y > rect.y + rect.height || other.y + other.height < rect.y);
    }

}
