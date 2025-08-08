/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
    NOZ_DEFINE_TYPEID(Node)
    uint64_t Node::_nextId = 1;

    Node::Node()
        : _name("Node")
        , _id(_nextId++)
        , _state(NodeState::None)
    {
    }

    Node::~Node()
    {
        // Mark as destroyed
        _state = NodeState::Destroyed;
        
        // Remove all children
        _children.clear();
    }

    void Node::initialize()
    {
    }

    void Node::addChild(std::shared_ptr<Node> parent, std::shared_ptr<Node> child)
    {
        if (!parent || !child)
        {
            return;
        }

        // Remove child from its current parent
        child->removeFromParent();

        // Add to parent's children
        parent->_children.push_back(child);
        child->_parent = parent;

        // Propagate scene pointer to child
        child->setScene(parent->_scene);

        // Mark for start if we have a scene and the child needs to start
        if (parent->_scene && child->needsStart())
        {
            parent->_scene->markForStart(child);
        }

        // Notify child
        child->onAttachedToParent();
    }

    void Node::add(std::shared_ptr<Node> child)
    {
        try 
        {
            addChild(as<Node>(), child);
        }
        catch (const std::bad_weak_ptr&)
        {
            // This node is not managed by shared_ptr yet
            // This is expected when called from start() method before the node is fully started
            // Handle it gracefully by using the fallback approach
            if (!child)
                return;
                
            child->removeFromParent();
            _children.push_back(child);
            // Note: child's _parent will remain empty since we can't get shared_from_this()
            child->setScene(_scene);
            
            if (_scene && child->needsStart())
            {
                _scene->markForStart(child);
            }
            
            child->onAttachedToParent();
        }
    }

    void Node::remove(std::shared_ptr<Node> child)
    {
        if (!child)
        {
            return;
        }

        auto it = std::find(_children.begin(), _children.end(), child);
        if (it != _children.end())
        {
            child->onDetachedFromParent();
            child->_parent.reset();
            
            // Clear scene reference from child and all its descendants
            child->setScene(nullptr);
            
            _children.erase(it);
        }
    }

    void Node::removeFromParent()
    {
        auto parentNode = _parent.lock();
        if (parentNode)
        {
            parentNode->remove(as<Node>());
        }
    }

    std::shared_ptr<Node> Node::child(size_t index) const
    {
        if (index < _children.size())
        {
            return _children[index];
        }
        return nullptr;
    }
    
    std::shared_ptr<Node> Node::child(const noz::TypeId& typeId) const
    {
        for (const auto& childNode : _children)
        {
            if (childNode->isA(typeId))
            {
                return childNode;
            }
        }
        return nullptr;
    }

    size_t Node::childCount() const
    {
        return _children.size();
    }

    std::shared_ptr<Node> Node::parent() const
    {
        return _parent.lock();
    }

    std::shared_ptr<Node> Node::findChild(const std::string& name) const
    {
        for (const auto& child : _children)
        {
            if (child->name() == name)
            {
                return child;
            }
        }
        return nullptr;
    }

    std::shared_ptr<Node> Node::findChildRecursive(const std::string& name) const
    {
        // Check direct children first
        auto child = findChild(name);
        if (child)
        {
            return child;
        }

        // Recursively search in children
        for (const auto& child : _children)
        {
            auto result = child->findChildRecursive(name);
            if (result)
            {
                return result;
            }
        }

        return nullptr;
    }

    std::shared_ptr<Node> Node::findCommonAncestor(std::shared_ptr<Node> other) const
    {
        if (!other)
            return nullptr;            

        auto thisNode = ((Node*)this)->as<Node>();
        if (thisNode == other)
            return thisNode;
            
        // Get paths to root for both nodes
        std::vector<std::shared_ptr<Node>> pathThis;
        std::vector<std::shared_ptr<Node>> pathOther;
        
        auto current = thisNode;
        while (current)
        {
            pathThis.push_back(current);
            current = current->parent();
        }
        
        current = other;
        while (current)
        {
            pathOther.push_back(current);
            current = current->parent();
        }
        
        // Find common ancestor by comparing from root down
        std::shared_ptr<Node> commonAncestor = nullptr;
        auto minPath = std::min(pathThis.size(), pathOther.size());
        
        for (auto i = 1; i <= minPath; ++i)
        {
            auto nodeA = pathThis[pathThis.size() - i];
            auto nodeB = pathOther[pathOther.size() - i];
            
            if (nodeA == nodeB)
            {
                commonAncestor = nodeA;
            }
            else
            {
                break;
            }
        }
        
        return commonAncestor;
    }

    void Node::updateInternal()
    {
        if (!_scene || _state != NodeState::Active)
        {
            return;
        }

        // Call user-defined update method
        update();

        // Update all active children
        for (const auto& child : _children)
        {
            if (child->isActive())
            {
                child->updateInternal();
            }
        }
    }

    void Node::lateUpdateInternal()
    {
        if (!_scene || _state != NodeState::Active)
        {
            return;
        }

        // Call user-defined lateUpdate method
        lateUpdate();

        // LateUpdate all active children
        for (const auto& child : _children)
        {
            if (child->isActive())
            {
                child->lateUpdateInternal();
            }
        }
    }

    void Node::startInternal()
    {
        if (!_scene)
        {
            return;
        }

        // Only transition our own state if we're in InScene state
        if (_state == NodeState::InScene)
        {
            _state = NodeState::Active;
            
            // Call user-defined start method
            start();
        }

        // Always check and start children that need starting
        for (const auto& child : _children)
        {
            if (child->needsStart())
            {
                child->startInternal();
            }
        }
    }
    
    void Node::renderInternal(noz::renderer::CommandBuffer* commandBuffer)
    {
        if (!_scene || _state != NodeState::Active || !commandBuffer)
        {
            return;
        }

        // Call user-defined render method
        render(commandBuffer);

        // Render all active children
        for (const auto& child : _children)
        {
            if (child->isActive())
            {
                child->renderInternal(commandBuffer);
            }
        }
    }
    
    void Node::destroyInternal()
    {
        if (_state == NodeState::PendingDestroy || _state == NodeState::Destroyed)
        {
            return;
        }
        
        _state = NodeState::PendingDestroy;
        
        // Call user-defined destroy method
        destroy();
        
        // Mark all children for destruction
        for (const auto& child : _children)
        {
            child->destroyInternal();
        }
        
        // Add to scene's destroy list if we have a scene
        if (_scene)
        {
            _scene->markForDestroy(as<Node>());
        }
    }

    void Node::onAttachedToParent()
    {
        // Override in derived classes if needed
    }

    void Node::onDetachedFromParent()
    {
        // Override in derived classes if needed
    }

    void Node::setScene(std::shared_ptr<Scene> scene)
    {
		// Already set to the same scene, no need to update
		if (_scene == scene)
			return;

        // If we're being removed from a scene, reset state
        if (!scene && _scene)
        {
            _state = NodeState::None;
            onDetachFromScene();
        }
        // If we're being added to a scene, set to in-scene (needs start)
        else if (scene && !_scene)
        {
            _state = NodeState::InScene;
        }

        _scene = scene;
        
        // Call onAttachToScene if we just got attached to a scene
        if (scene)
        {
            onAttachToScene();
        }
        
        // Mark for start if we need to start and have a scene
        if (scene && needsStart())
        {
            scene->markForStart(as<Node>());
        }
        
        // Propagate scene pointer to all children
        for (auto& child : _children)
            child->setScene(scene);
    }

	void Node::setName(const std::string& name)
	{
		if (name == _name)
			return;

		_name = name;
		onNameChanged();
	}
} 