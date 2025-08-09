/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

class b2World;

namespace noz::physics
{
    class Physics;
	class ContactListener;

    /**
     * @brief Physics system that manages Box2D world and physics simulation
     */
    class PhysicsSystem : public noz::ISingleton<PhysicsSystem>
    {
    public:
        PhysicsSystem();
        ~PhysicsSystem();
        
        // Initialize the physics system
        bool initialize(const glm::vec2& gravity = glm::vec2(0.0f, -9.81f));

        // Update physics simulation (fixed timestep)
        void update();
        
        // Get/set fixed timestep
        float getFixedTimeStep() const { return _fixedTimeStep; }
        void setFixedTimeStep(float timeStep) { _fixedTimeStep = timeStep; }

        // Get/set gravity
        glm::vec2 getGravity() const;
        void setGravity(const glm::vec2& gravity);

        // Get/set physics scale (pixels per meter)
        float getPhysicsScale() const { return _physicsScale; }
        void setPhysicsScale(float scale) { _physicsScale = scale; }

        // Convert world coordinates to physics coordinates
        glm::vec2 worldToPhysics(const glm::vec2& worldPos) const;
        glm::vec2 physicsToWorld(const glm::vec2& physicsPos) const;

        // Raycast
        struct RaycastResult
        {
            bool hit;
            glm::vec2 point;
            glm::vec2 normal;
            float fraction;
            void* userData;
        };

        RaycastResult raycast(const glm::vec2& start, const glm::vec2& end, uint16_t categoryMask = 0xFFFF);

        // Query area
        struct QueryResult
        {
            std::vector<void*> bodies;
        };

        QueryResult queryArea(const glm::vec2& center, const glm::vec2& size, uint16_t categoryMask = 0xFFFF);

        // Debug drawing (optional)
        void enableDebugDraw(bool enable);
        bool isDebugDrawEnabled() const { return _debugDrawEnabled; }

		static void load();
		static void unload();

    private:

        friend class noz::ISingleton<PhysicsSystem>;
		friend class Physics;
		friend class RigidBody;

		b2World* world() const { return _world; }

		void loadInternal();
		void unloadInternal();

        b2World* _world;
        std::unique_ptr<ContactListener> _contactListener;
        float _physicsScale;
        bool _debugDrawEnabled;
        float _fixedTimeStep;
        float _accumulator;
    };
} // namespace noz::physics 