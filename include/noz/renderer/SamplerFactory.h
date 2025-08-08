/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <SDL3/SDL_gpu.h>
#include <unordered_map>
#include <string>

namespace noz::renderer
{
    enum class TextureFilter
    {
        Nearest,
        Linear
    };

    enum class TextureClampMode
    {
        Repeat,
        ClampToEdge,
        MirroredRepeat
    };

    struct SamplerOptions
    {
        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;
        TextureClampMode clampU = TextureClampMode::ClampToEdge;
        TextureClampMode clampV = TextureClampMode::ClampToEdge;
        TextureClampMode clampW = TextureClampMode::ClampToEdge;
        bool enableCompare = false;
        SDL_GPUCompareOp compareOp = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

        bool operator==(const SamplerOptions& other) const
        {
            return minFilter == other.minFilter &&
                   magFilter == other.magFilter &&
                   clampU == other.clampU &&
                   clampV == other.clampV &&
                   clampW == other.clampW &&
                   enableCompare == other.enableCompare &&
                   compareOp == other.compareOp;
        }
    };

    struct SamplerOptionsHash
    {
        std::size_t operator()(const SamplerOptions& options) const
        {
            std::size_t hash = 0;
            hash ^= std::hash<int>{}(static_cast<int>(options.minFilter)) << 0;
            hash ^= std::hash<int>{}(static_cast<int>(options.magFilter)) << 4;
            hash ^= std::hash<int>{}(static_cast<int>(options.clampU)) << 8;
            hash ^= std::hash<int>{}(static_cast<int>(options.clampV)) << 12;
            hash ^= std::hash<int>{}(static_cast<int>(options.clampW)) << 16;
            hash ^= std::hash<bool>{}(options.enableCompare) << 20;
            hash ^= std::hash<int>{}(static_cast<int>(options.compareOp)) << 21;
            return hash;
        }
    };

    class SamplerFactory
    {
    public:
        SamplerFactory(SDL_GPUDevice* device);
        ~SamplerFactory();

        SDL_GPUSampler* getSampler(const SamplerOptions& options);
        
        static SDL_GPUFilter toSDLFilter(TextureFilter filter);
        static SDL_GPUSamplerAddressMode toSDLAddressMode(TextureClampMode mode);
        
        void cleanup();

    private:
        SDL_GPUDevice* m_device;
        std::unordered_map<SamplerOptions, SDL_GPUSampler*, SamplerOptionsHash> m_samplers;
    };
}