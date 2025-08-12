/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/renderer/effects/BorderEffect.h>
#include <noz/renderer/Shader.h>
#include <noz/renderer/CommandBuffer.h>
#include <noz/renderer/Renderer.h>
#include <noz/Asset.h>

namespace noz::renderer::effects
{
    NOZ_DEFINE_TYPEID(BorderEffect);

    BorderEffect::BorderEffect() 
    {
    }
    
    void BorderEffect::initialize(const std::shared_ptr<Texture>& texture) 
    {
		auto material = Object::create<Material>("shaders/border_effect");
		material->setTexture(texture);
        FullscreenEffect::initialize(material);
    }
    
    void BorderEffect::setBorderParams(const BorderParams& params)
    {
        _params = params;
        _needsUniformUpdate = true;
    }
    
    void BorderEffect::render(CommandBuffer* commandBuffer)
    {
        assert(commandBuffer);
        assert(material());
        
        struct alignas(16) BorderBuffer
        {
            float borderWidth;
            float borderColorR;
            float borderColorG; 
            float borderColorB;
            float borderAlpha;
            float textureWidth;
            float textureHeight;
            float padding;
        };        
        
        BorderBuffer bufferData;
        bufferData.borderWidth = _params.width;
        bufferData.borderColorR = _params.color.r;
        bufferData.borderColorG = _params.color.g;
        bufferData.borderColorB = _params.color.b;
        bufferData.borderAlpha = _params.color.a;
        bufferData.textureWidth = _params.textureWidth;
        bufferData.textureHeight = _params.textureHeight;
        bufferData.padding = 0.0f;
        
		material()->setFragmentUniformData(0, &bufferData, sizeof(BorderBuffer));

		FullscreenEffect::render(commandBuffer);
    }
}