/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "noz/Renderer/SamplerFactory.h"
#include <iostream>

namespace noz::renderer
{
    SamplerFactory::SamplerFactory(SDL_GPUDevice* device)
        : m_device(device)
    {
    }

    SamplerFactory::~SamplerFactory()
    {
        cleanup();
    }

    SDL_GPUSampler* SamplerFactory::getSampler(const SamplerOptions& options)
    {
        auto it = m_samplers.find(options);
        if (it != m_samplers.end())
        {
            return it->second;
        }

        SDL_GPUSamplerCreateInfo samplerInfo = {};
        samplerInfo.min_filter = toSDLFilter(options.minFilter);
        samplerInfo.mag_filter = toSDLFilter(options.magFilter);
        samplerInfo.mipmap_mode = (options.minFilter == TextureFilter::Nearest) ? 
            SDL_GPU_SAMPLERMIPMAPMODE_NEAREST : SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        samplerInfo.address_mode_u = toSDLAddressMode(options.clampU);
        samplerInfo.address_mode_v = toSDLAddressMode(options.clampV);
        samplerInfo.address_mode_w = toSDLAddressMode(options.clampW);
        samplerInfo.enable_compare = options.enableCompare;
        samplerInfo.compare_op = options.compareOp;

        SDL_GPUSampler* sampler = SDL_CreateGPUSampler(m_device, &samplerInfo);
        if (!sampler)
        {
            std::cerr << "Failed to create sampler: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        m_samplers[options] = sampler;
        return sampler;
    }

    SDL_GPUFilter SamplerFactory::toSDLFilter(TextureFilter filter)
    {
        switch (filter)
        {
        case TextureFilter::Nearest:
            return SDL_GPU_FILTER_NEAREST;
        case TextureFilter::Linear:
            return SDL_GPU_FILTER_LINEAR;
        default:
            return SDL_GPU_FILTER_LINEAR;
        }
    }

    SDL_GPUSamplerAddressMode SamplerFactory::toSDLAddressMode(TextureClampMode mode)
    {
        switch (mode)
        {
        case TextureClampMode::Repeat:
            return SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        case TextureClampMode::ClampToEdge:
            return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        case TextureClampMode::MirroredRepeat:
            return SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT;
        default:
            return SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        }
    }

    void SamplerFactory::cleanup()
    {
        for (auto& pair : m_samplers)
        {
            if (pair.second)
            {
                SDL_ReleaseGPUSampler(m_device, pair.second);
            }
        }
        m_samplers.clear();
    }
}