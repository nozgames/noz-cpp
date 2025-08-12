/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/renderer/effects/GammaCorrection.h>

namespace noz::renderer::effects
{
    NOZ_DEFINE_TYPEID(GammaCorrection);

    GammaCorrection::GammaCorrection() 
    {
    }
    
    void GammaCorrection::initialize() 
    {
        FullscreenEffect::initialize(Object::create<Material>("shaders/gamma_correction"));
    }
}
