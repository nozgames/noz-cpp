//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if 0

#include "physics_internal.h"

struct ColliderImpl : Component
{
    b2ShapeId shape;
    b2ShapeDef shape_def;
    ColliderType collider_type;
    Vec2 box_size;
    Vec2 center;
    float radius;
    bool initialized;
    Entity* target;
};

static ColliderImpl* Impl(Collider* c) { return (ColliderImpl*)Cast(c, TYPE_COLLIDER);}

Collider* CreateCollider(Allocator* allocator)
{
    auto collider = (Collider*)CreateObject(allocator, sizeof(ColliderImpl), TYPE_COLLIDER);
    auto impl = Impl(collider);
    impl->shape = b2_nullShapeId;
    impl->shape_def = b2DefaultShapeDef();
    impl->initialized = false;
    impl->target = nullptr;
    return collider;
}

Collider* CreateBoxCollider(Allocator* allocator, const Vec2& center, const Vec2& size, float density, float friction, float restitution, bool is_sensor)
{
    auto collider = CreateCollider(allocator);
    auto impl = Impl(collider);
    impl->collider_type = COLLIDER_TYPE_BOX;
    impl->center = center;
    impl->box_size = size;
    impl->shape_def.density = density;
    impl->shape_def.material.friction = friction;
    impl->shape_def.material.restitution = restitution;
    impl->shape_def.isSensor = is_sensor;
    return collider;
}

Collider* CreateCircleCollider(Allocator* allocator, const Vec2& center, float radius, float density, float friction, float restitution, bool is_sensor)
{
    auto collider = CreateCollider(allocator);
    auto impl = Impl(collider);
    impl->collider_type = COLLIDER_TYPE_CIRCLE;
    impl->center = center;
    impl->radius = radius;
    impl->shape_def.density = density;
    impl->shape_def.material.friction = friction;
    impl->shape_def.material.restitution = restitution;
    impl->shape_def.isSensor = is_sensor;
    return collider;
}

void CreateColliderShape(Collider* collider, Entity* entity)
{
    assert(collider);
    assert(entity);

    auto impl = Impl(collider);
    if (B2_IS_NON_NULL(impl->shape))
        return; // Shape already created

    // Get or create the rigid body for this entity
    // TODO: Fix component system integration
    /*
    auto rigid_body = (RigidBody*)GetComponent(entity, TYPE_RIGID_BODY);
    if (!rigid_body)
        return; // No rigid body component
    
    auto body_id = GetOrCreateRigidBodyId(rigid_body, entity);
    if (!B2_IS_NON_NULL(body_id))
        return; // Failed to create body
    */
    
    // For now, skip rigid body integration
    return;

    /*
    // Create shape based on collider type
    switch (impl->collider_type)
    {
    case COLLIDER_TYPE_BOX:
        {
            b2Polygon box = b2MakeOffsetBox(impl->box_size.x * 0.5f, impl->box_size.y * 0.5f, ToBox2d(impl->center), b2Rot_identity);
            impl->shape = b2CreatePolygonShape(body_id, &impl->shape_def, &box);
        }
        break;
    
    case COLLIDER_TYPE_CIRCLE:
        {
            b2Circle circle = {ToBox2d(impl->center), impl->radius};
            impl->shape = b2CreateCircleShape(body_id, &impl->shape_def, &circle);
        }
        break;
    
    default:
        return;
    }

    impl->initialized = true;
    impl->target = entity;
    */
}

void DestroyColliderShape(Collider* collider)
{
    if (!collider)
        return;

    auto impl = Impl(collider);
    if (!B2_IS_NON_NULL(impl->shape))
        return;

    b2DestroyShape(impl->shape, true);
    impl->shape = b2_nullShapeId;
    impl->initialized = false;
    impl->target = nullptr;
}

b2ShapeId GetColliderShapeId(Collider* collider)
{
    auto impl = Impl(collider);
    return impl->shape;
}

#if 0
void collider_scene_enter(collider component, entity parent)
{
    auto impl = component.impl();
	    NOZ_ASSERT_MSG(!impl->fixture, "Collider already has a fixture");

	    auto rb = parent.component<RigidBody>();
	    NOZ_ASSERT_MSG(rb, "Collider must be attached to a parent with a rigid body component");

    auto b2_body = rigid_body_get_or_create_body(rb);
    NOZ_ASSERT(b2_body);

    impl->fixture = b2_body->CreateFixture(&impl->fixture_def);
    }

void collider_scene_leave(collider component, entity parent)
{
    auto impl = component.impl();
    NOZ_ASSERT(impl->fixture);

	    auto fixture = impl->fixture;
    impl->fixture = nullptr;

    auto rb = parent.component<RigidBody>();
    if (!rb)
        return;

    auto b2_body = rigid_body_get_body(rb);
    if (!b2_body)
        return;

	    b2_body->DestroyFixture(fixture);
}
#endif

#if 0

class b2Fixture;

namespace noz::physics
{
class RigidBody;

class Collider : public noz::node::Node
{
public:

    void update() override;
    void start() override;

    // Collider properties
    float density() const { return _density; }
    void setDensity(float density) { _density = density; }

    float friction() const { return _friction; }
    void setFriction(float friction) { _friction = friction; }

    float restitution() const { return _restitution; }
    void setRestitution(float restitution) { _restitution = restitution; }

    bool isSensor() const { return _is_sensor; }
    void setSensor(bool sensor) { _is_sensor = sensor; }

    // Find and bind to physics body in parent hierarchy
    void bindToPhysicsBody();

    // Cleanup fixture
    void destroyFixture();

protected:
    void onAttachedToParent() override;
    void onDetachedFromParent() override;

    // Override in derived classes to create specific collider shapes
    virtual void createCollider(RigidBody* rigidBody) = 0;

};

#endif

#endif