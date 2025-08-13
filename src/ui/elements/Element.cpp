/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/ui/StyleSheet.h>

namespace noz::ui
{
    class Canvas;
    
    NOZ_DEFINE_TYPEID(Element)
    
    std::shared_ptr<noz::renderer::Material> Element::s_uiMaterial;
    std::shared_ptr<noz::renderer::Mesh> Element::s_uiMesh;
    bool Element::s_resourcesInitialized = false;

    Element::Element()
        : noz::node::Node2d()
        , _style(Style::default())
        , _bounds(0, 0, 0, 0)
        , _measuredSize(0.0f, 0.0f)
        , _layoutOffset(0.0f, 0.0f, 0.0f)
        , _flags(ElementFlags::LayoutDirty)
        , _controlId(0)
        , _visible(true)
        , _pseudoState(PseudoState::None)
        , _hasExplicitPseudoState(false)
    {
        setName("Element");
    }
    
    void Element::markLayoutDirty()
    {
        if (hasFlag(_flags, ElementFlags::LayoutDirty))
            return; // Already dirty
            
        _flags |= ElementFlags::LayoutDirty;
        
        // Only propagate to parent if parent has auto-sizing
        auto parentElem = parent<Element>();
        if (parentElem && parentElem->hasAutoSizing())
        {
            parentElem->markLayoutDirty();
			return;
        }
        
        // Notify canvas if we're the root of the dirty subtree
        auto canvas = findParentCanvas();
        if (canvas)
			canvas->markLayoutDirty(this);
    }
    
    std::shared_ptr<Canvas> Element::findParentCanvas()
    {
        return parent<Canvas>();
    }

    void Element::setStyle(const Style& style)
    {
        _style = style;
        markLayoutDirty();
    }
    
	void Element::onNameChanged()
	{
		markStyleDirty();
	}
    
    void Element::markStyleDirty()
    {
        if (hasFlag(_flags, ElementFlags::StyleDirty))
            return; // Already dirty
            
        _flags |= ElementFlags::StyleDirty;
        
        // Notify canvas that this element needs style processing
        auto canvas = findParentCanvas();
        if (canvas)
            canvas->markStyleDirty(this);
    }
    
    void Element::updateParentCanvas()
    {
        auto canvas = findParentCanvas();
        _parentCanvas = canvas;
        
        // If we have a style class name and a canvas, mark style dirty
        if (hasName() && canvas)
            markStyleDirty();
    }
    

    void Element::start()
    {
        noz::node::Node2d::start();
        
        // Initialize shared UI resources if needed
        if (!s_resourcesInitialized)
        {
            initializeUIResources();
        }
    }

    void Element::update()
    {
        noz::node::Node2d::update();
        
        // Note: Layout and measure are now handled by Canvas during update
        // Individual elements don't perform layout directly anymore
    }

    void Element::render(noz::renderer::CommandBuffer* commandBuffer)
    {
		assert(commandBuffer);

        if (!isActive())
            return;
            
        // Only render if visible
        if (_visible)
        {
            // Render this element
            renderElement(commandBuffer);
        }
        
        // Always render children (they have their own visibility)
        noz::node::Node2d::render(commandBuffer);
    }

    vec2 Element::measureContent(const vec2& availableSize)
    {
        // Base implementation returns zero size
        // Derived classes override this to measure their content
        return vec2(0.0f, 0.0f);
    }
    
