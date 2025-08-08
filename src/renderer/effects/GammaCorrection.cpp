#include <noz/renderer/effects/GammaCorrection.h>
#include <noz/renderer/Shader.h>
#include <noz/renderer/CommandBuffer.h>
#include <noz/Asset.h>

namespace noz::renderer::effects
{
    NOZ_DEFINE_TYPEID(GammaCorrection);

    GammaCorrection::GammaCorrection() 
    {
    }
    
    void GammaCorrection::initialize() 
    {
        FullscreenEffect::initialize();
        _shader = Asset::load<Shader>("shaders/gamma_correction");
    }
}