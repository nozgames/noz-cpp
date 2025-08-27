//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#define isfinite(x) ((x) == std::numeric_limits<float>::infinity())
#include <box2d/box2d.h>
#undef isfinite

inline b2Vec2 ToBox2d(const vec2& v) { return {v.x, v.y}; }

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
