/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/renderer/effects/Vignette.h>

namespace noz::renderer::effects
{
    NOZ_DEFINE_TYPEID(Vignette);

    Vignette::Vignette() 
    {
    }
    
    void Vignette::initialize()
    {
        FullscreenEffect::initialize(Object::create<Material>("shaders/vignette"));
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
        assert(commandBuffer);
        assert(material());
        
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

		material()->setFragmentUniformData(0, &bufferData, sizeof(VignetteBuffer));

		FullscreenEffect::render(commandBuffer);
    }
}