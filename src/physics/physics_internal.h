//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if 0

#pragma once

#define isfinite(x) ((x) == std::numeric_limits<float>::infinity())
#include <box2d/box2d.h>
#undef isfinite

// Box2D 3.x uses a global world and ID-based system
extern b2WorldId g_physics_world;

// Conversion functions for Box2D 3.x API
inline b2Vec2 ToBox2d(const Vec2& v) { return {v.x, v.y}; }
inline Vec2 FromBox2d(const b2Vec2& v) { return {v.x, v.y}; }

inline b2BodyType ToBox2d(RigidBodyType bt)
{
    switch (bt)
    {
    case RIGID_BODY_TYPE_STATIC:
        return b2_staticBody;
    case RIGID_BODY_TYPE_DYNAMIC:
        return b2_dynamicBody;
    case RIGID_BODY_TYPE_KINEMATIC:
        return b2_kinematicBody;
    default:
        return b2_dynamicBody;
    }
}

// Physics world management
b2WorldId GetPhysicsWorld();

// Rigid body management functions
b2BodyId GetRigidBodyId(RigidBody* rigid_body);
b2BodyId GetOrCreateRigidBodyId(RigidBody* rigid_body, Entity* entity);
void SetRigidBodyType(RigidBody* rigid_body, RigidBodyType type);

// Collider management functions
void CreateColliderShape(Collider* collider, Entity* entity);
void DestroyColliderShape(Collider* collider);
b2ShapeId GetColliderShapeId(Collider* collider);


#endif
