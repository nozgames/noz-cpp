//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

bool OverlapPoint(
    const Vec2& v0,
    const Vec2& v1,
    const Vec2& v2,
    const Vec2& overlap_point,
    Vec2* where)
{
    // Calculate the area using cross product (can be negative if clockwise)
    float area = (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y);

    // Handle degenerate triangles (zero area)
    if (fabsf(area) < 1e-6f)
        return false;

    // Calculate barycentric coordinates
    float inv_area = 1.0f / area;
    float s = ((v2.y - v0.y) * (overlap_point.x - v0.x) + (v0.x - v2.x) * (overlap_point.y - v0.y)) * inv_area;
    float t = ((v0.y - v1.y) * (overlap_point.x - v0.x) + (v1.x - v0.x) * (overlap_point.y - v0.y)) * inv_area;

    if (s >= 0 && t >= 0 && (s + t) <= 1)
    {
        if (where)
            *where = {s, t};

        return true;
    }

    return false;
}

bool OverlapLine(const Vec2& l0v0, const Vec2& l0v1, const Vec2& l1v0, const Vec2& l1v1, Vec2* where) {
    Vec2 d0 = l0v1 - l0v0;
    Vec2 d1 = l1v1 - l1v0;

    float cross = d0.x * d1.y - d0.y * d1.x;
    if (Abs(cross) < F32_EPSILON)
        return false;

    Vec2 delta = l1v0 - l0v0;
    float t = (delta.x * d1.y - delta.y * d1.x) / cross;
    float u = (delta.x * d0.y - delta.y * d0.x) / cross;

    if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f)
    {
        if (where)
            *where = l0v0 + d0 * t;

        return true;
    }

    return false;
}

