#pragma once

// Forward declarations
class b2World;
class b2Body;
enum b2BodyType;

namespace noz::physics
{
    class Collider;
    class BoxCollider;
    class CircleCollider;

    class RigidBody : public noz::node::Node
    {
        friend class Collider;
        friend class BoxCollider;
        friend class CircleCollider;
    public:
		
		NOZ_DECLARE_TYPEID(RigidBody, noz::node::Node)
        enum class BodyType
        {
            Static,     // Immovable body
            Dynamic,    // Full physics simulation
            Kinematic   // Moved by code but affects other bodies
        };

        RigidBody();
        virtual ~RigidBody();

        // Node lifecycle
        void start() override;
        void update() override;

        // Body type management
        void setBodyType(BodyType bodyType);
        BodyType bodyType() const { return _bodyType; }

        // Physics properties
        glm::vec2 linearVelocity() const;
        void setLinearVelocity(const glm::vec3& velocity);

        float angularVelocity() const;
        void setAngularVelocity(float velocity);

        void applyForce(const glm::vec2& force, const glm::vec2& point = glm::vec2(0.0f));
        void applyForceToCenter(const glm::vec2& force);
        void applyLinearImpulse(const glm::vec2& impulse, const glm::vec2& point = glm::vec2(0.0f));
        void applyLinearImpulseToCenter(const glm::vec2& impulse);

        float gravityScale() const;
        void setGravityScale(float scale);

        float linearDamping() const;
        void setLinearDamping(float damping);

        float angularDamping() const;
        void setAngularDamping(float damping);

        bool isFixedRotation() const;
        void setFixedRotation(bool fixed);

        bool isBullet() const;
        void setBullet(bool bullet);

        bool isAwake() const;
        void wakeUp();
        void sleep();


        // Transform sync
        void syncTransformFromPhysics();
        void syncTransformToPhysics();

    protected:
        void onAttachedToParent() override;
        void onDetachedFromParent() override;
        void onAttachToScene() override;
        void onDetachFromScene() override;

    private:
        b2Body* _body;
        BodyType _bodyType;
        bool _initialized;
        uint32_t _lastTransformVersion;

        // Helper methods
        b2BodyType convertBodyType(BodyType bodyType) const;
        void createPhysicsBody();
        void destroyPhysicsBody();
        b2World* getPhysicsWorld() const;
    };
}