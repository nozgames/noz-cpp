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

// @collision
bool OverlapPoint(Collider* collider, const Vec2& point);
