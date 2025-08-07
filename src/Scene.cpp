/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::node
{
	NOZ_DEFINE_TYPEID(Scene);

    Scene::Scene()
        : _name("Scene")
        , _started(false)
    {
    }

    Scene::~Scene()
    {
        // Clear root to break circular references
        _root.reset();
    }

    void Scene::setRoot(std::shared_ptr<Node> root)
    {
        // Remove old root from scene if it exists
        if (_root)
        {
            _root->setScene(nullptr);
        }

        _root = root;

        // Set new root's scene pointer
        if (_root)
        {
            _root->setScene(as<Scene>());
        }
    }

    void Scene::update()
    {        
        // Call start() on first update if not already started
        if (!_started)
        {
            start();
            _started = true;
        }

		if (_root == nullptr)
			return;

        // Process any pending starts
        if (_startRoot)
        {
            _startRoot->startInternal();
            _startRoot.reset();
        }

        _root->updateInternal();
        
        // Call lateUpdate after all updates are complete
        lateUpdate();
        
        // Process destroy list at the end of the frame
        processDestroyList();
    }

    void Scene::lateUpdate()
    {
        if (!_root)
            return;
            
        _root->lateUpdateInternal();
    }

    void Scene::start()
    {
		_startRoot = _root;
    }
    
    void Scene::render(noz::renderer::CommandBuffer* commandBuffer)
    {
		assert(commandBuffer);		

        if (!_root)
            return;

        // Set the main camera if we have one
        auto mainCamera = Camera::main();
        if (mainCamera && !commandBuffer->isShadowPass())
        {
			mainCamera->forceMatrixUpdate();
            commandBuffer->setViewProjection(mainCamera->viewMatrix(), mainCamera->projectionMatrix());
        }

        // Bind the active directional light if we have one
        if (_activeDirectionalLight)
        {
            commandBuffer->bindLight(
                _activeDirectionalLight->direction(),
                _activeDirectionalLight->ambientIntensity(),
                _activeDirectionalLight->ambientColor(),
                1.0f - _activeDirectionalLight->ambientIntensity(),
                _activeDirectionalLight->diffuseColor()
            );
        }

        _root->renderInternal(commandBuffer);
    }
    
    void Scene::markForStart(std::shared_ptr<Node> node)
    {
        if (!node)
        {
            return;
        }
        
        // Find the root of the hierarchy that needs to start
        auto current = node;
        while (auto parent = current->parent())
        {
            if (parent->needsStart())
            {
                current = parent;
            }
            else
            {
                break;
            }
        }
        
        // Set as the start root if we don't have one, or if this node is higher in the hierarchy
        // We determine hierarchy by checking if current is an ancestor of _startRoot
        bool shouldReplace = !_startRoot;
        if (!shouldReplace && _startRoot)
        {
            // Check if current is an ancestor of _startRoot (higher in hierarchy)
            auto checkNode = _startRoot;
            while (checkNode && checkNode != current)
            {
                checkNode = checkNode->parent();
            }
            shouldReplace = (checkNode == current); // current is ancestor of _startRoot
        }
        
        if (shouldReplace)
        {
            _startRoot = current;
        }
    }
    
    void Scene::markForDestroy(std::shared_ptr<Node> node)
    {
        if (!node)
        {
            return;
        }
        
        // Add to destroy list if not already there
        auto it = std::find(_destroyList.begin(), _destroyList.end(), node);
        if (it == _destroyList.end())
        {
            _destroyList.push_back(node);
        }
    }
    
    void Scene::processDestroyList()
    {
        for (auto& node : _destroyList)
        {
            if (node && node->state() == NodeState::PendingDestroy)
            {
                // Remove from parent
                node->removeFromParent();
                
                // Mark as destroyed
                node->_state = NodeState::Destroyed;
                
                // Remove from scene
                node->setScene(nullptr);
            }
        }
        
        // Clear the destroy list
        _destroyList.clear();
    }

    void Scene::setActiveDirectionalLight(std::shared_ptr<DirectionalLight> light)
    {
        _activeDirectionalLight = light;
    }
} 