//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

bool Contains(const Rect& rect, float px, float py)
{
    return px >= rect.x && px <= rect.x + rect.width && py >= rect.y && py <= rect.y + rect.height;
}


Rect Intersection(const Rect& rect, const Rect& other)
{
    f32 left = Max(rect.x, other.x);
    f32 top = Max(rect.y, other.y);
    f32 right = Min(rect.x + rect.width, other.x + other.width);
    f32 bottom = Min(rect.y + rect.height, other.y + other.height);

    if (left < right && top < bottom)
        return {left, top, right - left, bottom - top };

    return {};
}
