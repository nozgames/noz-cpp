/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "ContactListener.h"

namespace noz::physics
{
    // PhysicsSystem implementation
    PhysicsSystem::PhysicsSystem()
        : _world(nullptr)
        , _contactListener(std::make_unique<ContactListener>())
        , _physicsScale(1.0f)
		, _accumulator(0.0)
        , _debugDrawEnabled(false)
    {
    }

    PhysicsSystem::~PhysicsSystem()
    {
        if (_world)
        {
            delete _world;
            _world = nullptr;
        }
    }

    void PhysicsSystem::load()
    {
		ISingleton<PhysicsSystem>::load();
		instance()->loadInternal();
    }

	void PhysicsSystem::loadInternal()
	{
		assert(!_world);

		_world = new b2World(b2Vec2(0,0));
		//_world->SetContactListener(_contactListener.get());
	}

    void PhysicsSystem::unload()
    {
		instance()->unloadInternal();
		ISingleton<PhysicsSystem>::unload();
    }

	void PhysicsSystem::unloadInternal()
	{
		assert(_world);

		delete _world;
		_world = nullptr;
	}

    void PhysicsSystem::update()
    {
        if (!_world)
            return;

		_accumulator += noz::Time::deltaTime();

		while (_accumulator > Time::fixedTime())
		{
			_world->Step(Time::fixedTime(), 6, 2);
			_accumulator -= Time::fixedTime();
		}        

        // Clear forces (optional, but recommended for performance)
        _world->ClearForces();
    }

    glm::vec2 PhysicsSystem::getGravity() const
    {
        if (!_world)
            return glm::vec2(0.0f);

        b2Vec2 gravity = _world->GetGravity();
        return glm::vec2(gravity.x, gravity.y);
    }

    void PhysicsSystem::setGravity(const glm::vec2& gravity)
    {
        if (!_world)
            return;

        _world->SetGravity(b2Vec2(gravity.x, gravity.y));
    }

    glm::vec2 PhysicsSystem::worldToPhysics(const glm::vec2& worldPos) const
    {
        return worldPos / _physicsScale;
    }

    glm::vec2 PhysicsSystem::physicsToWorld(const glm::vec2& physicsPos) const
    {
        return physicsPos * _physicsScale;
    }

    PhysicsSystem::RaycastResult PhysicsSystem::raycast(const glm::vec2& start, const glm::vec2& end, uint16 categoryMask)
    {
        RaycastResult result;
        result.hit = false;

        if (!_world)
            return result;

        // Convert to physics coordinates
        b2Vec2 b2Start = b2Vec2(start.x, start.y);
        b2Vec2 b2End = b2Vec2(end.x, end.y);

        // Create raycast callback
        class RaycastCallback : public b2RayCastCallback
        {
        public:
            RaycastCallback(RaycastResult& result) : _result(result) {}

            float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override
            {
                _result.hit = true;
                _result.point = glm::vec2(point.x, point.y);
                _result.normal = glm::vec2(normal.x, normal.y);
                _result.fraction = fraction;
                _result.userData = reinterpret_cast<void*>(fixture->GetBody()->GetUserData().pointer);
                return fraction; // Return fraction to continue raycast
            }

        private:
            RaycastResult& _result;
        };

        RaycastCallback callback(result);
        _world->RayCast(&callback, b2Start, b2End);

        return result;
    }

    PhysicsSystem::QueryResult PhysicsSystem::queryArea(const glm::vec2& center, const glm::vec2& size, uint16 categoryMask)
    {
        QueryResult result;

        if (!_world)
            return result;

        // Create AABB
        b2AABB aabb;
        aabb.lowerBound = b2Vec2(center.x - size.x * 0.5f, center.y - size.y * 0.5f);
        aabb.upperBound = b2Vec2(center.x + size.x * 0.5f, center.y + size.y * 0.5f);

        // Create query callback
        class QueryCallback : public b2QueryCallback
        {
        public:
            QueryCallback(QueryResult& result, uint16 categoryMask) 
                : _result(result), _categoryMask(categoryMask) {}

            bool ReportFixture(b2Fixture* fixture) override
            {
                // Check category mask
                if ((fixture->GetFilterData().categoryBits & _categoryMask) != 0)
                {
                    _result.bodies.push_back(reinterpret_cast<void*>(fixture->GetBody()->GetUserData().pointer));
                }
                return true; // Continue query
            }

        private:
            QueryResult& _result;
            uint16 _categoryMask;
        };

        QueryCallback callback(result, categoryMask);
        _world->QueryAABB(&callback, aabb);

        return result;
    }

    //void PhysicsSystem::setBeginContactCallback(ContactListener::ContactCallback callback)
    //{
    //    _contactListener->setBeginContactCallback(callback);
    //}

    //void PhysicsSystem::setEndContactCallback(ContactListener::ContactCallback callback)
    //{
    //    _contactListener->setEndContactCallback(callback);
    //}

    //void PhysicsSystem::setPreSolveCallback(ContactListener::ContactCallback callback)
    //{
    //    _contactListener->setPreSolveCallback(callback);
    //}

    //void PhysicsSystem::setPostSolveCallback(ContactListener::ContactCallback callback)
    //{
    //    _contactListener->setPostSolveCallback(callback);
    //}

    void PhysicsSystem::enableDebugDraw(bool enable)
    {
        _debugDrawEnabled = enable;
        // TODO: Implement debug drawing if needed
    }
}
