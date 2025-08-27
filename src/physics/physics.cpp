//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "physics_internal.h"

struct raycast_callback : b2RayCastCallback
{
    raycast_callback(RaycastResult& result) : _result(result) {}

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
    {
        _result.hit = true;
        _result.point = vec2(point.x, point.y);
        _result.normal = vec2(normal.x, normal.y);
        _result.fraction = fraction;
        _result.user_data = reinterpret_cast<void*>(fixture->GetBody()->GetUserData().pointer);
        return fraction;
    }

    RaycastResult& _result;
};

struct Physics
{
    b2World* world;
    float accumulator;
};

static Physics g_physics = {};

void UpdatePhysics()
{
    assert(g_physics.world);

    g_physics.accumulator += GetDeltaTime();

    auto fixed = GetFixedTime();
    while (g_physics.accumulator > fixed)
    {
        g_physics.world->Step(fixed, 6, 2);
        g_physics.accumulator -= fixed;
    }

    g_physics.world->ClearForces();
}

b2World* world()
{
    return g_physics.world;
}

RaycastResult Raycast(const vec2& start, const vec2& end, uint16 category_mask)
{
    RaycastResult result {};
    result.hit = false;

    if (!g_physics.world)
        return result;

    raycast_callback callback(result);
    g_physics.world->RayCast(&callback, ToBox2d(start), ToBox2d(end));
    return result;
}

void InitPhysics()
{
    g_physics.world = new b2World(b2Vec2(0, 0));
}

void ShutdownPhysics()
{
    delete g_physics.world;
    g_physics.world = nullptr;
    g_physics = {};
}
