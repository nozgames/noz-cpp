/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "Element.h"

namespace noz::ui
{
    class StyleSheet;
}

namespace noz::renderer
{
    class CommandBuffer;
}

namespace noz::node
{
    class Camera;
}

namespace noz::ui
{
    class Canvas : public Element
    {
    public:

		NOZ_DECLARE_TYPEID(Canvas, Element);

        virtual ~Canvas() = default;

        // Canvas properties
        void setSortOrder(int sortOrder) { _sortOrder = sortOrder; }
        int sortOrder() const { return _sortOrder; }
        
        void setReferenceSize(const vec2& size) { _referenceSize = size; }
        const vec2& referenceSize() const { return _referenceSize; }
                
        // Camera access for UI elements
        const noz::node::Camera& uiCamera() const { return *_camera; }
        
        // Canvas size in UI coordinates
        const vec2& canvasSize() const { return _canvasSize; }
        
		void markLayoutDirty();
        void markLayoutDirty(Element* element);

		void markStyleDirty();
        void markStyleDirty(Element* element);
        
        // StyleSheet management
        void setStyleSheet(std::shared_ptr<noz::ui::StyleSheet> styleSheet);
        std::shared_ptr<noz::ui::StyleSheet> styleSheet() const { return _styleSheet; }
                
        // Node lifecycle
        void update() override;
        void render(noz::renderer::CommandBuffer* commandBuffer) override;
        void start() override;

    protected:

        Canvas();

		void initialize() override;

        void onAttachedToParent() override;
        void onDetachedFromParent() override;
		void onAttachToScene() override;
        
        // Override Element methods for Canvas-specific behavior
        vec2 measureContent(const vec2& availableSize) override;

    private:

        int _sortOrder = 0;
        vec2 _referenceSize = vec2(1920.0f, 1080.0f);
        vec2 _canvasSize;
        std::shared_ptr<noz::ui::StyleSheet> _styleSheet;        
        std::shared_ptr<noz::node::Camera> _camera;
        
        // Layout invalidation tracking
        Element* _layoutDirtyRoot = nullptr;
        
        // Style invalidation tracking
        Element* _styleDirtyRoot = nullptr;
        
        // Screen size tracking for resize detection
        int _lastScreenWidth = 0;
        int _lastScreenHeight = 0;
        
        void updateCamera();
        void setupCamera(noz::renderer::CommandBuffer* commandBuffer);
        void styleElements();
        void layoutElements();
        Element* findCommonAncestor(Element* a, Element* b);
    };

	inline void Canvas::markLayoutDirty()
	{
		markLayoutDirty(this);
	}

	inline void Canvas::markStyleDirty()
	{
		markStyleDirty(this);
	}
}