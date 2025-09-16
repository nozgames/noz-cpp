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
