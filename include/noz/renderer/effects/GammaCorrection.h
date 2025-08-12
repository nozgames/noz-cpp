/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "FullscreenEffect.h"

namespace noz::renderer::effects
{
    class GammaCorrection : public FullscreenEffect 
    {
    public:

        NOZ_DECLARE_TYPEID(GammaCorrection, FullscreenEffect);
        
        virtual ~GammaCorrection() = default;
        
    private:

        GammaCorrection();

        void initialize();
    };
}