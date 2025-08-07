#pragma once

class b2Fixture;

namespace noz::physics
{
    class RigidBody;

    class Collider : public noz::node::Node
    {
    public:

		NOZ_DECLARE_TYPEID(Collider, Node);

        Collider();
        virtual ~Collider();

        void update() override;
        void start() override;

        // Collider properties
        float density() const { return _density; }
        void setDensity(float density) { _density = density; }

        float friction() const { return _friction; }
        void setFriction(float friction) { _friction = friction; }

        float restitution() const { return _restitution; }
        void setRestitution(float restitution) { _restitution = restitution; }

        bool isSensor() const { return _isSensor; }
        void setSensor(bool sensor) { _isSensor = sensor; }

        // Find and bind to physics body in parent hierarchy
        void bindToPhysicsBody();

        // Cleanup fixture
        void destroyFixture();

    protected:
        void onAttachedToParent() override;
        void onDetachedFromParent() override;

        // Override in derived classes to create specific collider shapes
        virtual void createCollider(RigidBody* rigidBody) = 0;

    protected:
        float _density;
        float _friction;
        float _restitution;
        bool _isSensor;
        bool _bound;
        std::weak_ptr<RigidBody> _rigidBody;
        b2Fixture* _fixture;
    };
} 