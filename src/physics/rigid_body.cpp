//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

//#define isfinite(x) ((x) == std::numeric_limits<float>::infinity())
#include "physics_internal.h"

struct RigidBodyImpl : Component
{
    b2Body* body;
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
    impl->body = nullptr;
    impl->body_type = RIGID_BODY_TYPE_DYNAMIC;
    impl->initialized = false;
    impl->last_transform_version = 0;
    return rigid_body;
}


#if 0
void rigid_body_create_body(RigidBodyImpl* impl, entity entity)
{
    NOZ_ASSERT(impl);

    // Extract Y-axis rotation from quaternion
    auto rot = entity.rotation();
    auto pos = entity.position();
    float angle = atan2(
        2.0f * (rot.w * rot.y + rot.x * rot.z),
        1.0f - 2.0f * (rot.y * rot.y + rot.z * rot.z));

    b2BodyDef body_def;
    body_def.type = to_b2(impl->body_type);
    body_def.position.Set(pos.x, pos.z);
    body_def.angle = angle;
    body_def.userData.pointer = reinterpret_cast<uintptr_t>(impl);

    auto world = physics::world();
    NOZ_ASSERT(world);
    impl->body = world->CreateBody(&body_def);

    impl->last_transform_version = entity.version();
}

void rigid_body_destroy_body(RigidBodyImpl* impl)
{
    if (!impl->body)
        return;

    auto world = physics::world();
    NOZ_ASSERT(world);

    world->DestroyBody(impl->body);
    impl->body = nullptr;
}

void rigid_body_scene_enter(RigidBody component, entity entity)
{
    auto impl = component.impl();
    NOZ_ASSERT(!impl->body);
    rigid_body_create_body(impl, entity);
}

void rigid_body_scene_leave(RigidBody component, entity entity)
{
    auto impl = component.impl();
    NOZ_ASSERT(impl->body);
    rigid_body_destroy_body(impl);
}

b2Body* rigid_body_get_body(RigidBody component)
{
    auto impl = component.impl();
    return impl->body;
}

b2Body* rigid_body_get_or_create_body(RigidBody component)
{
    auto impl = component.impl();
    NOZ_ASSERT(impl);
    if (impl->body == nullptr)
        rigid_body_create_body(impl, entity::for_component(component));

    return impl->body;
}
#endif
