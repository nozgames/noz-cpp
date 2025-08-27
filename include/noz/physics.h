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

// @structs
struct RaycastResult
{
    bool hit;
    vec2 point;
    vec2 normal;
    float fraction;
    void* user_data;
};

// @physics
RaycastResult Raycast(const vec2& start, const vec2& end, uint16_t category_mask = 0xFFFF);

// @rigid_body
RigidBody* CreateRigidBody(Allocator* allocator);

// @collider
Collider* CreateBoxCollider(Allocator* allocator, RigidBody* rigid_body, const vec2& center, const vec2& size, float density, float friction, float restitution, bool is_sensor);
Collider* CreateCircleCollider(Allocator* allocator, RigidBody* rigid_body, const vec2& center, float radius, float density, float friction, float restitution, bool is_sensor);
