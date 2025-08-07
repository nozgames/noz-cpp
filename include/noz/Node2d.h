/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "Node.h"

namespace noz::node
{
    class Node2d : public Node
    {
    public:

		NOZ_DECLARE_TYPEID(Node2d, Node)
        
        Node2d();
        virtual ~Node2d() = default;

        // Position
        const vec2& position() const { return _position; }
        void setPosition(const vec2& position);
        void translate(const vec2& offset);

        // Rotation (in radians)
        float rotation() const { return _rotation; }
        void setRotation(float rotation);
        void rotate(float angle);

        // Scale
        const vec2& scale() const { return _scale; }
        void setScale(const vec2& scale);
        void scaleBy(const vec2& scale);

        // Matrix getters (lazy evaluation)
        const mat3& localToWorldMatrix() const;
        const mat3& worldToLocalMatrix() const;

        // Utility methods
        float distance(const Node2d& other) const;
        bool isNear(const Node2d& other, float threshold) const;

        // Direction vectors
        vec2 forward() const;
        vec2 right() const;
        vec2 up() const;

        // Transform point from local to world space
        vec2 transformPoint(const vec2& localPoint) const;

        // Transform point from world to local space
        vec2 inverseTransformPoint(const vec2& worldPoint) const;

        // Transform direction from local to world space
        vec2 transformDirection(const vec2& localDirection) const;

        // Transform direction from world to local space
        vec2 inverseTransformDirection(const vec2& worldDirection) const;

        void lookAt(const vec2& target);

        // Override lifecycle methods
		void start() override;
        void update() override;

    protected:
        // Override parent methods
        void onAttachedToParent() override;
        void onDetachedFromParent() override;
        void onParentTransformChanged() override;

    private:
        vec2 _position;
        float _rotation;
        vec2 _scale;

        // Cached matrices
        mutable mat3 _localToWorldMatrix;
        mutable mat3 _worldToLocalMatrix;
        
        // Dirty flags for lazy evaluation
        mutable bool _localToWorldDirty;
        mutable bool _worldToLocalDirty;

        // Matrix update methods
        void updateLocalToWorldMatrix() const;
        void updateWorldToLocalMatrix() const;
        void markMatricesDirty();
    };
} 