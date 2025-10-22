//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct ColliderImpl : Collider
{
    Vec2* points;
    u32 point_count;
    Bounds2 bounds;
};

static void ColliderDestructor(void *ptr)
{
    ColliderImpl* impl = (ColliderImpl*)ptr;
    Free(impl->points);
}

Collider* CreateCollider(Allocator* allocator, const Vec2* points, u32 point_count)
{
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
        impl->points[i] = GetVertices(mesh)[i].position;

    return impl;
}

bool OverlapPoint(Collider* collider, const Vec2& point, const Mat3& transform) {
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

bool OverlapBounds(Collider* collider, const Bounds2& bounds, const Mat3& transform) {
    ColliderImpl* impl = (ColliderImpl*)collider;

    // Check if any of the bounds corners are inside the collider
    Vec2 corners[4] = {
        {bounds.min.x, bounds.min.y},
        {bounds.max.x, bounds.min.y},
        {bounds.max.x, bounds.max.y},
        {bounds.min.x, bounds.max.y}
    };

    for (u32 i = 0; i < 4; i++) {
        if (OverlapPoint(collider, corners[i], transform))
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

bool Raycast(Collider* colider, const Vec2& origin, const Vec2& dir, float distance, RaycastResult* result)
{
    ColliderImpl* impl = (ColliderImpl*)colider;

    Vec2 ray_end = origin + dir * distance;

    result->fraction = 1.0f;

    Vec2 v1 = impl->points[impl->point_count - 1];
    for (u32 i = 0; i < impl->point_count; i++)
    {
        Vec2 v2 = impl->points[i];

        Vec2 where;
        if (OverlapLine(origin, ray_end, v1, v2, &where))
        {
            float fraction = Distance(where, origin) / distance;
            if (fraction < result->fraction)
            {
                result->point = where;
                result->fraction = fraction;

                Vec2 edge = v2 - v1;
                result->normal = Normalize(Vec2(-edge.y, edge.x));
            }
        }

        v1 = v2;
    }

    return result->fraction < 1.0f;
}
