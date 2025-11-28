//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct ColliderImpl : Collider {
    Vec2* points;
    u32 point_count;
    Bounds2 bounds;
};

static void ColliderDestructor(void *ptr) {
    ColliderImpl* impl = (ColliderImpl*)ptr;
    Free(impl->points);
}

Collider* CreateCollider(Allocator* allocator, const Bounds2& bounds) {
    ColliderImpl* impl = (ColliderImpl*)Alloc(allocator, sizeof(ColliderImpl), ColliderDestructor);
    impl->point_count = 4;
    impl->points = static_cast<Vec2*>(Alloc(allocator, sizeof(Vec2) * impl->point_count));
    impl->bounds = bounds;
    impl->points[0] = Vec2{bounds.min.x, bounds.min.y};
    impl->points[1] = Vec2{bounds.max.x, bounds.min.y};
    impl->points[2] = Vec2{bounds.max.x, bounds.max.y};
    impl->points[3] = Vec2{bounds.min.x, bounds.max.y};
    return impl;
}

Collider* CreateCollider(Allocator* allocator, const Vec2* points, u32 point_count) {
    ColliderImpl* impl = (ColliderImpl*)Alloc(allocator, sizeof(ColliderImpl), ColliderDestructor);
    impl->point_count = point_count;
    impl->points = (Vec2*)Alloc(allocator, sizeof(Vec2) * point_count);
    impl->bounds = ToBounds(points, point_count);
    memcpy(impl->points, points, sizeof(Vec2) * point_count);

    return impl;
}

Collider* CreateCollider(Allocator* allocator, Mesh* mesh)
{
    ColliderImpl* impl = (ColliderImpl*)Alloc(allocator, sizeof(ColliderImpl), ColliderDestructor);
    impl->point_count = GetVertexCount(mesh);
    impl->points = (Vec2*)Alloc(allocator, sizeof(Vec2) * impl->point_count);
    impl->bounds = GetBounds(mesh);
    for (u32 i=0; i < impl->point_count; i++)
        impl->points[i] = XY(GetVertices(mesh)[i].position);

    return impl;
}

bool OverlapPoint(Collider* collider, const Mat3& transform, const Vec2& point) {
    ColliderImpl* impl = (ColliderImpl*)collider;
    Vec2 v1 = TransformPoint(transform, impl->points[impl->point_count - 1]);

    for (u32 i = 0; i < impl->point_count; i++) {
        Vec2 v2 = TransformPoint(transform, impl->points[i]);
        Vec2 edge = v2 - v1;
        Vec2 to_point = point - v1;
        f32 cross = edge.x * to_point.y - edge.y * to_point.x;
        if (cross < 0)
            return false;

        v1 = v2;
    }

    return true;
}

bool OverlapBounds(Collider* collider, const Mat3& transform, const Bounds2& bounds) {
    ColliderImpl* impl = (ColliderImpl*)collider;

    // Check if any of the bounds corners are inside the collider
    Vec2 corners[4] = {
        {bounds.min.x, bounds.min.y},
        {bounds.max.x, bounds.min.y},
        {bounds.max.x, bounds.max.y},
        {bounds.min.x, bounds.max.y}
    };

    for (u32 i = 0; i < 4; i++) {
        if (OverlapPoint(collider, transform, corners[i]))
            return true;
    }

    // check if any collider edges overlap the bounds
    Vec2 v1 = TransformPoint(transform, impl->points[impl->point_count - 1]);
    for (u32 i = 0; i < impl->point_count; i++) {
        Vec2 v2 = TransformPoint(transform, impl->points[i]);
        if (Intersects(bounds, v1, v2))
            return true;

        v1 = v2;
    }

    return false;
}

Bounds2 GetBounds(Collider* collider) {
    return static_cast<ColliderImpl*>(collider)->bounds;
}

bool Raycast(Collider* colider, const Mat3& transform, const Vec2& p0, const Vec2& p1, RaycastResult* result) {
    return Raycast(colider, transform, p0, Normalize(p1 - p0), Length(p1 - p0), result);
}

bool Raycast(Collider* colider, const Mat3& transform, const Vec2& origin, const Vec2& dir, float distance, RaycastResult* result) {
    const ColliderImpl* impl = static_cast<ColliderImpl*>(colider);

    Vec2 ray_end = origin + dir * distance;

    RaycastResult best_result = {};
    best_result.fraction = 1.0f;

    Vec2 v1 = TransformPoint(transform, impl->points[impl->point_count - 1]);
    for (u32 i = 0; i < impl->point_count; i++) {
        Vec2 v2 = TransformPoint(transform, impl->points[i]);

        Vec2 where;
        if (OverlapLine(origin, ray_end, v1, v2, &where)) {
            float overlap_distance = Distance(where, origin);
            float fraction = overlap_distance / distance;
            if (fraction < best_result.fraction) {
                best_result.point = where;
                best_result.fraction = fraction;
                best_result.distance = overlap_distance;

                Vec2 edge = v2 - v1;
                best_result.normal = Normalize(Vec2{-edge.y, edge.x});
            }
        }

        v1 = v2;
    }

    if (result) {
        *result = best_result;
    }

    return best_result.fraction < 1.0f;
}
