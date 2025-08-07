/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::ui
{
    Image::Image()
        : Element()
        , _texture(nullptr)
        , _sourceRect(0, 0, 0, 0) // Empty rect means use full texture
    {
        setName("Image");
    }

    void Image::setTexture(const std::shared_ptr<noz::renderer::Texture>& texture)
    {
        if (_texture != texture)
        {
            _texture = texture;
            markLayoutDirty();
        }
    }

    void Image::setTexture(const std::string& texturePath)
    {
        // Load the texture using the resource system
        auto texture = noz::Resources::instance()->load<noz::renderer::Texture>(texturePath);
        if (!texture)
        {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            return;
        }
        
        setTexture(texture);
    }

    vec2 Image::measureContent(const vec2& availableSize)
    {
        if (!_texture)
            return vec2(0.0f, 0.0f);
        
        // If source rect is specified, use it
        if (_sourceRect.width > 0 && _sourceRect.height > 0)
        {
            return vec2(_sourceRect.width, _sourceRect.height);
        }
        
        // Otherwise use texture dimensions
        return vec2(static_cast<float>(_texture->width()), static_cast<float>(_texture->height()));
    }

    void Image::renderElement(noz::renderer::CommandBuffer* commandBuffer)
    {
        // Render background first
        Element::renderElement(commandBuffer);
        
        // Render image if we have a texture
        if (_texture && commandBuffer)
        {
            renderImage(commandBuffer, bounds());
        }
    }

    void Image::renderImage(noz::renderer::CommandBuffer* commandBuffer, const noz::Rect& rect)
    {
        if (!_texture)
            return;

        noz::Rect source = _sourceRect;
        
        // If source is empty, use full texture dimensions
        if (source.width <= 0 || source.height <= 0)
        {
            source = noz::Rect(0, 0, static_cast<float>(_texture->width()), static_cast<float>(_texture->height()));
        }

        // Calculate destination rectangle based on scale mode
        noz::Rect dest = calculateDestinationRect(rect, source);

        // Apply image flip if specified in style
        if (style().imageFlip)
        {
            // TODO: Implement proper texture flipping
            // For now, just render normally
        }
        
        // Draw the image using the inherited renderQuad method
        renderQuad(commandBuffer, dest.x, dest.y, dest.width, dest.height, _texture);
    }

    noz::Rect Image::calculateDestinationRect(const noz::Rect& bounds, const noz::Rect& sourceRect) const
    {
        noz::Rect dest = bounds;
        
        switch (style().imageScaleMode)
        {
        case ScaleMode::ScaleToFit:
            {
                if (sourceRect.width > sourceRect.height)
                {
                    float aspect = sourceRect.height / sourceRect.width;
                    float height = aspect * dest.width;
                    dest.y += (dest.height - height) * 0.5f;
                    dest.height = height;
                }
                else
                {
                    float aspect = sourceRect.width / sourceRect.height;
                    float width = aspect * dest.height;
                    dest.x += (dest.width - width) * 0.5f;
                    dest.width = width;
                }
            }
            break;
            
        case ScaleMode::StretchToFill:
            // Use bounds as-is (default case)
            break;
            
        case ScaleMode::ScaleAndCrop:
            // TODO: Implement scale and crop mode
            break;
        }
        
        return dest;
    }

	void Image::setSourceRect(const noz::Rect& sourceRect)
	{
		_sourceRect = sourceRect;
		markLayoutDirty();
	}
}
