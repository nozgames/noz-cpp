/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::physics
{
    NOZ_DEFINE_TYPEID(RigidBody)
    RigidBody::RigidBody()
        : noz::node::Node()
        , _body(nullptr)
        , _bodyType(BodyType::Dynamic)
        , _initialized(false)
        , _lastTransformVersion(0)
    {
        setName("RigidBody");
    }

    RigidBody::~RigidBody()
    {
        destroyPhysicsBody();
    }

    void RigidBody::start()
    {
        noz::node::Node::start();
    }

    void RigidBody::update()
    {
        noz::node::Node::update();
        
        if (!_body) return;
        
        auto parentNode = std::dynamic_pointer_cast<noz::node::Node3d>(parent());
        if (!parentNode) return;
        
        // Check if transform has changed since last sync
        uint32_t currentVersion = parentNode->transformVersion();
        
        if (_bodyType == BodyType::Dynamic)
        {
            // For dynamic bodies, only sync from node to physics if transform changed externally
            if (currentVersion != _lastTransformVersion)
            {
                // Transform was changed externally, sync to physics
                syncTransformToPhysics();
                _lastTransformVersion = currentVersion;
            }
            else
            {
                // Normal physics simulation, sync from physics to node
                syncTransformFromPhysics();
            }
        }
        else if (_bodyType == BodyType::Kinematic || _bodyType == BodyType::Static)
        {
            // For kinematic/static bodies, always sync from node to physics if changed
            if (currentVersion != _lastTransformVersion)
            {
                syncTransformToPhysics();
                _lastTransformVersion = currentVersion;
            }
        }
    }

    void RigidBody::setBodyType(BodyType bodyType)
    {
        if (_bodyType != bodyType)
        {
            _bodyType = bodyType;
            if (_body)
            {
                _body->SetType(convertBodyType(bodyType));
            }
        }
    }

    glm::vec2 RigidBody::linearVelocity() const
    {
        if (!_body) return glm::vec2(0.0f);
        b2Vec2 velocity = _body->GetLinearVelocity();
        return glm::vec2(velocity.x, velocity.y);
    }

    void RigidBody::setLinearVelocity(const glm::vec3& velocity)
    {
		assert(_body);

        _body->SetLinearVelocity(b2Vec2(velocity.x, velocity.z));
    }

    float RigidBody::angularVelocity() const
    {
		assert(_body);
		return _body ? _body->GetAngularVelocity() : 0.0f;
    }

    void RigidBody::setAngularVelocity(float velocity)
    {
        if (_body)
        {
            _body->SetAngularVelocity(velocity);
        }
    }

    void RigidBody::applyForce(const glm::vec2& force, const glm::vec2& point)
    {
        if (_body)
        {
            _body->ApplyForce(b2Vec2(force.x, force.y), b2Vec2(point.x, point.y), true);
        }
    }

    void RigidBody::applyForceToCenter(const glm::vec2& force)
    {
        if (_body)
        {
            _body->ApplyForceToCenter(b2Vec2(force.x, force.y), true);
        }
    }

    void RigidBody::applyLinearImpulse(const glm::vec2& impulse, const glm::vec2& point)
    {
        if (_body)
        {
            _body->ApplyLinearImpulse(b2Vec2(impulse.x, impulse.y), b2Vec2(point.x, point.y), true);
        }
    }

    void RigidBody::applyLinearImpulseToCenter(const glm::vec2& impulse)
    {
        if (_body)
        {
            _body->ApplyLinearImpulseToCenter(b2Vec2(impulse.x, impulse.y), true);
        }
    }

    float RigidBody::gravityScale() const
    {
        return _body ? _body->GetGravityScale() : 1.0f;
    }

    void RigidBody::setGravityScale(float scale)
    {
        if (_body)
        {
            _body->SetGravityScale(scale);
        }
    }

    float RigidBody::linearDamping() const
    {
        return _body ? _body->GetLinearDamping() : 0.0f;
    }

    void RigidBody::setLinearDamping(float damping)
    {
        if (_body)
        {
            _body->SetLinearDamping(damping);
        }
    }

    float RigidBody::angularDamping() const
    {
        return _body ? _body->GetAngularDamping() : 0.0f;
    }

    void RigidBody::setAngularDamping(float damping)
    {
        if (_body)
        {
            _body->SetAngularDamping(damping);
        }
    }

    bool RigidBody::isFixedRotation() const
    {
        return _body ? _body->IsFixedRotation() : false;
    }

    void RigidBody::setFixedRotation(bool fixed)
    {
        if (_body)
        {
            _body->SetFixedRotation(fixed);
        }
    }

    bool RigidBody::isBullet() const
    {
        return _body ? _body->IsBullet() : false;
    }

    void RigidBody::setBullet(bool bullet)
    {
        if (_body)
        {
            _body->SetBullet(bullet);
        }
    }

    bool RigidBody::isAwake() const
    {
        return _body ? _body->IsAwake() : false;
    }

    void RigidBody::wakeUp()
    {
        if (_body)
        {
            _body->SetAwake(true);
        }
    }

    void RigidBody::sleep()
    {
        if (_body)
        {
            _body->SetAwake(false);
        }
    }


    void RigidBody::syncTransformFromPhysics()
    {
		assert(_body);

        auto parentNode = (noz::node::Node3d*)parent().get();
		assert(parentNode);

        auto pos = _body->GetPosition();
        auto rot = _body->GetAngle();

        parentNode->setPosition(glm::vec3(pos.x, parentNode->position().y, pos.y));
		parentNode->setEulerAngles(glm::vec3(0, glm::degrees(rot), 0));
        
        // Update our tracked version to match the new transform version
        _lastTransformVersion = parentNode->transformVersion();
    }

    void RigidBody::syncTransformToPhysics()
    {
        if (!_body) return;

		auto parentNode = (noz::node::Node3d*)parent().get();
		assert(parentNode);

        auto pos = parentNode->position();
		auto rot = parentNode->rotation();

		// Extract Y-axis rotation from quaternion
		// For a rotation purely around Y, the quaternion is (w, 0, y, 0)
		// The Y rotation angle can be extracted as: atan2(2*(w*y + x*z), 1 - 2*(y*y + z*z))
		float yAngle = atan2(2.0f * (rot.w * rot.y + rot.x * rot.z), 
		                     1.0f - 2.0f * (rot.y * rot.y + rot.z * rot.z));

		_body->SetTransform(b2Vec2(pos.x, pos.z), yAngle);
    }

    void RigidBody::onAttachedToParent()
    {
        noz::node::Node::onAttachedToParent();

		// Rigidbody must have a node3d as a parent
		assert(std::dynamic_pointer_cast<noz::node::Node3d>(parent()));
    }

    void RigidBody::onDetachedFromParent()
    {
        noz::node::Node::onDetachedFromParent();
    }
    
    void RigidBody::onAttachToScene()
    {
        noz::node::Node::onAttachToScene();
        
        // Create physics body immediately when attached to scene
        if (!_body && parent())
        {
            createPhysicsBody();
        }
    }
    
    void RigidBody::onDetachFromScene()
    {
        noz::node::Node::onDetachFromScene();
        destroyPhysicsBody();
    }

    b2BodyType RigidBody::convertBodyType(BodyType bodyType) const
    {
        switch (bodyType)
        {
        case BodyType::Static:
            return b2_staticBody;
        case BodyType::Dynamic:
            return b2_dynamicBody;
        case BodyType::Kinematic:
            return b2_kinematicBody;
        default:
            return b2_dynamicBody;
        }
    }

    void RigidBody::createPhysicsBody()
    {
		assert(!_body);

        auto world = getPhysicsWorld();
		assert(world);

        auto parentNode = (noz::node::Node3d*)parent().get();
		assert(parentNode);

		// Extract Y-axis rotation from quaternion
		auto rot = parentNode->rotation();
		float yAngle = atan2(2.0f * (rot.w * rot.y + rot.x * rot.z), 
		                     1.0f - 2.0f * (rot.y * rot.y + rot.z * rot.z));

        b2BodyDef bodyDef;
        bodyDef.type = convertBodyType(_bodyType);
        bodyDef.position.Set(parentNode->position().x, parentNode->position().z);
        bodyDef.angle = yAngle;
        bodyDef.userData.pointer = reinterpret_cast<uintptr_t>(this);

        _body = world->CreateBody(&bodyDef);
        _initialized = true;
        
        // Initialize transform version tracking
        _lastTransformVersion = parentNode->transformVersion();
    }

    void RigidBody::destroyPhysicsBody()
    {
        if (!_body) return;

        b2World* world = getPhysicsWorld();
        if (world)
        {
            world->DestroyBody(_body);
        }
        _body = nullptr;
        _initialized = false;
    }

    b2World* RigidBody::getPhysicsWorld() const
    {
        // Get the physics world from the PhysicsSystem
        auto physicsSystem = noz::physics::PhysicsSystem::instance();
        return physicsSystem ? physicsSystem->world() : nullptr;
    }
}