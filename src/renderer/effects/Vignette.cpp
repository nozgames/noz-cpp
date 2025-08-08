#include <noz/renderer/effects/Vignette.h>
#include <noz/renderer/Shader.h>
#include <noz/renderer/CommandBuffer.h>
#include <noz/Asset.h>
#include <glm/glm.hpp>

namespace noz::renderer::effects
{
    NOZ_DEFINE_TYPEID(Vignette);

    Vignette::Vignette() 
    {
    }
    
    void Vignette::initialize() 
    {
        FullscreenEffect::initialize();
        setupShader();
    }

    void Vignette::setupShader() 
    {
        _shader = Asset::load<Shader>("shaders/vignette");
    }

    void Vignette::setIntensity(float intensity) 
    {
        _intensity = glm::clamp(intensity, 0.0f, 1.0f);
    }

    void Vignette::setRadius(float radius) 
    {
        _radius = glm::clamp(radius, 0.0f, 2.0f);
    }

    void Vignette::setSoftness(float softness) 
    {
        _softness = glm::clamp(softness, 0.0f, 1.0f);
    }
    
    void Vignette::setColor(const glm::vec3& color)
    {
        _color = color;
    }

    void Vignette::render(CommandBuffer* commandBuffer) 
    {
        if (!_mesh || !_shader) 
        {
            return;
        }
        
        // Set up for fullscreen rendering
        commandBuffer->setCamera(glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f));
        commandBuffer->setTransform(glm::mat4(1.0f));
        
        // Bind shader
        commandBuffer->bindShader(_shader);
        
        // Create and set vignette buffer data
        VignetteBuffer bufferData;
        bufferData.vignetteColorR = _color.r;
        bufferData.vignetteColorG = _color.g;
        bufferData.vignetteColorB = _color.b;
        bufferData.intensity = _intensity;
        bufferData.radius = _radius;
        bufferData.softness = _softness;
        bufferData.padding1 = 0.0f;
        bufferData.padding2 = 0.0f;
        
        // Set the buffer data for b1 register (VignetteBuffer)
        commandBuffer->setBufferData(0, &bufferData, sizeof(VignetteBuffer));
        
        // Draw the fullscreen quad
        commandBuffer->drawMesh(_mesh);
    }
}