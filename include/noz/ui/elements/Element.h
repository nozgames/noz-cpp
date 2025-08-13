/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/ui/PseudoState.h>

namespace noz::renderer
{
    class CommandBuffer;
    class Shader;
    class Mesh;
    class Texture;
}

namespace noz::ui
{
    class Canvas;
	struct StyleLength;
    
    class Element : public noz::node::Node2d
    {
    public:
        
		NOZ_DECLARE_TYPEID(Element, noz::node::Node2d)
        
        virtual ~Element() = default;

        // Style management
        void setStyle(const Style& style);
        const Style& style() const { return _style; }
                
        // Layout system
        const Rect& bounds() const { return _bounds; }
        const vec2& measuredSize() const { return _measuredSize; }
        
        void markLayoutDirty();
        bool isLayoutDirty() const { return hasFlag(_flags, ElementFlags::LayoutDirty); }
        
        void markStyleDirty();
        bool isStyleDirty() const { return hasFlag(_flags, ElementFlags::StyleDirty); }
        
        // Canvas management
        std::shared_ptr<Canvas> findParentCanvas();
        void updateParentCanvas();
        
        void applyStyle();
		virtual void applyStyle(const std::string& styleName);
        
        // Check if style has auto-sizing
        bool hasAutoWidth() const { return _style.width.isAuto(); }
        bool hasAutoHeight() const { return _style.height.isAuto(); }
        bool hasAutoSizing() const { return hasAutoWidth() || hasAutoHeight(); }
        
        // Control ID for input handling
        void setControlId(int controlId) { _controlId = controlId; }
        int controlId() const { return _controlId; }
        
        // Visibility control (affects rendering but not layout)
        void setVisible(bool visible) { _visible = visible; }
        bool visible() const { return _visible; }
        
        // Pseudo state management
        void setPseudoState(PseudoState state, bool cascade = true);
        PseudoState pseudoState() const { return _pseudoState; }
        
        // Get the effective pseudo state (considering parent cascading)
        PseudoState effectivePseudoState() const;
        
        // Node lifecycle
        void update() override;
        void render(noz::renderer::CommandBuffer* commandBuffer) override;
        void start() override;

    protected:

        Element();

		virtual vec2 measureContent(const vec2& availableSize);
        void measure(const vec2& availableSize);
        void layout(const Rect& parentBounds);
        void layoutAxis(
            const StyleLength& marginMin,
            const StyleLength& marginMax,
            const StyleLength& size,
            float measuredSize,
            float availableSpace,
            float& resolvedMarginMin,
            float& resolvedMarginMax,
            float& resolvedSize);
        
        void layoutChildrenWithFlexMargins(float contentLeft, float contentTop, float contentWidth, float contentHeight);
        
        // Friend class for layout access
        friend class Canvas;
        
        // Rendering methods
        virtual void renderElement(noz::renderer::CommandBuffer* commandBuffer);
        void renderBackground(noz::renderer::CommandBuffer* commandBuffer);
        void renderBorder(noz::renderer::CommandBuffer* commandBuffer);
        
        // UI rendering helpers
        void renderQuad(noz::renderer::CommandBuffer* commandBuffer,
                       float x, float y, float width, float height,
                       const glm::vec4& color);
        void renderQuad(noz::renderer::CommandBuffer* commandBuffer,
                       float x, float y, float width, float height,
                       const std::shared_ptr<noz::renderer::Texture>& texture);

		void onAttachToScene() override;

		void onNameChanged() override;

    private:

        Style _style;
        std::weak_ptr<Canvas> _parentCanvas;
        noz::Rect _bounds;
        vec2 _measuredSize;
        vec3 _layoutOffset;
        ElementFlags _flags;
        int _controlId;
        bool _visible;
        PseudoState _pseudoState;
        bool _hasExplicitPseudoState;
        
        static std::shared_ptr<noz::renderer::Material> s_uiMaterial;
        static std::shared_ptr<noz::renderer::Mesh> s_uiMesh;
        static bool s_resourcesInitialized;
        
        static void initializeUIResources();
        static void createQuadMesh();
        static void createWhiteTexture();
    };
}