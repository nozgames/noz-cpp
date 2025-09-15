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

bool OverlapPoint(Collider* collider, const Vec2& point)
{
    ColliderImpl* impl = (ColliderImpl*)collider;
    if (!Contains(impl->bounds, point))
        return false;

    Vec2 v1 = impl->points[impl->point_count - 1];

    for (u32 i = 0; i < impl->point_count; i++)
    {
        Vec2 v2 = impl->points[i];
        Vec2 edge = v2 - v1;
        Vec2 to_point = point - v1;
        f32 cross = edge.x * to_point.y - edge.y * to_point.x;
        if (cross < 0)
            return false;

        v1 = v2;
    }

    return true;
}
