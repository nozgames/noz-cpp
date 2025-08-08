/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "FullscreenEffect.h"
#include <glm/glm.hpp>

namespace noz::renderer::effects
{
    // Buffer structure matching the shader's VignetteBuffer
    // Must match HLSL packing rules: float3 + float = 16 bytes, then next 16-byte aligned block
    struct alignas(16) VignetteBuffer 
    {
        float vignetteColorR;     // Individual components to avoid packing issues
        float vignetteColorG; 
        float vignetteColorB;
        float intensity;          // This completes first 16-byte block
        float radius;            // Start of second 16-byte block
        float softness;
        float padding1;          // Padding to complete 16-byte alignment
        float padding2;
    };

    class Vignette : public FullscreenEffect 
    {
    public:

        NOZ_DECLARE_TYPEID(Vignette, FullscreenEffect);
    
        virtual ~Vignette() = default;
    
        void setIntensity(float intensity);
        float getIntensity() const { return _intensity; }
    
        void setRadius(float radius);
        float getRadius() const { return _radius; }
    
        void setSoftness(float softness);
        float getSoftness() const { return _softness; }
        
        void setColor(const glm::vec3& color);
        glm::vec3 getColor() const { return _color; }
    
    protected:
        Vignette();
    
        virtual void setupShader() override;
        
        virtual void render(CommandBuffer* commandBuffer) override;
    
    private:
        void initialize();
    
        float _intensity = 0.8f;
        float _radius = 0.8f;
        float _softness = 0.5f;
        glm::vec3 _color = glm::vec3(0.0f, 0.0f, 0.0f); // Default to black vignette
    };
}