    void Element::measure(const vec2& availableSize)
    {       
        // Calculate available size for content after padding
        auto contentAvailableSize = availableSize;
        if (_style.paddingLeft.isFixed())
            contentAvailableSize.x -= _style.paddingLeft.evaluate(0);
        if (_style.paddingRight.isFixed())
            contentAvailableSize.x -= _style.paddingRight.evaluate(0);
        if (_style.paddingTop.isFixed())
            contentAvailableSize.y -= _style.paddingTop.evaluate(0);
        if (_style.paddingBottom.isFixed())
            contentAvailableSize.y -= _style.paddingBottom.evaluate(0);

		auto measuredContentSize = measureContent(contentAvailableSize);

		// Children get the content size after padding as their available size
		auto childAvailableSize = contentAvailableSize;

		// Now measure the children
		auto measuredChildSize = glm::vec2(0.0f, 0.0f);
		for (size_t i = 0; i < childCount(); ++i)
		{
			auto child = this->child(i)->as<Element>();
			if (!child)
				continue;

			child->measure(childAvailableSize);

			// Calculate child size including margins for parent's auto-sizing
			float childWidthWithMargins = child->_measuredSize.x;
			float childHeightWithMargins = child->_measuredSize.y;
			
			// Add margins to child size for parent calculations
			if (child->_style.marginLeft.isFixed())
				childWidthWithMargins += child->_style.marginLeft.evaluate(childAvailableSize.x);
			if (child->_style.marginRight.isFixed())
				childWidthWithMargins += child->_style.marginRight.evaluate(childAvailableSize.x);
			if (child->_style.marginTop.isFixed())
				childHeightWithMargins += child->_style.marginTop.evaluate(childAvailableSize.y);
			if (child->_style.marginBottom.isFixed())
				childHeightWithMargins += child->_style.marginBottom.evaluate(childAvailableSize.y);

			if (_style.width.isAuto())
			{
				if (_style.flexDirection == FlexDirection::Column ||
					_style.flexDirection == FlexDirection::ColumnReverse)
					measuredChildSize.x += childWidthWithMargins;  // Column = horizontal, so sum widths with margins
				else
					measuredChildSize.x = math::max(childWidthWithMargins, measuredChildSize.x);  // Row = vertical, so max width with margins
			}

			if (_style.height.isAuto())
			{
				if (_style.flexDirection == FlexDirection::Row ||
					_style.flexDirection == FlexDirection::RowReverse)
					measuredChildSize.y += childHeightWithMargins;  // Row = vertical, so sum heights with margins
				else
					measuredChildSize.y = math::max(childHeightWithMargins, measuredChildSize.y);  // Column = horizontal, so max height with margins
			}
		}

		// According to ElementLayout.md: _measuredSize should NOT include margins
		_measuredSize = vec2(0.0f, 0.0f);
		if (_style.width.isAuto())
		{
			_measuredSize.x = std::max(measuredContentSize.x, measuredChildSize.x);
			// Add padding to the measured size
			if (_style.paddingLeft.isFixed())
				_measuredSize.x += _style.paddingLeft.evaluate(availableSize.x);
			if (_style.paddingRight.isFixed())
				_measuredSize.x += _style.paddingRight.evaluate(availableSize.x);
		}
		else
			_measuredSize.x = _style.width.evaluate(availableSize.x);

		if (_style.height.isAuto())
		{
			_measuredSize.y = std::max(measuredContentSize.y, measuredChildSize.y);
			// Add padding to the measured size
			if (_style.paddingTop.isFixed())
				_measuredSize.y += _style.paddingTop.evaluate(availableSize.y);
			if (_style.paddingBottom.isFixed())
				_measuredSize.y += _style.paddingBottom.evaluate(availableSize.y);
		}
		else
			_measuredSize.y = _style.height.evaluate(availableSize.y);

		// Do NOT add margins to _measuredSize - they are handled separately in layout
    }
    
