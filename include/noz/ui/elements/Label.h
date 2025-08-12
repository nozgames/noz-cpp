/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::renderer
{
    class Font;
}

namespace noz::ui
{
	struct TextMesh;

    class Label : public Element
    {
    public:

		NOZ_DECLARE_TYPEID(Label, Element);

		Label();
		virtual ~Label() = default;

        void setText(const std::string& text);
        const std::string& text() const { return _text; }
        
        void applyStyle(const std::string& styleName) override;

    protected:
        // Override measurement to account for text size
        vec2 measureContent(const vec2& availableSize) override;
        
        // Override rendering to draw text
        void renderElement(noz::renderer::CommandBuffer* commandBuffer) override;

    private:

		std::string _text;
        std::shared_ptr<noz::ui::TextMesh> _textMesh;
        bool _textMeshDirty;
                
        void updateTextMesh();
        void ensureTextMesh();
        void markTextMeshDirty();
        void renderTextMesh(noz::renderer::CommandBuffer* commandBuffer,
                           std::shared_ptr<noz::ui::TextMesh> textMesh,
                           const noz::Rect& rect,
                           const glm::vec4& color);

        std::shared_ptr<noz::renderer::Font> font() const;
    };
}