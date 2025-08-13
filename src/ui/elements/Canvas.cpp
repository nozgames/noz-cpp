/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/ui/StyleSheet.h>

namespace noz::ui
{
	NOZ_DEFINE_TYPEID(Canvas)

    Canvas::Canvas()
        : Element()
        , _sortOrder(0)
        , _referenceSize(1920.0f, 1080.0f)
        , _canvasSize(1920.0f, 1080.0f)
        , _layoutDirtyRoot(nullptr)
        , _styleDirtyRoot(nullptr)
    {
        // Canvas starts clean, will be marked dirty when attached to scene
        _flags = ElementFlags::None;
    }

    void Canvas::initialize()
    {
		_camera = Object::create<noz::node::Camera>();
    }

    void Canvas::start()
    {
        Element::start();
        
        // Initialize screen size tracking
        _lastScreenWidth = Application::instance()->screenWidth();
        _lastScreenHeight = Application::instance()->screenHeight();
        
        // Initialize UI camera
		updateCamera();
        
        // Set Canvas bounds to screen rect for layout
        _bounds = noz::Rect(0, 0, _canvasSize.x, _canvasSize.y);
    }

    void Canvas::update()
    {
        // Check for screen size changes
        auto sw = Application::instance()->screenWidth();
        auto sh = Application::instance()->screenHeight();
        
        if (sw != _lastScreenWidth || sh != _lastScreenHeight)
        {
            _lastScreenWidth = sw;
            _lastScreenHeight = sh;
            
			updateCamera();
			markLayoutDirty();
        }

		_bounds = noz::Rect(0, 0, _canvasSize.x, _canvasSize.y);
                
        styleElements();
        layoutElements();
        
        Element::update();
    }

    void Canvas::render(noz::renderer::CommandBuffer* commandBuffer)
    {
        if (!commandBuffer || !isActive())
            return;

        if (!visible())
			return;
            
        // Setup UI camera for this canvas
        setupCamera(commandBuffer);
        
        // Render all child UI elements
        Element::render(commandBuffer);
    }

    void Canvas::updateCamera()
    {
        if (!_camera)
            return;
            
        // Calculate canvas size using same logic as original UI system
		auto sw = Application::instance()->screenWidth();
		auto sh = Application::instance()->screenHeight();
        auto ws = static_cast<float>(sw) / _referenceSize.x;
        auto hs = static_cast<float>(sh) / _referenceSize.y;
        
        float orthoWidth;
        float orthoHeight;
        if (std::abs(ws - 1.0f) < std::abs(hs - 1.0f))
        {
            orthoWidth = _referenceSize.x;
            orthoHeight = _referenceSize.x * sh / sw;
        }
        else
        {
            orthoHeight = _referenceSize.y;
            orthoWidth = _referenceSize.y * sw / sh;
        }
        
        _canvasSize = vec2(orthoWidth, orthoHeight);
        
        // Update our bounds to match the new canvas size
        _bounds = noz::Rect(0, 0, _canvasSize.x, _canvasSize.y);
		_style.width = _canvasSize.x;
		_style.height = _canvasSize.y;
        
        // Setup orthographic camera for UI rendering
        _camera->setOrthographic(0, orthoWidth, orthoHeight, 0, -1.0f, 1.0f);
        _camera->forceMatrixUpdate();
    }

    void Canvas::setupCamera(noz::renderer::CommandBuffer* commandBuffer)
    {
        if (!_camera)
            return;
            
        // Set view-projection matrix for UI rendering
        commandBuffer->setViewProjection(_camera->viewMatrix(), _camera->projectionMatrix());
    }

    void Canvas::markLayoutDirty(Element* element)
    {
		assert(element);

        // Find the lowest common ancestor that needs to be updated
        if (!_layoutDirtyRoot)
            _layoutDirtyRoot = element;
        else
            _layoutDirtyRoot = findCommonAncestor(_layoutDirtyRoot, element);
    }
    
    void Canvas::markStyleDirty(Element* element)
    {
		assert(element);
            
        // Find the lowest common ancestor that needs style updates
        if (!_styleDirtyRoot)
            _styleDirtyRoot = element;
        else
            _styleDirtyRoot = findCommonAncestor(_styleDirtyRoot, element);

        if (!_styleDirtyRoot)
            _styleDirtyRoot = this;
        
        if (_styleDirtyRoot == this)
			markLayoutDirty(this);
        else
            _styleDirtyRoot->markLayoutDirty();
    }
    
    vec2 Canvas::measureContent(const vec2& availableSize)
    {
        // Canvas measures to its reference size by default
        return _canvasSize;
    }
        
    void Canvas::layoutElements()
    {
		if (nullptr == _layoutDirtyRoot)
			return;

		auto dirtyRoot = _layoutDirtyRoot;
		_layoutDirtyRoot = nullptr;
		dirtyRoot->measure(glm::vec2(_canvasSize.x, _canvasSize.y));
		dirtyRoot->layout(Rect(0,0,_canvasSize.x,_canvasSize.y));
    }
    
    Element* Canvas::findCommonAncestor(Element* a, Element* b)
    {
        if (!a || !b)
            return a ? a : b;
            
        if (a == b)
            return a;
        
        // Use Node's findCommonAncestor method
        auto sharedA = a->as<Node>();
        auto sharedB = b->as<Node>();
        auto commonNode = sharedA->findCommonAncestor(sharedB);
        
        // Cast back to Element (should be safe since we're in UI hierarchy)
        return std::dynamic_pointer_cast<Element>(commonNode).get();
    }

    void Canvas::onAttachedToParent()
    {
        noz::node::Node2d::onAttachedToParent();
    }

    void Canvas::onDetachedFromParent()
    {
        noz::node::Node2d::onDetachedFromParent();
    }

	void Canvas::onAttachToScene()
	{
		Element::onAttachToScene();
		
		// Ensure canvas bounds are set
		updateCamera();		
		
		markStyleDirty();
	}
	
	void Canvas::setStyleSheet(std::shared_ptr<noz::ui::StyleSheet> styleSheet)
	{
		_styleSheet = styleSheet;
		markStyleDirty();
	}
	
	void Canvas::styleElements()
	{
		if (!_styleDirtyRoot)
			return;

		auto dirtyRoot = _styleDirtyRoot;
		_styleDirtyRoot = nullptr;
		dirtyRoot->applyStyle();
	}		
}