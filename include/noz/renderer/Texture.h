/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

struct SDL_GPUTexture;
struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;
struct SDL_GPUSampler;

namespace noz::renderer
{

    class Texture : public noz::IResource
    {
    public:
        
        Texture(const std::string& name);

        ~Texture();

        static Texture* load(const std::string& name);

        static Texture* createWhite(SDL_GPUDevice* device);

        // Create texture from existing SDL_GPUTexture (for GPU text rendering)
        static Texture* createFromGPUTexture(SDL_GPUDevice* device, SDL_GPUTexture* gpuTexture, const std::string& name);

        bool createFromSurface(SDL_Surface* surface);

        // Create texture from raw pixel data
        bool createFromMemory(SDL_GPUDevice* device, const void* data, int width, int height, int channels);

        // Create render target texture
        static Texture* createRenderTarget(SDL_GPUDevice* device, int width, int height, const std::string& name = "RenderTarget");
        
        // Create texture from Image
        static Texture* createFromImage(SDL_GPUDevice* device, const noz::Image& image, const std::string& name = "ImageTexture");
        
        // Load texture from file
        bool loadFromFile(SDL_GPUDevice* device, const std::string& filepath);
        
        SDL_GPUTexture* handle() const
        {
            return m_texture;
        }

        // Get texture dimensions
        int width() const
        {
            return m_width;
        }

        int height() const
        {
            return m_height;
        }

        // Clear the texture
        void clear();

    private:

        SDL_GPUTexture* m_texture;
        SDL_GPUDevice* m_device;
        int m_width;
        int m_height;
    };
}
