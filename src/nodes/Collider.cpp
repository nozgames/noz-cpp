/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::physics
{
	NOZ_DEFINE_TYPEID(Collider);

    Collider::Collider()
        : _density(1.0f)
        , _friction(0.2f)
        , _restitution(0.0f)
        , _isSensor(false)
        , _bound(false)
        , _fixture(nullptr)
    {
        setName("Collider");
    }

    Collider::~Collider()
    {
        destroyFixture();
    }

    void Collider::update()
    {
        noz::node::Node::update();
    }

    void Collider::start()
    {
        noz::node::Node::start();

        // Try to bind to a physics body
        bindToPhysicsBody();
    }

    void Collider::bindToPhysicsBody()
    {
        if (_bound)
            return;

        // First check if this collider is a sibling of a RigidBody
        auto parentNode = parent();
        if (parentNode)
        {
            for (size_t i = 0; i < parentNode->childCount(); ++i)
            {
                auto sibling = parentNode->child(i);
                auto rigidBody = std::dynamic_pointer_cast<RigidBody>(sibling);
                if (rigidBody)
                {
                    // Found a rigid body sibling, create the collider
                    _rigidBody = rigidBody;
                    createCollider(rigidBody.get());
                    _bound = true;
                    return;
                }
            }
        }

        // Look for a RigidBody in the parent hierarchy
        auto current = parent();
        while (current)
        {
            // Try to cast to RigidBody
            auto rigidBody = std::dynamic_pointer_cast<RigidBody>(current);
            if (rigidBody)
            {
                // Found a rigid body parent, create the collider
                _rigidBody = rigidBody;
                createCollider(rigidBody.get());
                _bound = true;
                return;
            }

            current = current->parent();
        }
    }

    void Collider::onAttachedToParent()
    {
        noz::node::Node::onAttachedToParent();
    }

    void Collider::onDetachedFromParent()
    {
        noz::node::Node::onDetachedFromParent();
        destroyFixture();
    }

    void Collider::destroyFixture()
    {
        if (_fixture)
        {
            auto rigidBody = _rigidBody.lock();
            if (rigidBody && rigidBody->_body)
            {
                rigidBody->_body->DestroyFixture(_fixture);
            }
            _fixture = nullptr;
            _bound = false;
        }
    }
} 