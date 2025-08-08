#pragma once

#include "renderer/effects/FullscreenEffect.h"

namespace noz 
{

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
    
    virtual void render(renderer::CommandBuffer* commandBuffer) override;
    
protected:
    Vignette();
    
    virtual void setupShader() override;
    
private:
    void initialize();
    
    float _intensity = 0.8f;
    float _radius = 0.8f;
    float _softness = 0.5f;
};

} // namespace noz