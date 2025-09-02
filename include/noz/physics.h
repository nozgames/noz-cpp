//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @components
struct Collider : Object {};
struct RigidBody : Object{};

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

// @rigid_body
RigidBody* CreateRigidBody(Allocator* allocator);

// @collider
Collider* CreateBoxCollider(Allocator* allocator, const Vec2& center, const Vec2& size, float density, float friction, float restitution, bool is_sensor);
Collider* CreateCircleCollider(Allocator* allocator, const Vec2& center, float radius, float density, float friction, float restitution, bool is_sensor);
