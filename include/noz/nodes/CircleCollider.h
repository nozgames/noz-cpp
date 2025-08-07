#pragma once

#include "Collider.h"

namespace noz::physics
{
    class CircleCollider : public Collider
    {
    public:
        CircleCollider();
        virtual ~CircleCollider() = default;

        // Circle properties
        float radius() const { return _radius; }
        void setRadius(float radius) { _radius = radius; }

        glm::vec2 center() const { return _center; }
        void setCenter(const glm::vec2& center) { _center = center; }

    protected:
        void createCollider(RigidBody* rigidBody) override;

    private:
        float _radius;
        glm::vec2 _center;
    };
} 