	void Element::layout(const Rect& parentBounds)
    {
		_flags &= ~ElementFlags::LayoutDirty;

        float hmin = 0.0f;
        float hmax = 0.0f;
        float hsize = 0.0f;
		layoutAxis(
            _style.marginLeft,
            _style.marginRight, 
            _style.width,
            _measuredSize.x,
			parentBounds.width,
            hmin,
            hmax,
            hsize);

        float vmin = 0.0f;
        float vmax = 0.0f;
        float vsize = 0.0f;
		layoutAxis(
            _style.marginTop,
            _style.marginBottom, 
            _style.height,
            _measuredSize.y,
			parentBounds.height,
            vmin,
            vmax,
            vsize);

		_bounds = noz::Rect(
			parentBounds.x + hmin,
			parentBounds.y + vmin,
			hsize,
			vsize);

		// Calculate content area after padding (relative to element bounds)
		float contentLeft = 0;
		float contentTop = 0;
		float contentWidth = hsize;
		float contentHeight = vsize;
		
		if (_style.paddingLeft.isFixed())
		{
			float paddingValue = _style.paddingLeft.evaluate(0);
			contentLeft += paddingValue;
			contentWidth -= paddingValue;
		}
		if (_style.paddingRight.isFixed())
		{
			contentWidth -= _style.paddingRight.evaluate(0);
		}
		if (_style.paddingTop.isFixed())
		{
			float paddingValue = _style.paddingTop.evaluate(0);
			contentTop += paddingValue;
			contentHeight -= paddingValue;
		}
		if (_style.paddingBottom.isFixed())
		{
			contentHeight -= _style.paddingBottom.evaluate(0);
		}

		// Two-pass flex layout for proper auto margin handling
		// Pass in the world-space content bounds
		layoutChildrenWithFlexMargins(
			_bounds.x + contentLeft, 
			_bounds.y + contentTop, 
			contentWidth, 
			contentHeight);
    }

    void Element::layoutChildrenWithFlexMargins(float contentLeft, float contentTop, float contentWidth, float contentHeight)
    {
        size_t count = childCount();
        if (count == 0)
            return;
            
        // Column = horizontal layout, Row = vertical layout
        bool isHorizontalLayout = (_style.flexDirection == FlexDirection::Column || _style.flexDirection == FlexDirection::ColumnReverse);
        bool isReverse = (_style.flexDirection == FlexDirection::RowReverse || _style.flexDirection == FlexDirection::ColumnReverse);
        
        // Pass 1: Calculate total intrinsic size and count auto margins
        float totalIntrinsicSize = 0.0f;
        int autoMarginCount = 0;
        
        for (size_t i = 0; i < count; ++i)
        {
            auto child = this->child(i)->as<Element>();
            if (!child)
                continue;
                
            // Add child's measured size (which does NOT include margins)
            totalIntrinsicSize += isHorizontalLayout ? child->_measuredSize.x : child->_measuredSize.y;
            
            // Add fixed margins to total size and count auto margins
            if (isHorizontalLayout)
            {
                if (child->_style.marginLeft.isAuto())
                    autoMarginCount++;
                else
                    totalIntrinsicSize += child->_style.marginLeft.evaluate(contentWidth);
                    
                if (child->_style.marginRight.isAuto())
                    autoMarginCount++;
                else
                    totalIntrinsicSize += child->_style.marginRight.evaluate(contentWidth);
            }
            else
            {
                if (child->_style.marginTop.isAuto())
                    autoMarginCount++;
                else
                    totalIntrinsicSize += child->_style.marginTop.evaluate(contentHeight);
                    
                if (child->_style.marginBottom.isAuto())
                    autoMarginCount++;
                else
                    totalIntrinsicSize += child->_style.marginBottom.evaluate(contentHeight);
            }
        }
        
        // Pass 2: Calculate auto margin size
        float mainAxisSize = isHorizontalLayout ? contentWidth : contentHeight;
        float remainingSpace = mainAxisSize - totalIntrinsicSize;
        float autoMarginSize = (autoMarginCount > 0 && remainingSpace > 0) ? remainingSpace / autoMarginCount : 0.0f;
        
        // Pass 3: Layout children with resolved margins
        float currentOffset = 0.0f;
        
        // Set up iteration based on direction
        size_t childIndex = isReverse ? (count - 1) : 0;
        int increment = isReverse ? -1 : 1;
        
        for (size_t i = 0; i < count; ++i)
        {
            auto child = this->child(childIndex)->as<Element>();
            if (!child)
            {
                childIndex += increment;
                continue;
            }
            
            // Calculate resolved margins
            float marginStart, marginEnd;
            if (isHorizontalLayout)
            {
                marginStart = child->_style.marginLeft.isAuto() ? autoMarginSize : child->_style.marginLeft.evaluate(contentWidth);
                marginEnd = child->_style.marginRight.isAuto() ? autoMarginSize : child->_style.marginRight.evaluate(contentWidth);
            }
            else
            {
                marginStart = child->_style.marginTop.isAuto() ? autoMarginSize : child->_style.marginTop.evaluate(contentHeight);
                marginEnd = child->_style.marginBottom.isAuto() ? autoMarginSize : child->_style.marginBottom.evaluate(contentHeight);
            }
            
            // Don't add margin to offset - give child the full space including margins
            // The child will position itself within this space using its own margins
            
            // Create child bounds that include space for margins
            Rect childBounds;
            if (isHorizontalLayout)
            {
                float childTotalWidth = marginStart + child->_measuredSize.x + marginEnd;
                childBounds = Rect(
                    contentLeft + currentOffset,
                    contentTop,
                    childTotalWidth,
                    contentHeight);
                currentOffset += childTotalWidth;
            }
            else
            {
                float childTotalHeight = marginStart + child->_measuredSize.y + marginEnd;
                childBounds = Rect(
                    contentLeft,
                    contentTop + currentOffset,
                    contentWidth,
                    childTotalHeight);
                currentOffset += childTotalHeight;
            }
            
            child->layout(childBounds);
            
            // Move to next child
            childIndex += increment;
        }
    }

