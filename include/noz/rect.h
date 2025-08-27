//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct rect_t
{
    float x;
    float y;
    float width;
    float height;
};

inline float GetLeft(const rect_t& rect) { return rect.x; }
inline float GetTop(const rect_t& rect) { return rect.y; }
inline float GetBottom(const rect_t& rect) { return rect.y + rect.height; }
inline float GetRight(const rect_t& rect) { return rect.x + rect.width; }

inline void SetLeft(rect_t& rect, float value) { rect.x = value; }
inline void SetTop(rect_t& rect, float value) { rect.y = value; }
inline void SetBottom(rect_t& rect, float value) { rect.height = value - rect.y; }
inline void SetRight(rect_t& rect, float value) { rect.width = value - rect.x; }

inline bool Contains(const rect_t& rect, float x, float y);
inline bool Intersects(const rect_t& rect, const rect_t& other);
inline rect_t Intersection(const rect_t& rect, const rect_t& other);
