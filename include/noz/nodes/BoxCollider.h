#pragma once

#include "Collider.h"

namespace noz::physics
{
    class BoxCollider : public Collider
    {
    public:
		
		NOZ_DECLARE_TYPEID(BoxCollider, Collider);

        BoxCollider();
        virtual ~BoxCollider() = default;

        // Box properties
        glm::vec2 size() const { return _size; }
        void setSize(const glm::vec2& size) { _size = size; }

        glm::vec2 center() const { return _center; }
        void setCenter(const glm::vec2& center) { _center = center; }

    protected:
        void createCollider(RigidBody* rigidBody) override;

    private:
        glm::vec2 _size;
        glm::vec2 _center;
    };
} 