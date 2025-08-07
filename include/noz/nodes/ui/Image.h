/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::renderer
{
    class Texture;
}

namespace noz::ui
{
    /**
     * @brief Image node for texture/sprite rendering
     * Supports different scale modes and texture regions
     */
    class Image : public Element
    {
    public:
        Image();
        virtual ~Image() = default;

        // Texture management
        void setTexture(const std::shared_ptr<noz::renderer::Texture>& texture);
        void setTexture(const std::string& texturePath);
        const std::shared_ptr<noz::renderer::Texture>& texture() const { return _texture; }
        
		void setSourceRect(const noz::Rect& sourceRect);
        const noz::Rect& sourceRect() const { return _sourceRect; }

    protected:
        // Override measurement to account for texture size
        vec2 measureContent(const vec2& availableSize) override;
        
        // Override rendering to draw texture
        void renderElement(noz::renderer::CommandBuffer* commandBuffer) override;

    private:
        std::shared_ptr<noz::renderer::Texture> _texture;
        noz::Rect _sourceRect; // Empty rect means use full texture
        
        void renderImage(noz::renderer::CommandBuffer* commandBuffer, const noz::Rect& rect);
        noz::Rect calculateDestinationRect(const noz::Rect& bounds, const noz::Rect& sourceRect) const;
    };
}