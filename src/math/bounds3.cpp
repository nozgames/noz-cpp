//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

Bounds3 ToBounds(const Vec3* positions, u32 count) {
    if (count == 0)
        return { VEC3_ZERO, VEC3_ZERO };

    Vec3 min_pos = positions[0];
    Vec3 max_pos = positions[0];

    for (size_t i = 1; i < count; i++)
    {
        min_pos = Min(min_pos, positions[i]);
        max_pos = Max(max_pos, positions[i]);
    }

    return Bounds3(min_pos, max_pos);
}

bool Contains(const Bounds3& bounds, const Vec3& point)
{
    const Vec3& min = bounds.min;
    const Vec3& max = bounds.max;
    return point.x >= min.x && point.x <= max.x &&
        point.y >= min.y && point.y <= max.y &&
        point.z >= min.z && point.z <= max.z;
}

bool Intersects(const Bounds3& bounds, const Bounds3& other)
{
    const Vec3& min = bounds.min;
    const Vec3& max = bounds.max;
    return min.x <= other.max.x && max.x >= other.min.x &&
        min.y <= other.max.y && max.y >= other.min.y &&
        min.z <= other.max.z && max.z >= other.min.z;
}

Bounds3 Expand(const Bounds3& bounds, const Vec3& point)
{
    Vec3 min = bounds.min;
    Vec3 max = bounds.max;
    min = Min(min, point);
    max = Max(max, point);
    return { min, max };
}

Bounds3 Expand(const Bounds3& bounds, const Bounds3& other)
{
    Vec3 min = bounds.min;
    Vec3 max = bounds.max;
    min = Min(min, other.min);
    max = Max(max, other.max);
    return { min, max };
}

