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
        
        NOZ_DECLARE_TYPEID(Texture, Asset);

        ~Texture();

        static std::shared_ptr<Texture> createWhite();


        static std::shared_ptr<Texture> createFromMemory(const void* data, int width, int height, int channels, bool generateMipmaps = false, const std::string& name="Memory");
        static std::shared_ptr<Texture> createRenderTarget(int width, int height, const std::string& name = "RenderTarget");
        static std::shared_ptr<Texture> createRenderTarget(int width, int height, SDL_GPUTextureFormat format, const std::string& name = "RenderTarget");
        static std::shared_ptr<Texture> createFromImage(const noz::Image& image, const std::string& name = "Image");
                
        SDL_GPUTexture* handle() const;

        int width() const;

        int height() const;

        const SamplerOptions& samplerOptions() const;

        void setSamplerOptions(const SamplerOptions& options);

        void clear();

    private:

        friend class AssetDatabase;

        Texture();

        static std::shared_ptr<Texture> load(const std::string& name);

        void loadInternal();
        void createFromMemoryInternal(const void* data, int width, int height, int channels, bool generateMipmaps);

        SDL_GPUTexture* _texture;
        SamplerOptions _samplerOptions;
        int _width;
        int _height;
    };

    inline SDL_GPUTexture* Texture::handle() const
    {
        return _texture;
    }

    inline int Texture::width() const
    {
        return _width;
    }

    inline int Texture::height() const
    {
        return _height;
    }

    // Get sampler options for this texture
    inline const SamplerOptions& Texture::samplerOptions() const
    {
        return _samplerOptions;
    }
}
