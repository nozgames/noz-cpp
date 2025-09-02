//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if 0

//#define isfinite(x) ((x) == std::numeric_limits<float>::infinity())
#include "physics_internal.h"

struct RigidBodyImpl : Component
{
    b2BodyId body;
    RigidBodyType body_type;
    bool initialized;
    uint32_t last_transform_version;
    Entity* target;
};

static RigidBodyImpl* Impl(RigidBody* r) { return (RigidBodyImpl*)Cast(r, TYPE_RIGID_BODY);}

RigidBody* CreateRigidBody(Allocator* allocator)
{
    auto rigid_body = (RigidBody*)CreateObject(allocator, sizeof(RigidBodyImpl), TYPE_RIGID_BODY);
    auto impl = Impl(rigid_body);
    impl->body = b2_nullBodyId;
    impl->body_type = RIGID_BODY_TYPE_DYNAMIC;
    impl->initialized = false;
    impl->last_transform_version = 0;
    return rigid_body;
}


void CreateRigidBodyInWorld(RigidBodyImpl* impl, Entity* entity)
{
    assert(impl);
    assert(entity);

    if (B2_IS_NON_NULL(impl->body))
        return; // Body already created

    // For now, create at origin with no rotation
    // TODO: Extract transform from entity when transform system is available
    Vec3 pos = {0.0f, 0.0f, 0.0f};
    float angle = 0.0f;

    // Create body with Box2D 3.x API
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = ToBox2d(impl->body_type);
    body_def.position = {pos.x, pos.z}; // Use X,Z for 2D physics (top-down view)
    body_def.rotation = b2MakeRot(angle);
    body_def.userData = entity; // Store entity pointer as user data

    impl->body = b2CreateBody(g_physics_world, &body_def);
    impl->initialized = true;
    impl->target = entity;
}

void DestroyRigidBodyInWorld(RigidBodyImpl* impl)
{
    if (!B2_IS_NON_NULL(impl->body))
        return;

    b2DestroyBody(impl->body);
    impl->body = b2_nullBodyId;
    impl->initialized = false;
    impl->target = nullptr;
}

b2BodyId GetRigidBodyId(RigidBody* rigid_body)
{
    auto impl = Impl(rigid_body);
    return impl->body;
}

b2BodyId GetOrCreateRigidBodyId(RigidBody* rigid_body, Entity* entity)
{
    auto impl = Impl(rigid_body);
    assert(impl);
    
    if (!B2_IS_NON_NULL(impl->body))
        CreateRigidBodyInWorld(impl, entity);

    return impl->body;
}

void SetRigidBodyType(RigidBody* rigid_body, RigidBodyType type)
{
    auto impl = Impl(rigid_body);
    impl->body_type = type;
    
    if (B2_IS_NON_NULL(impl->body))
        b2Body_SetType(impl->body, ToBox2d(type));
}


#endif