//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @components
struct Collider {};
struct RigidBody {};

// @enums
enum RigidBodyType
{
    RIGID_BODY_TYPE_STATIC = 0,
    RIGID_BODY_TYPE_DYNAMIC = 1,
    RIGID_BODY_TYPE_KINEMATIC = 2
};

enum ColliderType
{
    COLLIDER_TYPE_BOX = 0,
    COLLIDER_TYPE_CIRCLE = 1
};

// @structs
struct RaycastResult
{
    Vec2 point;
    Vec2 normal;
    float fraction;
};

// @collider
extern Collider* CreateCollider(Allocator* allocator, const Vec2* points, u32 point_count);
extern Collider* CreateCollider(Allocator* allocator, Mesh* mesh);
extern Collider* CreateCollider(Allocator* allocator, const Bounds2& bounds);
extern bool OverlapPoint(Collider* collider, const Vec2& point, const Mat3& transform=MAT3_IDENTITY);
extern bool OverlapBounds(Collider* collider, const Bounds2& bounds, const Mat3& transform=MAT3_IDENTITY);
extern bool Raycast(Collider* colider, const Vec2& origin, const Vec2& dir, float distance, RaycastResult* result);
extern bool Raycast(Collider* colider, const Vec2& p0, const Vec2& p1, RaycastResult* result);
extern Bounds2 GetBounds(Collider* collider);

// @collision
bool OverlapPoint(const Vec2& v0, const Vec2& v1, const Vec2& v2, const Vec2& overlap_point, Vec2* where);
bool OverlapLine(const Vec2& l0v0, const Vec2& l0v1, const Vec2& l1v0, const Vec2& l1v1, Vec2* where);

