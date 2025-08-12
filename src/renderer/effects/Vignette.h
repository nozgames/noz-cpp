/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "renderer/effects/FullscreenEffect.h"

namespace noz::renderer::effects
{
    class Vignette : public FullscreenEffect 
    {
    public:

        NOZ_DECLARE_TYPEID(Vignette, FullscreenEffect);
    
        virtual ~Vignette() = default;
    
        void setIntensity(float intensity);
        float intensity() const { return _intensity; }
    
        void setRadius(float radius);
        float radius() const { return _radius; }
    
        void setSoftness(float softness);
        float softness() const { return _softness; }
    
        virtual void render(renderer::CommandBuffer* commandBuffer) override;   
    
    private:

        Vignette();

        void initialize(const std::shared_ptr<Texture>& texture);
        
        float _intensity = 0.8f;
        float _radius = 0.8f;
        float _softness = 0.5f;
    };
}
