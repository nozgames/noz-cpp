//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

bool Contains(const Rect& rect, float px, float py)
{
    return px >= rect.x && px <= rect.x + rect.width && py >= rect.y && py <= rect.y + rect.height;
}

bool Intersects(const Rect& rect, const Rect& other)
{
    return !(other.x > rect.x + rect.width || other.x + other.width < rect.x ||
             other.y > rect.y + rect.height || other.y + other.height < rect.y);
}

Rect Intersection(const Rect& rect, const Rect& other)
{
    float left = max(rect.x, other.x);
    float top = max(rect.y, other.y);
    float right = min(rect.x + rect.width, other.x + other.width);
    float bottom = min(rect.y + rect.height, other.y + other.height);

    if (left < right && top < bottom)
        return {left, top, right - left, bottom - top };

    return {};
}
