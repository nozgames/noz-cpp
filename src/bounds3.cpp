//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

bounds3 to_bounds(const vec3* positions, size_t count)
{
    if (count == 0)
        return { VEC3_ZERO, VEC3_ZERO };

    vec3 min_pos = positions[0];
    vec3 max_pos = positions[0];

    for (size_t i = 1; i < count; i++)
    {
        min_pos = glm::min(min_pos, positions[i]);
        max_pos = glm::max(max_pos, positions[i]);
    }

    vec3 center = (min_pos + max_pos) * 0.5f;
    vec3 extents = (max_pos - min_pos) * 0.5f;
    return bounds3(center, extents);
}

bool contains(const bounds3& bounds, const vec3& point)
{
    const vec3& min = bounds.min;
    const vec3& max = bounds.max;
    return point.x >= min.x && point.x <= max.x &&
        point.y >= min.y && point.y <= max.y &&
        point.z >= min.z && point.z <= max.z;
}

bool intersects(const bounds3& bounds, const bounds3& other)
{
    const vec3& min = bounds.min;
    const vec3& max = bounds.max;
    return min.x <= other.max.x && max.x >= other.min.x &&
        min.y <= other.max.y && max.y >= other.min.y &&
        min.z <= other.max.z && max.z >= other.min.z;
}

bounds3 expand(const bounds3& bounds, const vec3& point)
{
    vec3 min = bounds.min;
    vec3 max = bounds.max;
    min = glm::min(min, point);
    max = glm::max(max, point);
    return { min, max };
}

bounds3 expand(const bounds3& bounds, const bounds3& other)
{
    vec3 min = bounds.min;
    vec3 max = bounds.max;
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
    return { min, max };
}

