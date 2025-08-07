/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
    NOZ_DEFINE_TYPEID(Node2d)
    Node2d::Node2d()
        : _position(0.0f, 0.0f)
        , _rotation(0.0f)
        , _scale(1.0f, 1.0f)
        , _localToWorldDirty(true)
        , _worldToLocalDirty(true)
    {
    }

    void Node2d::setPosition(const vec2& position)
    {
        _position = position;
        markMatricesDirty();
    }

    void Node2d::translate(const vec2& offset)
    {
        _position += offset;
        markMatricesDirty();
    }

    void Node2d::setRotation(float rotation)
    {
        _rotation = rotation;
        markMatricesDirty();
    }

    void Node2d::rotate(float angle)
    {
        _rotation += angle;
        markMatricesDirty();
    }

    void Node2d::setScale(const vec2& scale)
    {
        _scale = scale;
        markMatricesDirty();
    }

    void Node2d::scaleBy(const vec2& scale)
    {
        _scale *= scale;
        markMatricesDirty();
    }

    const mat3& Node2d::localToWorldMatrix() const
    {
        if (_localToWorldDirty)
        {
            updateLocalToWorldMatrix();
        }
        return _localToWorldMatrix;
    }

    const mat3& Node2d::worldToLocalMatrix() const
    {
        if (_worldToLocalDirty)
        {
            updateWorldToLocalMatrix();
        }
        return _worldToLocalMatrix;
    }

    float Node2d::distance(const Node2d& other) const
    {
        return glm::distance(_position, other._position);
    }

    bool Node2d::isNear(const Node2d& other, float threshold) const
    {
        return distance(other) <= threshold;
    }

    vec2 Node2d::forward() const
    {
        return vec2(cos(_rotation), sin(_rotation));
    }

    vec2 Node2d::right() const
    {
        return vec2(cos(_rotation + glm::half_pi<float>()), sin(_rotation + glm::half_pi<float>()));
    }

    vec2 Node2d::up() const
    {
        return vec2(-sin(_rotation), cos(_rotation));
    }

    vec2 Node2d::transformPoint(const vec2& localPoint) const
    {
        return localToWorldMatrix() * vec3(localPoint, 1.0f);
    }

    vec2 Node2d::inverseTransformPoint(const vec2& worldPoint) const
    {
        return worldToLocalMatrix() * vec3(worldPoint, 1.0f);
    }

    vec2 Node2d::transformDirection(const vec2& localDirection) const
    {
        return localToWorldMatrix() * vec3(localDirection, 0.0f);
    }

    vec2 Node2d::inverseTransformDirection(const vec2& worldDirection) const
    {
        return worldToLocalMatrix() * vec3(worldDirection, 0.0f);
    }

    void Node2d::lookAt(const vec2& target)
    {
        vec2 direction = glm::normalize(target - _position);
        _rotation = atan2(direction.y, direction.x);
        markMatricesDirty();
    }

    void Node2d::start()
    {
        Node::start();
    }

    void Node2d::update()
    {
        Node::update();
    }

    void Node2d::onAttachedToParent()
    {
        Node::onAttachedToParent();
        
        // Ensure that Node2d can only be added to other Node2d parents (or no parent for root)
        auto parentNode = parent();
        if (parentNode)
        {
            assert(std::dynamic_pointer_cast<Node2d>(parentNode) && 
                   "Node2d can only be added as child of another Node2d");
        }
        
        markMatricesDirty();
    }

    void Node2d::onDetachedFromParent()
    {
        Node::onDetachedFromParent();
        markMatricesDirty();
    }

    void Node2d::updateLocalToWorldMatrix() const
    {
        // Create local transformation matrix: T * R * S
        // For 2D, we'll use a 3x3 matrix
        mat3 localMatrix = mat3(1.0f);
        
        // Apply translation
        localMatrix[2][0] = _position.x;
        localMatrix[2][1] = _position.y;
        
        // Apply rotation (around Z-axis)
        float cosRot = cos(_rotation);
        float sinRot = sin(_rotation);
        localMatrix[0][0] = cosRot * _scale.x;
        localMatrix[0][1] = sinRot * _scale.x;
        localMatrix[1][0] = -sinRot * _scale.y;
        localMatrix[1][1] = cosRot * _scale.y;
        
        // Apply hierarchical transform: worldMatrix = parentWorldMatrix * localMatrix
        auto parentNode = parent();
        if (parentNode)
        {
            // Parent must be Node2d (enforced by onAttachedToParent assertion)
            auto parent2d = std::static_pointer_cast<Node2d>(parentNode);
            _localToWorldMatrix = parent2d->localToWorldMatrix() * localMatrix;
        }
        else
        {
            // No parent, world matrix is the same as local matrix
            _localToWorldMatrix = localMatrix;
        }
        
        _localToWorldDirty = false;
    }

    void Node2d::updateWorldToLocalMatrix() const
    {
        _worldToLocalMatrix = glm::inverse(localToWorldMatrix());
        _worldToLocalDirty = false;
    }

    void Node2d::markMatricesDirty()
    {
        _localToWorldDirty = true;
        _worldToLocalDirty = true;
        
        // Notify all children that parent transform has changed
        for (size_t i = 0; i < childCount(); ++i)
        {
            auto childNode = child(i);
            childNode->onParentTransformChanged();
        }
    }
    
    void Node2d::onParentTransformChanged()
    {
        markMatricesDirty();
    }
} 