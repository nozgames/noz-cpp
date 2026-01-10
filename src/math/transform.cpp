//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz {

    void SetIdentity(Transform& transform)
    {
        transform.position = VEC2_ZERO;
        transform.scale = VEC2_ONE;
        transform.rotation = 0.0f;
        transform.flags = TRANSFORM_FLAG_DIRTY;
    }

    inline void UpdateIfDirty(Transform& transform)
    {
        if ((transform.flags & TRANSFORM_FLAG_DIRTY) == 0) return;
        transform.local_to_world = TRS(transform.position, transform.rotation, transform.scale);
        transform.world_to_local = Inverse(transform.local_to_world);
    }

    const Mat3& GetLocalToWorld(Transform& transform)
    {
        UpdateIfDirty(transform);
        return transform.local_to_world;
    }

    const Mat3& GetWorldToLocal(Transform& transform)
    {
        UpdateIfDirty(transform);
        return transform.world_to_local;
    }

    void SetPosition(Transform& transform, float x, float y)
    {
        transform.position = {x,y};
        transform.flags |= TRANSFORM_FLAG_DIRTY;
    }

    void SetPosition(Transform& transform, const Vec2& position)
    {
        transform.position = position;
        transform.flags |= TRANSFORM_FLAG_DIRTY;
    }

    void SetRotation(Transform& transform, const Vec2& direction)
    {
        transform.rotation = Degrees(atan2f(direction.y, direction.x));
        transform.flags |= TRANSFORM_FLAG_DIRTY;
    }

    void SetRotation(Transform& transform, f32 rotation)
    {
        transform.rotation = rotation;
        transform.flags |= TRANSFORM_FLAG_DIRTY;
    }

    void SetScale(Transform& transform, float scale)
    {
        transform.scale = {scale,scale};
        transform.flags |= TRANSFORM_FLAG_DIRTY;
    }

    void SetScale(Transform& transform, const Vec2& scale)
    {
        transform.scale = scale;
        transform.flags |= TRANSFORM_FLAG_DIRTY;
    }

    Vec2 TransformPoint(Transform& transform, const Vec2& point)
    {
        UpdateIfDirty(transform);
        return transform.local_to_world * point;
    }

    Vec2 TransformPoint(Transform& transform)
    {
        UpdateIfDirty(transform);
        return transform.local_to_world * VEC2_ZERO;
    }

    Vec2 InverseTransformPoint(Transform& transform, const Vec2& point)
    {
        UpdateIfDirty(transform);
        return transform.world_to_local * point;
    }

    Vec2 TransformVector(Transform& transform, const Vec2& vector)
    {
        UpdateIfDirty(transform);
        return (Mat3{
            transform.local_to_world.m[0], transform.local_to_world.m[1], 0,
            transform.local_to_world.m[3], transform.local_to_world.m[4], 0,
            0, 0, 1
        }) * vector;
    }

    Vec2 InverseTransformVector(Transform& transform, const Vec2& vector)
    {
        UpdateIfDirty(transform);
        return (Mat3{
            transform.world_to_local.m[0], transform.world_to_local.m[1], 0,
            transform.world_to_local.m[3], transform.world_to_local.m[4], 0,
            0, 0, 1
        }) * vector;
    }

    Transform Mix(const Transform& t1, const Transform& t2, f32 t) {
        return {
            .position = Mix(t1.position, t2.position, t),
            .scale = Mix(t1.scale, t2.scale, t),
            .rotation = t1.rotation + NormalizeAngle180(t2.rotation - t1.rotation) * t,
            .flags = TRANSFORM_FLAG_DIRTY
        };
    }

    void Update(Transform& transform) {
        transform.flags = TRANSFORM_FLAG_DIRTY;
        UpdateIfDirty(transform);
    }
}
