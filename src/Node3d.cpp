/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
    NOZ_DEFINE_TYPEID(Node3d)
    Node3d::Node3d()
        : _localPosition(0.0f, 0.0f, 0.0f)
        , _localScale(1.0f, 1.0f, 1.0f)
        , _localRotation(1.0f, 0.0f, 0.0f, 0.0f)
        , _localToWorldDirty(true)
        , _worldToLocalDirty(true)
        , _transformVersion(0)
    {
    }

    void Node3d::setLocalPosition(const vec3& localPosition)
    {
        _localPosition = localPosition;
        markMatricesDirty();
    }

	void Node3d::setPosition(const vec3& position)
	{
		// Convert world position to local position
		auto parentNode = parent();
		if (parentNode)
		{
			// Parent must be Node3d
			auto parent3d = std::static_pointer_cast<Node3d>(parentNode);
			// Transform world position to parent's local space
			_localPosition = parent3d->inverseTransformPoint(position);
		}
		else
		{
			// No parent, world position is local position
			_localPosition = position;
		}
		markMatricesDirty();
	}

    void Node3d::translate(const vec3& offset)
    {
		_localPosition += offset;
        markMatricesDirty();
    }

    vec3 Node3d::localEulerAngles() const
    {
        return glm::degrees(glm::eulerAngles(_localRotation));
    }

	vec3 Node3d::eulerAngles() const
	{
		return glm::degrees(glm::eulerAngles(glm::quat_cast(localToWorld())));
	}

    void Node3d::setLocalEulerAngles(const vec3& rotation)
    {
        setLocalRotation(quat(glm::radians(rotation)));
    }

    void Node3d::setLocalRotation(const quat& rotation)
    {
        _localRotation = rotation;
        markMatricesDirty();
    }

	void Node3d::setRotation(const quat& rotation)
	{
		// Convert world rotation to local rotation
		auto parentNode = std::static_pointer_cast<Node3d>(parent());
		if (parentNode)
			_localRotation = glm::quat_cast(parentNode->worldToLocal()) * rotation;
		else
			_localRotation = rotation;

		markMatricesDirty();
	}

	void Node3d::setEulerAngles(const vec3& angles)
	{
		// Convert euler angles to quaternion and use setRotation
		setRotation(quat(glm::radians(angles)));
	}

	void Node3d::setLocalScale(float scale)
	{
		_localScale = glm::vec3(scale, scale, scale);
		markMatricesDirty();
	}

    void Node3d::setLocalScale(const vec3& scale)
    {
        _localScale = scale;
        markMatricesDirty();
    }

    const mat4& Node3d::localToWorld() const
    {
        if (_localToWorldDirty)
        {
            updateLocalToWorldMatrix();
        }
        return _localToWorldMatrix;
    }

    const mat4& Node3d::worldToLocal() const
    {
        if (_worldToLocalDirty)
        {
            updateWorldToLocalMatrix();
        }
        return _worldToLocalMatrix;
    }

    float Node3d::distance(const Node3d& other) const
    {
        return glm::distance(_localPosition, other._localPosition);
    }

    bool Node3d::isNear(const Node3d& other, float threshold) const
    {
        return distance(other) <= threshold;
    }

    vec3 Node3d::forward() const
    {
        return localToWorld()[2];
    }

    vec3 Node3d::right() const
    {
        return localToWorld()[0];
    }

    vec3 Node3d::up() const
    {
        return localToWorld()[1];
    }

    vec3 Node3d::transformPoint(const vec3& localPoint) const
    {
        const mat4& matrix = localToWorld();
        vec4 worldPoint = matrix * vec4(localPoint, 1.0f);
        return vec3(worldPoint);
    }

    vec3 Node3d::inverseTransformPoint(const vec3& worldPoint) const
    {
        const mat4& matrix = worldToLocal();
        vec4 localPoint = matrix * vec4(worldPoint, 1.0f);
        return vec3(localPoint);
    }

    vec3 Node3d::transformDirection(const vec3& localDirection) const
    {
        const mat4& matrix = localToWorld();
        vec4 worldDirection = matrix * vec4(localDirection, 0.0f);
        return normalize(vec3(worldDirection));
    }

    vec3 Node3d::inverseTransformDirection(const vec3& worldDirection) const
    {
        const mat4& matrix = worldToLocal();
        vec4 localDirection = matrix * vec4(worldDirection, 0.0f);
        return normalize(vec3(localDirection));
    }

    void Node3d::lookAt(const vec3& target, const vec3& up)
    {
        // Calculate direction from this node to target
        vec3 direction = normalize(target - _localPosition);
        
        // Handle case where direction is parallel to up vector
        if (abs(dot(direction, up)) > 0.999f)
        {
            // Use alternative up vector
            vec3 altUp = abs(up.y) > 0.999f ? vec3(1, 0, 0) : vec3(0, 1, 0);
            setRotation(glm::quatLookAt(direction, altUp));
        }
        else
        {
            setRotation(glm::quatLookAt(direction, up));
        }
    }

    void Node3d::start()
    {
        Node::start();
    }

    void Node3d::update()
    {
        Node::update();
    }

    void Node3d::onAttachedToParent()
    {
        Node::onAttachedToParent();
        
        // Ensure that Node3d can only be added to other Node3d parents (or no parent for root)
        auto parentNode = parent();
        if (parentNode)
        {
            assert(std::dynamic_pointer_cast<Node3d>(parentNode) && 
                   "Node3d can only be added as child of another Node3d");
        }
        
        markMatricesDirty();
    }

    void Node3d::onDetachedFromParent()
    {
        Node::onDetachedFromParent();
        markMatricesDirty();
    }

    void Node3d::updateLocalToWorldMatrix() const
    {
        // Compute local transform matrix (T * R * S)
        mat4 localMatrix = 
			glm::translate(_localPosition) *
			glm::mat4_cast(_localRotation) *
			glm::scale(_localScale);
        
        // Apply hierarchical transform: worldMatrix = parentWorldMatrix * localMatrix
        auto parentNode = parent();
        if (parentNode)
        {
            // Parent must be Node3d (enforced by onAttachedToParent assertion)
            auto parent3d = std::static_pointer_cast<Node3d>(parentNode);
            _localToWorldMatrix = parent3d->localToWorld() * localMatrix;
        }
        else
        {
            // No parent, world matrix is the same as local matrix
            _localToWorldMatrix = localMatrix;
        }
        
        _localToWorldDirty = false;
        _worldToLocalDirty = true;
    }

    void Node3d::updateWorldToLocalMatrix() const
    {
        _worldToLocalMatrix = inverse(localToWorld());
        _worldToLocalDirty = false;
    }

    void Node3d::markMatricesDirty()
    {
        _localToWorldDirty = true;
        _worldToLocalDirty = true;
        _transformVersion++;
        
        // Notify all children that parent transform has changed
        for (size_t i = 0; i < childCount(); ++i)
        {
            auto childNode = child(i);
            childNode->onParentTransformChanged();
        }
    }
    
    void Node3d::onParentTransformChanged()
    {
        markMatricesDirty();
    }
} 