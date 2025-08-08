/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "noz/Renderer/SamplerFactory.h"

struct SDL_GPUTexture;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;
struct SDL_GPUSampler;

namespace noz::renderer
{
    class Texture : public noz::Asset
    {
    public:
        
        Texture(const std::string& name);

        ~Texture();

        static Texture* createWhite(SDL_GPUDevice* device);

        // Create texture from existing SDL_GPUTexture (for GPU text rendering)
        static Texture* createFromGPUTexture(SDL_GPUDevice* device, SDL_GPUTexture* gpuTexture, const std::string& name);

        bool createFromSurface(SDL_Surface* surface);

        // Create texture from raw pixel data
        bool createFromMemory(SDL_GPUDevice* device, const void* data, int width, int height, int channels, bool generateMipmaps = false);

        // Create render target texture
        static Texture* createRenderTarget(SDL_GPUDevice* device, int width, int height, const std::string& name = "RenderTarget");
        
        // Create texture from Image
        static Texture* createFromImage(SDL_GPUDevice* device, const noz::Image& image, const std::string& name = "ImageTexture");
        
        // Load texture from file
        bool loadFromFile(SDL_GPUDevice* device, const std::string& filepath);
        
        SDL_GPUTexture* handle() const
        {
            return _texture;
        }

        // Get texture dimensions
        int width() const
        {
            return _width;
        }

        int height() const
        {
            return _height;
        }

        // Get sampler options for this texture
        const SamplerOptions& samplerOptions() const
        {
            return _samplerOptions;
        }

        // Set sampler options for this texture
        void setSamplerOptions(const SamplerOptions& options)
        {
            _samplerOptions = options;
        }

        // Clear the texture
        void clear();

    private:

        friend class AssetDatabase;

        static Texture* load(const std::string& name);

        SDL_GPUTexture* _texture;
        SDL_GPUDevice* _device;
        int _width;
        int _height;
        SamplerOptions _samplerOptions;
    };
}
