/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::node
{
    /**
     * @brief Follow node that tracks another Node3d's position
     * Uses lateUpdate to ensure the target has moved before following
     */
    class Follow : public Node3d
    {
    public:
        Follow();
        virtual ~Follow() = default;

        // Target management
        void setTarget(std::shared_ptr<Node3d> target) { _target = target; }
        std::shared_ptr<Node3d> target() const { return _target.lock(); }

        // Follow settings
        void setFollowX(bool follow) { _followX = follow; }
        void setFollowY(bool follow) { _followY = follow; }
        void setFollowZ(bool follow) { _followZ = follow; }
        void setFollowRotation(bool follow) { _followRotation = follow; }

        bool followX() const { return _followX; }
        bool followY() const { return _followY; }
        bool followZ() const { return _followZ; }
        bool followRotation() const { return _followRotation; }

        // Offset from target
        void setOffset(const vec3& offset) { _offset = offset; }
        const vec3& offset() const { return _offset; }

        // Override lateUpdate to follow after target has moved
        void lateUpdate() override;

    private:
        std::weak_ptr<Node3d> _target;
        vec3 _offset;
        
        // Follow flags
        bool _followX;
        bool _followY; 
        bool _followZ;
        bool _followRotation;
    };
}