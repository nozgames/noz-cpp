/*

    NoZ Game Engine

    Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "FullscreenEffect.h"

namespace noz::renderer::effects
{
    struct BorderParams
    {
        float width = 4.0f;
        noz::Color color = noz::Color::Black;
        float textureWidth = 256.0f;
        float textureHeight = 256.0f;
    };

    class BorderEffect : public FullscreenEffect 
    {
    public:

        NOZ_DECLARE_TYPEID(BorderEffect, FullscreenEffect);
        
        virtual ~BorderEffect() = default;
        
        void setBorderParams(const BorderParams& params);
        const BorderParams& getBorderParams() const { return _params; }
        
        virtual void render(CommandBuffer* commandBuffer) override;
        
    private:

        BorderEffect();

        void initialize(const std::shared_ptr<Texture>& texture);
        
        BorderParams _params;
        bool _needsUniformUpdate = true;
    };
}