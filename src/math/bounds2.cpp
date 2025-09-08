//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

Bounds2 ToBounds(const Vec2* positions, u32 count)
{
    if (count == 0)
        return { VEC2_ZERO, VEC2_ZERO };

    Vec2 min_pos = positions[0];
    Vec2 max_pos = positions[0];

    for (size_t i = 1; i < count; i++)
    {
        min_pos = Min(min_pos, positions[i]);
        max_pos = Max(max_pos, positions[i]);
    }

    Vec2 center = (min_pos + max_pos) * 0.5f;
    Vec2 extents = (max_pos - min_pos) * 0.5f;
    return Bounds2(center, extents);
}

bool Contains(const Bounds2& bounds, const Vec2& point)
{
    return point.x >= bounds.min.x && point.x <= bounds.max.x &&
           point.y >= bounds.min.y && point.y <= bounds.max.y;
}

bool Intersects(const Bounds2& bounds, const Bounds2& other)
{
    const Vec2& min = bounds.min;
    const Vec2& max = bounds.max;
    return min.x <= other.max.x && max.x >= other.min.x &&
        min.y <= other.max.y && max.y >= other.min.y;
}

Bounds2 Union(const Bounds2& a, const Bounds2& b)
{
    return { Min(a.min, b.min), Max(a.max, b.max) };
}