    void Element::layoutAxis(
        const StyleLength& marginMin,
        const StyleLength& marginMax,
        const StyleLength& size,
        float measuredSize,
        float availableSpace,
        float& resolvedMarginMin,
        float& resolvedMarginMax,
        float& resolvedSize)
    {
        resolvedMarginMin = 0;
        resolvedMarginMax = 0;
        resolvedSize = 0;

        // Calculate the total contribution of auto margins and size (treated as flex(1))
        float contrib = 0.0f;
        if (marginMin.isAuto())
            contrib += 1.0f;
        else
            resolvedMarginMin = marginMin.evaluate(availableSpace);

        if (marginMax.isAuto())
            contrib += 1.0f;
        else
            resolvedMarginMax = marginMax.evaluate(availableSpace);

        if (size.isAuto())
            resolvedSize = measuredSize;
        else
            resolvedSize = size.evaluate(availableSpace);

        availableSpace -= resolvedMarginMin + resolvedMarginMax + resolvedSize;

        if (contrib > 0.0f)
        {
            contrib = 1.0f / contrib;
            if (marginMin.isAuto())
                resolvedMarginMin = 1.0f * contrib * availableSpace;
            if (marginMax.isAuto())
                resolvedMarginMax = 1.0f * contrib * availableSpace;
        }
    }

    void Element::renderElement(noz::renderer::CommandBuffer* commandBuffer)
    {
        if (!commandBuffer)
            return;
            
        // Render background
        renderBackground(commandBuffer);
        
        // Render border
        renderBorder(commandBuffer);
    }

    void Element::renderBackground(noz::renderer::CommandBuffer* commandBuffer)
    {
        // Render background color if specified
        if (_style.backgroundColor.value.a > 0.0f)
        {
            if (_style.borderRadius > 0.001f)
            {
                // TODO: Implement rounded rectangle rendering
                // For now, draw as regular rectangle
                renderQuad(commandBuffer, _bounds.x, _bounds.y, _bounds.width, _bounds.height, _style.backgroundColor.value);
            }
            else
            {
                // Draw regular rectangle
                renderQuad(commandBuffer, _bounds.x, _bounds.y, _bounds.width, _bounds.height, _style.backgroundColor.value);
            }
        }
    }

    void Element::renderBorder(noz::renderer::CommandBuffer* commandBuffer)
    {
        if (_style.borderColor.value.a > 0.0f && _style.borderWidth > 0.001f)
        {
            // TODO: Implement border rendering
        }
    }

    void Element::renderQuad(noz::renderer::CommandBuffer* commandBuffer,
                            float x, float y, float width, float height,
                            const glm::vec4& color)
    {
        assert(s_uiMaterial);
        assert(s_uiMesh);

        // Create transform matrix for quad positioning and scaling
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(x, y, 0.0f));
        transform = glm::scale(transform, glm::vec3(width, height, 1.0f));

