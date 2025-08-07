/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::physics
{
    CircleCollider::CircleCollider()
        : _radius(0.5f)
        , _center(0.0f, 0.0f)
    {
        setName("CircleCollider");
    }

    void CircleCollider::createCollider(RigidBody* rigidBody)
    {
        if (rigidBody && rigidBody->_body)
        {
            // Create circle shape directly
            b2CircleShape circleShape;
            circleShape.m_p = b2Vec2(_center.x, _center.y);
            circleShape.m_radius = _radius;

            // Create fixture definition
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &circleShape;
            fixtureDef.density = _density;
            fixtureDef.friction = _friction;
            fixtureDef.restitution = _restitution;
            fixtureDef.isSensor = _isSensor;

            // Create the fixture on the rigid body and store it
            _fixture = rigidBody->_body->CreateFixture(&fixtureDef);
        }
    }
} 