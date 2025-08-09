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
    
    void BorderEffect::initialize() 
    {
        FullscreenEffect::initialize();
        _shader = Asset::load<Shader>("shaders/border_effect");
    }
    
    void BorderEffect::setBorderParams(const BorderParams& params)
    {
        _params = params;
        _needsUniformUpdate = true;
    }
    
    void BorderEffect::render(CommandBuffer* commandBuffer)
    {
        assert(_mesh);
        assert(_shader);
        assert(_inputTexture);

        commandBuffer->setCamera(glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f));
        commandBuffer->setTransform(glm::mat4(1.0f));
        commandBuffer->bindShader(_shader);
        commandBuffer->bindTextureWithSampler(_inputTexture, Renderer::instance()->linearSampler());
        
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
        
        // Set the buffer data for b0 register in space3 (BorderBuffer)
        commandBuffer->setBufferData(0, &bufferData, sizeof(BorderBuffer));
        
        // Draw the fullscreen quad
        commandBuffer->drawMesh(_mesh);
    }
}