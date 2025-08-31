//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "physics_internal.h"

struct ColliderImpl : Component
{
    b2Fixture* fixture;
    b2FixtureDef fixture_def;
    b2Shape* shape;
    RigidBody rigid_body;
};

static ColliderImpl* Impl(Collider* c) { return (ColliderImpl*)Cast(c, TYPE_COLLIDER);}

static Collider* CreateCollider(Allocator* allocator, float density, float friction, float restitution, bool is_sensor)
{
    auto collider = (Collider*)CreateObject(allocator, sizeof(ColliderImpl), TYPE_COLLIDER);
    auto impl = Impl(collider);
    impl->fixture_def = {};
    impl->fixture_def.density = density;;
	    impl->fixture_def.friction = friction;
	    impl->fixture_def.restitution = restitution;
	    impl->fixture_def.isSensor = is_sensor;
    impl->fixture_def.shape = nullptr;
    return collider;
}

Collider* CreateBoxCollider(Allocator* allocator, RigidBody* rigid_body, const vec2& center, const vec2& size, float density, float friction, float restitution, bool is_sensor)
{
    auto collider = CreateCollider(allocator, density, friction, restitution, is_sensor);
    auto impl = Impl(collider);
    auto shape = new b2PolygonShape();
    shape->SetAsBox(size.x * 0.5f, size.y * 0.5f, ToBox2d(center), 0.0f);
    impl->shape = shape;
    return collider;
}

Collider* CreateCircleCollider(Allocator* allocator, RigidBody* rigid_body, const vec2& center, float radius, float density, float friction, float restitution, bool is_sensor)
{
    Exit("not_implemented");
    return nullptr;
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