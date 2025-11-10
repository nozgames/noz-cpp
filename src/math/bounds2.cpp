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

    return Bounds2(min_pos, max_pos);
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

bool Intersects(const Bounds2& bounds, const Vec2& line_start, const Vec2& line_end)
{
    // Check if either end of the line is contained within the bounds
    if (Contains(bounds, line_start) || Contains(bounds, line_end))
        return true;

    // Define the corners of the bounds
    Vec2 bl = bounds.min;               // Bottom-left
    Vec2 br = { bounds.max.x, bounds.min.y }; // Bottom-right
    Vec2 tr = bounds.max;               // Top-right
    Vec2 tl = { bounds.min.x, bounds.max.y }; // Top-left

    // Check for intersection with each edge of the bounds
    if (OverlapLine(line_start, line_end, bl, br, nullptr)) return true; // Bottom edge
    if (OverlapLine(line_start, line_end, br, tr, nullptr)) return true; // Right edge
    if (OverlapLine(line_start, line_end, tr, tl, nullptr)) return true; // Top edge
    if (OverlapLine(line_start, line_end, tl, bl, nullptr)) return true; // Left edge

    return false;
}

bool Intersects(const Bounds2& bounds, const Vec2& tri_pt0, const Vec2& tri_pt1, const Vec2& tri_pt2)
{
    // Check if any of the triangle's points are inside the bounds
    if (Contains(bounds, tri_pt0) || Contains(bounds, tri_pt1) || Contains(bounds, tri_pt2))
        return true;

    // Define the corners of the bounds
    Vec2 bl = bounds.min;               // Bottom-left
    Vec2 br = { bounds.max.x, bounds.min.y }; // Bottom-right
    Vec2 tr = bounds.max;               // Top-right
    Vec2 tl = { bounds.min.x, bounds.max.y }; // Top-left

    // Check for intersection between triangle edges and bounds edges
    if (OverlapLine(tri_pt0, tri_pt1, bl, br, nullptr)) return true; // Triangle edge 0-1 with bottom edge
    if (OverlapLine(tri_pt1, tri_pt2, bl, br, nullptr)) return true; // Triangle edge 1-2 with bottom edge
    if (OverlapLine(tri_pt2, tri_pt0, bl, br, nullptr)) return true; // Triangle edge 2-0 with bottom edge

    if (OverlapLine(tri_pt0, tri_pt1, br, tr, nullptr)) return true; // Triangle edge 0-1 with right edge
    if (OverlapLine(tri_pt1, tri_pt2, br, tr, nullptr)) return true; // Triangle edge 1-2 with right edge
    if (OverlapLine(tri_pt2, tri_pt0, br, tr, nullptr)) return true; // Triangle edge 2-0 with right edge

    if (OverlapLine(tri_pt0, tri_pt1, tr, tl, nullptr)) return true; // Triangle edge 0-1 with top edge
    if (OverlapLine(tri_pt1, tri_pt2, tr, tl, nullptr)) return true; // Triangle edge 1-2 with top edge
    if (OverlapLine(tri_pt2, tri_pt0, tr, tl, nullptr)) return true; // Triangle edge 2-0 with top edge

    if (OverlapLine(tri_pt0, tri_pt1, tl, bl, nullptr)) return true; // Triangle edge 0-1 with left edge
    if (OverlapLine(tri_pt1, tri_pt2, tl, bl, nullptr)) return true; // Triangle edge 1-2 with left edge
    if (OverlapLine(tri_pt2, tri_pt0, tl, bl, nullptr)) return true; // Triangle edge 2-0 with left edge

    return false;
}

Bounds2 Union(const Bounds2& a, const Bounds2& b) {
    return { Min(a.min, b.min), Max(a.max, b.max) };
}

extern Bounds2 Intersection(const Bounds2& a, const Bounds2& b) {
    Vec2 new_min = Max(a.min, b.min);
    Vec2 new_max = Min(a.max, b.max);

    // If there is no intersection, return an empty bounds
    if (new_min.x > new_max.x || new_min.y > new_max.y) {
        return { VEC2_ZERO, VEC2_ZERO };
    }

    return { new_min, new_max };
}
