/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::node
{
    class Node3d : public Node
    {
    public:
		
		NOZ_DECLARE_TYPEID(Node3d, Node)
        
        virtual ~Node3d() = default;
        
		const vec3& localPosition() const;
		vec3 position() const;
		quat rotation() const;
		vec3 localEulerAngles() const;
		vec3 eulerAngles() const;
		const quat& localRotation() const;
		const vec3& localScale() const;

		void setLocalPosition(const vec3& position);
		void setLocalPosition(float x, float y, float z);
		void setLocalEulerAngles(const vec3& rotation);
		void setLocalRotation(const quat& rotation);
		void setLocalScale(const vec3& scale);
		void setLocalScale(float scale);
		void setPosition(const vec3& position);
		void setPosition(float x, float y, float z);
		void setRotation(const quat& rotation);
		void setEulerAngles(const vec3& angles);
        void setEulerAngles(float x, float y, float z);

        void translate(const vec3& offset);

        const mat4& localToWorld() const;
        const mat4& worldToLocal() const;

        // Utility methods
        float distance(const Node3d& other) const;
        bool isNear(const Node3d& other, float threshold) const;

        // Direction vectors
        vec3 forward() const;
        vec3 right() const;
        vec3 up() const;

        // Transform point from local to world space
        vec3 transformPoint(const vec3& localPoint) const;

        // Transform point from world to local space
        vec3 inverseTransformPoint(const vec3& worldPoint) const;

        // Transform direction from local to world space
        vec3 transformDirection(const vec3& localDirection) const;

        // Transform direction from world to local space
        vec3 inverseTransformDirection(const vec3& worldDirection) const;

        void lookAt(const vec3& target, const vec3& up = vec3(0.0f, 1.0f, 0.0f));
        
        // Transform version tracking
        uint32_t transformVersion() const { return _transformVersion; }

        // Override lifecycle methods
        void start() override;
        void update() override;

    protected:

		Node3d();

		// Override parent methods
        void onAttachedToParent() override;
        void onDetachedFromParent() override;
        void onParentTransformChanged() override;

    private:
		
		void initialize() {} // for create pattern

		vec3 _localPosition;
        vec3 _localScale;
        quat _localRotation;

        // Cached matrices
        mutable mat4 _localToWorldMatrix;
        mutable mat4 _worldToLocalMatrix;
        
        // Dirty flags for lazy evaluation
        mutable bool _localToWorldDirty;
        mutable bool _worldToLocalDirty;
        
        // Transform version tracking
        uint32_t _transformVersion;

        // Matrix update methods
        void updateLocalToWorldMatrix() const;
        void updateWorldToLocalMatrix() const;
        void markMatricesDirty();
    };

	inline const vec3& Node3d::localPosition() const { return _localPosition; }
	inline vec3 Node3d::position() const { return vec3(localToWorld() * vec4(0.0f, 0.0f, 0.0f, 1.0f)); }
	inline quat Node3d::rotation() const {  return glm::quat_cast(localToWorld()); }

	inline const quat& Node3d::localRotation() const { return _localRotation; }
	inline const vec3& Node3d::localScale() const { return _localScale; }

    inline void Node3d::setLocalPosition(float x, float y, float z)
    {
        setLocalPosition(vec3(x, y, z));
    }

    inline void Node3d::setPosition(float x, float y, float z)
    {
		setPosition(vec3(x, y, z));
    }

    inline void Node3d::setEulerAngles(float x, float y, float z)
    {
        setEulerAngles(vec3(x, y, z));
    }
} 