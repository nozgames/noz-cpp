/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::physics
{
	NOZ_DEFINE_TYPEID(BoxCollider);

    BoxCollider::BoxCollider()
        : _size(1.0f, 1.0f)
        , _center(0.0f, 0.0f)
    {
        setName("BoxCollider");
    }

    void BoxCollider::createCollider(RigidBody* rigidBody)
    {
        if (rigidBody && rigidBody->_body)
        {
            // Create box shape directly
            b2PolygonShape boxShape;
            boxShape.SetAsBox(_size.x * 0.5f, _size.y * 0.5f, b2Vec2(_center.x, _center.y), 0.0f);

            // Create fixture definition
            b2FixtureDef fixtureDef;
            fixtureDef.shape = &boxShape;
            fixtureDef.density = _density;
            fixtureDef.friction = _friction;
            fixtureDef.restitution = _restitution;
            fixtureDef.isSensor = _isSensor;

            // Create the fixture on the rigid body and store it
            _fixture = rigidBody->_body->CreateFixture(&fixtureDef);
        }
    }
} 