        commandBuffer->bind(s_uiMaterial);
        commandBuffer->setTransform(transform);
        commandBuffer->bindColor(color);
        commandBuffer->drawMesh(s_uiMesh);
    }

    void Element::renderQuad(
        noz::renderer::CommandBuffer* commandBuffer,
        float x,
        float y,
        float width,
        float height,
        const std::shared_ptr<noz::renderer::Texture>& texture)
    {
        assert(s_uiMaterial);
		assert(s_uiMesh);

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(x, y, 0.0f));
        transform = glm::scale(transform, glm::vec3(width, height, 1.0f));

        commandBuffer->bind(s_uiMaterial);
        commandBuffer->setTransform(transform);
        commandBuffer->bindColor(glm::vec4(1.0f)); // White color for no tinting
        commandBuffer->drawMesh(s_uiMesh);
    }

    void Element::initializeUIResources()
    {
        if (s_resourcesInitialized)
            return;

        // Create basic UI shader
		s_uiMaterial = Object::create<noz::renderer::Material>("shaders/ui");
        assert(s_uiMaterial);

        // Create quad mesh for UI elements
        createQuadMesh();
        
        s_resourcesInitialized = true;
    }

    void Element::createQuadMesh()
    {
        // Create a unit quad mesh using MeshBuilder
        noz::renderer::MeshBuilder builder;
        
        // Define quad vertices (unit square from 0,0 to 1,1)
        builder.addVertex(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f));
        builder.addVertex(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f));
        builder.addVertex(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f));
        builder.addVertex(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f));
        
        // Define quad indices (two triangles)
        builder.addTriangle(0, 1, 2);
        builder.addTriangle(0, 2, 3);
        
        // Create mesh from builder data
		s_uiMesh = builder.toMesh("UI");
        s_uiMesh->upload(true);
    }

	void Element::onAttachToScene()
	{
		noz::node::Node2d::onAttachToScene();
	
		updateParentCanvas();
		markLayoutDirty();
	}
	
	void Element::applyStyle(const std::string& styleName)
	{
		if (_parentCanvas.expired())
			return;

		if (styleName.empty())
			return;

		auto styleSheet = _parentCanvas.lock()->styleSheet();
		if (!styleSheet)
			return;

		// Try to get pseudo state style first (e.g., "button:hover")
		PseudoState effectiveState = effectivePseudoState();
		if (effectiveState != PseudoState::None)
		{
			std::string pseudoStyleName = styleName + ":" + pseudoStateToString(effectiveState);
			if (styleSheet->hasStyle(pseudoStyleName))
			{
				_style = styleSheet->resolveStyle(pseudoStyleName);
				return;
			}
		}
		
		// Fall back to base style
		_style = styleSheet->resolveStyle(styleName);
	}

	void Element::applyStyle()
	{
		_flags &= ~ElementFlags::StyleDirty;

		applyStyle(name());

		// Apply to all children as well
		for (size_t i = 0; i < childCount(); ++i)
		{
			auto child = this->child(i)->as<Element>();;
			if (!child)
				continue;
			child->applyStyle();
		}
	}
	
	void Element::setPseudoState(PseudoState state, bool cascade)
	{
		if (_pseudoState == state)
			return;
			
		_pseudoState = state;
		_hasExplicitPseudoState = (state != PseudoState::None);
		
		// Reapply style to pick up the pseudo state
		markStyleDirty();
		
		// Cascade to children if requested
		if (cascade)
		{
			for (size_t i = 0; i < childCount(); ++i)
			{
				auto child = this->child(i)->as<Element>();
				if (!child)
					continue;
					
				// Only cascade to children that don't have their own explicit pseudo state
				if (!child->_hasExplicitPseudoState)
				{
					child->markStyleDirty(); // Trigger style refresh to pick up new effective state
				}
			}
		}
	}
	
	PseudoState Element::effectivePseudoState() const
	{
		// If this element has an explicit pseudo state, use it
		if (_hasExplicitPseudoState)
			return _pseudoState;
		
		// Otherwise, inherit from parent Element
		auto parentElem = parent<Element>();
		if (parentElem)
			return parentElem->effectivePseudoState();
		
		// No parent or parent pseudo state, use None
		return PseudoState::None;
	}
}