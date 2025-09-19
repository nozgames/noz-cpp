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
    bool hit;
    Vec2 point;
    Vec2 normal;
    float fraction;
    void* user_data;
};

// @physics
RaycastResult Raycast(const Vec2& start, const Vec2& end, uint16_t category_mask = 0xFFFF);

// @collider
Collider* CreateCollider(Allocator* allocator, const Vec2* points, u32 point_count);
Collider* CreateCollider(Allocator* allocator, Mesh* mesh);
bool OverlapPoint(Collider* collider, const Vec2& point);

// @collision
bool OverlapPoint(const Vec2& v0, const Vec2& v1, const Vec2& v2, const Vec2& overlap_point, Vec2* where);
bool OverlapLine(const Vec2& l0v0, const Vec2& l0v1, const Vec2& l1v0, const Vec2& l1v1, Vec2* where);
