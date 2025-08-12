/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/ui/TextEngine.h>

namespace noz::ui
{
	NOZ_DEFINE_TYPEID(Label)

    Label::Label()
        : Element()
        , _text("")
        , _textMeshDirty(true)
    {
        setName("Label");
    }

    void Label::setText(const std::string& text)
    {
		if (_text == text)
			return;

		_text = text;
        markTextMeshDirty();
        markLayoutDirty();
    }

    vec2 Label::measureContent(const vec2& availableSize)
    {
        if (_text.empty())
            return vec2(0.0f, 0.0f);
            
        assert(font());

        return TextEngine::instance()->measureText(_text, font(), style().fontSize.value);
    }

    void Label::renderElement(noz::renderer::CommandBuffer* commandBuffer)
    {
        // Render background first
        Element::renderElement(commandBuffer);
        
        // Render text if we have content
        if (!_text.empty() && commandBuffer)
        {
            ensureTextMesh();
            if (_textMesh)
            {
                renderTextMesh(commandBuffer, _textMesh, bounds(), style().color.value);
            }
        }
    }

    void Label::updateTextMesh()
    {
        if (_text.empty())
        {
            _textMesh.reset();
            return;
        }
        
        assert(font());

        // Create text request
        noz::ui::TextRequest request;
        request.text = _text;
        request.font = font();
        request.fontSize = style().fontSize;
        request.color = style().color.value;
        request.outlineColor = style().textOutlineColor.value;
        request.outlineWidth = style().textOutlineWidth;
        
        _textMesh = TextEngine::instance()->getTextMesh(request);
        _textMeshDirty = false;
    }

    void Label::ensureTextMesh()
    {
        if (_textMeshDirty || !_textMesh)
        {
            updateTextMesh();
        }
    }

    void Label::markTextMeshDirty()
    {
        _textMeshDirty = true;
    }

    void Label::renderTextMesh(
        noz::renderer::CommandBuffer* commandBuffer,
        std::shared_ptr<noz::ui::TextMesh> textMesh,
        const noz::Rect& rect,
        const glm::vec4& color)
    {
        assert(commandBuffer);
        assert(textMesh);

        auto w = textMesh->size.x;
        auto h = textMesh->size.y;
        auto x = rect.x;
        auto y = rect.y;

        commandBuffer->bind(textMesh->fontMaterial);
		commandBuffer->setColor(color);
        commandBuffer->setTransform(glm::translate(glm::vec3(x, y, 0.0f)));
        commandBuffer->setTextOptions(color, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.0f, 0.1f);
        commandBuffer->drawMesh(textMesh->mesh);
    }

    std::shared_ptr<noz::renderer::Font> Label::font() const
    {
        // For now, use a simple font loading approach
        // TODO: Implement proper font management similar to original UI system
        const std::string defaultFontPath = "fonts/Roboto-Black";
        return Asset::load<noz::renderer::Font>(defaultFontPath);
    }

    void Label::applyStyle(const std::string& styleName)
    {
        // Store previous values to detect changes
        float previousFontSize = this->style().fontSize.value;
        Color previousColor = this->style().color.value;
        float previousOutlineWidth = this->style().textOutlineWidth.value;
        Color previousOutlineColor = this->style().textOutlineColor.value;
        
        // Call parent implementation
        Element::applyStyle(styleName);
        
        // Mark text mesh dirty if any text properties changed
        if (this->style().fontSize.value != previousFontSize ||
            this->style().color.value != previousColor ||
            this->style().textOutlineWidth.value != previousOutlineWidth ||
            this->style().textOutlineColor.value != previousOutlineColor)
        {
            markTextMeshDirty();
        }
    }
}