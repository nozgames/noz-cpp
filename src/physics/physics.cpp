//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "physics_internal.h"

// Global physics world for Box2D 3.x
b2WorldId g_physics_world = b2_nullWorldId;

struct Physics
{
    float accumulator;
};

static Physics g_physics = {};

void UpdatePhysics()
{
    assert(B2_IS_NON_NULL(g_physics_world));

    g_physics.accumulator += GetFrameTime();

    auto fixed = GetFixedTime();
    while (g_physics.accumulator > fixed)
    {
        b2World_Step(g_physics_world, fixed, 4);
        g_physics.accumulator -= fixed;
    }
}

b2WorldId GetPhysicsWorld()
{
    return g_physics_world;
}

RaycastResult Raycast(const vec2& start, const vec2& end, uint16 category_mask)
{
    RaycastResult result {};
    result.hit = false;

    if (!B2_IS_NON_NULL(g_physics_world))
        return result;

    // Box2D 3.x raycast API
    b2Vec2 translation = {ToBox2d(end).x - ToBox2d(start).x, ToBox2d(end).y - ToBox2d(start).y};
    b2QueryFilter filter = {0xFFFFFFFF, 0xFFFFFFFF}; // Default filter - collide with everything
    b2RayResult ray_result = b2World_CastRayClosest(g_physics_world, ToBox2d(start), translation, filter);
    
    if (ray_result.hit)
    {
        result.hit = true;
        result.point = FromBox2d(ray_result.point);
        result.normal = FromBox2d(ray_result.normal);
        result.fraction = ray_result.fraction;
        // TODO: Get user data from shape
        result.user_data = nullptr;
    }
    
    return result;
}

void InitPhysics()
{
    // Create Box2D 3.x world
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = {0.0f, 0.0f}; // No gravity by default
    g_physics_world = b2CreateWorld(&world_def);
}

void ShutdownPhysics()
{
    if (B2_IS_NON_NULL(g_physics_world))
    {
        b2DestroyWorld(g_physics_world);
        g_physics_world = b2_nullWorldId;
    }
    g_physics = {};
}
