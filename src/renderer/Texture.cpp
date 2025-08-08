/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <filesystem>
#include <algorithm>
#include <cmath>
#include "noz/StreamReader.h"

namespace noz::renderer
{
    Texture::Texture(const std::string& path)
        : noz::Asset(path)
        , _texture(nullptr)
        , _device(nullptr)
        , _width(0)
        , _height(0)
    {
    }

    Texture::~Texture()
    {
		if (_texture && _device)
		{
			SDL_ReleaseGPUTexture(_device, _texture);
		}
		_texture = nullptr;
		_device = nullptr;
		_width = 0;
		_height = 0;
    }

    Texture* Texture::load(const std::string& name)
    {
        auto* renderer = Renderer::instance();
        auto* gpu = renderer->GetGPUDevice();
        
        // Check if this should be a white texture
        if (name == "white")
            return createWhite(gpu);
        
        auto* texture = new Texture(name);
		auto texturePath = noz::AssetDatabase::getFullPath(name, "texture");
		if (!texture->loadFromFile(gpu, texturePath))
        {
            std::cerr << "Failed to load texture: " << texturePath << std::endl;
            delete texture;
            return nullptr;
        }
        
        return texture;
    }

    Texture* Texture::createFromGPUTexture(SDL_GPUDevice* device, SDL_GPUTexture* gpuTexture, const std::string& name)
    {
        if (!device || !gpuTexture)
        {
            std::cerr << "Invalid GPU device or texture for creating texture from GPU texture" << std::endl;
            return nullptr;
        }

        // Create a new texture instance
        auto* texture = new Texture(name);
        texture->_device = device;
        texture->_texture = gpuTexture;

        // Note: We don't own the SDL_GPUTexture, so we don't destroy it
        // The texture dimensions would need to be retrieved from the GPU texture
        // For now, we'll use placeholder dimensions
        texture->_width = 256;  // Placeholder - would need to get from GPU texture
        texture->_height = 256; // Placeholder - would need to get from GPU texture

        return texture;
    }

    bool Texture::createFromSurface(SDL_Surface* surface)
    {
        if (!surface)
        {
            std::cerr << "Invalid surface for texture creation" << std::endl;
            return false;
        }

        // Get GPU device from renderer
        auto* renderer = Renderer::instance();
        if (!renderer)
        {
            std::cerr << "Renderer not available for texture creation" << std::endl;
            return false;
        }

        _device = renderer->GetGPUDevice();
        if (!_device)
        {
            std::cerr << "GPU device not available for texture creation" << std::endl;
            return false;
        }

        // Convert surface to RGBA32 if needed
        SDL_Surface* convertedSurface = nullptr;
        if (SDL_BYTESPERPIXEL(surface->format) != 4)
        {
            convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            if (!convertedSurface)
            {
                std::cerr << "Failed to convert surface to RGBA32: " << SDL_GetError() << std::endl;
                return false;
            }
            surface = convertedSurface;
        }

        // Create transfer buffer for pixel data
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = surface->pitch * surface->h;
        transferInfo.props = 0;
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(_device, &transferInfo);
        if (!transferBuffer)
        {
            std::cerr << "Failed to create transfer buffer: " << SDL_GetError() << std::endl;
            if (convertedSurface) SDL_DestroySurface(convertedSurface);
            return false;
        }

        // Map transfer buffer and copy pixel data
        void* mapped = SDL_MapGPUTransferBuffer(_device, transferBuffer, false);
        if (!mapped)
        {
            std::cerr << "Failed to map transfer buffer: " << SDL_GetError() << std::endl;
            if (convertedSurface) SDL_DestroySurface(convertedSurface);
            SDL_ReleaseGPUTransferBuffer(_device, transferBuffer);
            return false;
        }
        SDL_memcpy(mapped, surface->pixels, transferInfo.size);
        SDL_UnmapGPUTransferBuffer(_device, transferBuffer);

        // Create GPU texture
        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureInfo.width = surface->w;
        textureInfo.height = surface->h;
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = 1;
        textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        _texture = SDL_CreateGPUTexture(_device, &textureInfo);
        if (!_texture)
        {
            std::cerr << "Failed to create GPU texture: " << SDL_GetError() << std::endl;
            if (convertedSurface) SDL_DestroySurface(convertedSurface);
            SDL_ReleaseGPUTransferBuffer(_device, transferBuffer);
            return false;
        }

        // Acquire a command buffer for texture loading
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(_device);
        if (!commandBuffer)
        {
            if (convertedSurface) SDL_DestroySurface(convertedSurface);
            SDL_ReleaseGPUTransferBuffer(_device, transferBuffer);
            return false;
        }

        // Upload pixel data to GPU texture
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        SDL_GPUTextureTransferInfo source = { transferBuffer, 0, static_cast<Uint32>(surface->w), static_cast<Uint32>(surface->h) };
        SDL_GPUTextureRegion destination = {};
        destination.texture = _texture;
        destination.w = static_cast<Uint32>(surface->w);
        destination.h = static_cast<Uint32>(surface->h);
        destination.d = 1;

        SDL_UploadToGPUTexture(copyPass, &source, &destination, false);
        SDL_EndGPUCopyPass(copyPass);

        // Submit and wait for command buffer
        if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
        {
            std::cerr << "Failed to submit command buffer for texture upload: " << SDL_GetError() << std::endl;
        }
        else
        {
            SDL_WaitForGPUIdle(_device);
        }

        // Store dimensions
        _width = surface->w;
        _height = surface->h;

        // Clean up
        SDL_ReleaseGPUTransferBuffer(_device, transferBuffer);
        if (convertedSurface) SDL_DestroySurface(convertedSurface);

        return true;
    }

    Texture* Texture::createWhite(SDL_GPUDevice* device)
    {
        if (!device)
        {
            std::cerr << "Invalid GPU device for creating white texture" << std::endl;
            return nullptr;
        }

        // Create a new texture instance
        auto* texture = new Texture("white");
        texture->_device = device;

        // Create a 16x16 white texture
        const int width = 16;
        const int height = 16;
        const int pitch = width * 4; // 4 bytes per pixel (RGBA)
        const int size = pitch * height;

        // Create pixel data: all white pixels
        std::vector<Uint8> pixels(size);
        memset(&pixels[0], 255, size);

        // Create transfer buffer
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = size;
        transferInfo.props = 0;
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
        if (!transferBuffer)
        {
            std::cerr << "Failed to create transfer buffer for test texture: " << SDL_GetError() << std::endl;
            return false;
        }

        // Copy pixel data to transfer buffer
        void* mapped = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        if (!mapped)
        {
            std::cerr << "Failed to map transfer buffer for test texture: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            return false;
        }
        SDL_memcpy(mapped, pixels.data(), size);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer);

        // Create GPU texture
        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureInfo.width = width;
        textureInfo.height = height;
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = 1;
        textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;

        texture->_texture = SDL_CreateGPUTexture(device, &textureInfo);
        if (!texture->_texture)
        {
            std::cerr << "Failed to create white GPU texture: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            delete texture;
            return nullptr;
        }

        // Upload pixel data
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (!commandBuffer)
        {
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            return false;
        }

        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        SDL_GPUTextureTransferInfo source = { transferBuffer, 0, static_cast<Uint32>(width), static_cast<Uint32>(height) };
        SDL_GPUTextureRegion destination = {};
        destination.texture = texture->_texture;
        destination.w = static_cast<Uint32>(width);
        destination.h = static_cast<Uint32>(height);
        destination.d = 1;

        SDL_UploadToGPUTexture(copyPass, &source, &destination, false);
        SDL_EndGPUCopyPass(copyPass);

        // Submit and wait
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        SDL_WaitForGPUIdle(device);

        // Store dimensions
        texture->_width = width;
        texture->_height = height;

        // Clean up
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

        return texture;
    }

    bool Texture::createFromMemory(SDL_GPUDevice* device, const void* data, int width, int height, int channels, bool generateMipmaps)
    {
        if (!device || !data || width <= 0 || height <= 0 || channels <= 0)
        {
            std::cerr << "Invalid parameters for createFromMemory" << std::endl;
            return false;
        }

        _device = device;

        // Handle different channel formats
        std::vector<uint8_t> rgbaData;
        if (channels == 1)
        {
            // Single channel - use as-is for R8_UNORM
            // No conversion needed
        }
        else if (channels == 3)
        {
            // Convert RGB to RGBA
            const uint8_t* srcData = static_cast<const uint8_t*>(data);
            rgbaData.resize(width * height * 4);
            
            for (int i = 0; i < width * height; ++i)
            {
                rgbaData[i * 4 + 0] = srcData[i * 3 + 0]; // R
                rgbaData[i * 4 + 1] = srcData[i * 3 + 1]; // G
                rgbaData[i * 4 + 2] = srcData[i * 3 + 2]; // B
                rgbaData[i * 4 + 3] = 255;                 // A
            }
            data = rgbaData.data();
            channels = 4;
        }
        else if (channels != 4)
        {
            std::cerr << "Unsupported channel count: " << channels << std::endl;
            return false;
        }

        // Create transfer buffer for pixel data
        const int pitch = width * channels;
        const int size = pitch * height;
        
        // Ensure proper alignment for texture upload
        const int alignedPitch = (pitch + 3) & ~3; // Align to 4-byte boundary
        
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = size;
        transferInfo.props = 0;
        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
        if (!transferBuffer)
        {
            std::cerr << "Failed to create transfer buffer: " << SDL_GetError() << std::endl;
            return false;
        }

        // Map transfer buffer and copy pixel data
        void* mapped = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        if (!mapped)
        {
            std::cerr << "Failed to map transfer buffer: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            return false;
        }
        SDL_memcpy(mapped, data, size);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer);

        // Calculate number of mipmap levels if requested
        uint32_t numLevels = 1;
        if (generateMipmaps)
        {
            // Calculate maximum number of mipmap levels
            int maxDim = std::max(width, height);
            numLevels = 1 + static_cast<uint32_t>(std::floor(std::log2(maxDim)));
        }

        // Create GPU texture with appropriate format based on channel count
        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = (channels == 1) ? SDL_GPU_TEXTUREFORMAT_R8_UNORM : SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureInfo.width = width;
        textureInfo.height = height;
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = numLevels;
        textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        textureInfo.props = SDL_CreateProperties();
        SDL_SetStringProperty(textureInfo.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, "FontAtlas");

        _texture = SDL_CreateGPUTexture(device, &textureInfo);
        SDL_DestroyProperties(textureInfo.props);
        if (!_texture)
        {
            std::cerr << "Failed to create GPU texture: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            return false;
        }

        // Acquire a command buffer for texture loading
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (!commandBuffer)
        {
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            return false;
        }

        // Upload pixel data to GPU texture
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        SDL_GPUTextureTransferInfo source = { transferBuffer, 0, static_cast<Uint32>(width), static_cast<Uint32>(height) };
        SDL_GPUTextureRegion destination = {};
        destination.texture = _texture;
        destination.w = static_cast<Uint32>(width);
        destination.h = static_cast<Uint32>(height);
        destination.d = 1;

        SDL_UploadToGPUTexture(copyPass, &source, &destination, false);
        SDL_EndGPUCopyPass(copyPass);

        // Submit and wait for command buffer
        if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
        {
            std::cerr << "Failed to submit command buffer for texture upload: " << SDL_GetError() << std::endl;
        }
        else
        {
            SDL_WaitForGPUIdle(device);
        }

        // Store dimensions
        _width = width;
        _height = height;

        // Clean up
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

        return true;
    }

    Texture* Texture::createRenderTarget(SDL_GPUDevice* device, int width, int height, const std::string& name)
    {
        if (!device || width <= 0 || height <= 0)
        {
            std::cerr << "Invalid parameters for createRenderTarget" << std::endl;
            return nullptr;
        }

        auto* texture = new Texture(name);
        texture->_device = device;
        texture->_width = width;
        texture->_height = height;

        // Create render target texture
        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureInfo.width = width;
        textureInfo.height = height;
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = 1;
        textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        textureInfo.props = SDL_CreateProperties();
        SDL_SetStringProperty(textureInfo.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, name.c_str());

        texture->_texture = SDL_CreateGPUTexture(device, &textureInfo);
        SDL_DestroyProperties(textureInfo.props);
        
        if (!texture->_texture)
        {
            std::cerr << "Failed to create render target texture: " << SDL_GetError() << std::endl;
            delete texture;
            return nullptr;
        }

        return texture;
    }

    Texture* Texture::createRenderTarget(SDL_GPUDevice* device, int width, int height, SDL_GPUTextureFormat format, const std::string& name)
    {
        if (!device || width <= 0 || height <= 0)
        {
            std::cerr << "Invalid parameters for createRenderTarget" << std::endl;
            return nullptr;
        }

        auto* texture = new Texture(name);
        texture->_device = device;
        texture->_width = width;
        texture->_height = height;

        // Create render target texture with specified format
        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = format;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureInfo.width = width;
        textureInfo.height = height;
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = 1;
        textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        textureInfo.props = SDL_CreateProperties();
        SDL_SetStringProperty(textureInfo.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, name.c_str());

        texture->_texture = SDL_CreateGPUTexture(device, &textureInfo);
        if (!texture->_texture)
        {
            std::cerr << "Failed to create render target texture: " << SDL_GetError() << std::endl;
            SDL_DestroyProperties(textureInfo.props);
            delete texture;
            return nullptr;
        }

        SDL_DestroyProperties(textureInfo.props);
        return texture;
    }

    Texture* Texture::createFromImage(SDL_GPUDevice* device, const noz::Image& image, const std::string& name)
    {
        if (!device || image.empty())
        {
            std::cerr << "Invalid parameters for createFromImage" << std::endl;
            return nullptr;
        }

        auto* texture = new Texture(name);
        texture->_device = device;
        texture->_width = image.width();
        texture->_height = image.height();

        // Convert Image format to appropriate GPU format
        SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM; // Default to RGBA
        const void* pixelData = image.data();
        size_t dataSize = image.dataSize();

        if (image.format() == noz::Image::Format::RGB)
        {
            // Need to convert RGB to RGBA for GPU texture
            std::vector<uint8_t> rgbaData;
            rgbaData.reserve(image.pixelCount() * 4);
            
            for (size_t i = 0; i < image.pixelCount(); ++i)
            {
                size_t rgbIndex = i * 3;
                rgbaData.push_back(image.data()[rgbIndex]);     // R
                rgbaData.push_back(image.data()[rgbIndex + 1]); // G
                rgbaData.push_back(image.data()[rgbIndex + 2]); // B
                rgbaData.push_back(255);                        // A (opaque)
            }
            
            pixelData = rgbaData.data();
            dataSize = rgbaData.size();
        }

        // Create transfer buffer for uploading pixel data
        SDL_GPUTransferBufferCreateInfo transferInfo = {};
        transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        transferInfo.size = (uint32_t)dataSize;
        transferInfo.props = 0;

        SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
        if (!transferBuffer)
        {
            std::cerr << "Failed to create transfer buffer for image texture: " << SDL_GetError() << std::endl;
            delete texture;
            return nullptr;
        }

        // Copy pixel data to transfer buffer
        void* mappedData = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
        if (!mappedData)
        {
            std::cerr << "Failed to map transfer buffer for image texture: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            delete texture;
            return nullptr;
        }

        std::memcpy(mappedData, pixelData, dataSize);
        SDL_UnmapGPUTransferBuffer(device, transferBuffer);

        // Create GPU texture
        SDL_GPUTextureCreateInfo textureInfo = {};
        textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
        textureInfo.format = format;
        textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        textureInfo.width = texture->_width;
        textureInfo.height = texture->_height;
        textureInfo.layer_count_or_depth = 1;
        textureInfo.num_levels = 1;
        textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
        textureInfo.props = SDL_CreateProperties();
        SDL_SetStringProperty(textureInfo.props, SDL_PROP_GPU_TEXTURE_CREATE_NAME_STRING, name.c_str());

        texture->_texture = SDL_CreateGPUTexture(device, &textureInfo);
        SDL_DestroyProperties(textureInfo.props);
        
        if (!texture->_texture)
        {
            std::cerr << "Failed to create GPU texture from image: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            delete texture;
            return nullptr;
        }

        // Upload pixel data to texture
        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
        if (!cmd)
        {
            std::cerr << "Failed to acquire command buffer for image texture upload" << std::endl;
            SDL_ReleaseGPUTransferBuffer(device, transferBuffer);
            delete texture;
            return nullptr;
        }

        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTextureTransferInfo source = {transferBuffer, 0, static_cast<Uint32>(texture->_width), static_cast<Uint32>(texture->_height)};
        SDL_GPUTextureRegion destination = {};
        destination.texture = texture->_texture;
        destination.w = static_cast<Uint32>(texture->_width);
        destination.h = static_cast<Uint32>(texture->_height);
        destination.d = 1;

        SDL_UploadToGPUTexture(copyPass, &source, &destination, false);
        SDL_EndGPUCopyPass(copyPass);

        // Submit and wait for completion
        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_WaitForGPUIdle(device);

        // Clean up transfer buffer
        SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

        return texture;
    }
    
    bool Texture::loadFromFile(SDL_GPUDevice* device, const std::string& filepath)
    {
        _device = device;
        
        noz::StreamReader reader;
        if (!reader.loadFromFile(filepath))
        {
            std::cerr << "Failed to open texture file: " << filepath << std::endl;
            return false;
        }

        // Validate file signature
        if (!reader.readFileSignature("NZXT"))
        {
            std::cerr << "Invalid texture file signature in: " << filepath << std::endl;
            return false;
        }

        // Read version
        uint32_t version = reader.readUInt32();
        if (version != 1)
        {
            std::cerr << "Unsupported texture version " << version << " in: " << filepath << std::endl;
            return false;
        }

        // Read texture data
        uint32_t format = reader.readUInt32();
        uint32_t width = reader.readUInt32();
        uint32_t height = reader.readUInt32();
        
        // Validate format
        if (format > 1)
        {
            std::cerr << "Unsupported texture format " << format << " in: " << filepath << std::endl;
            return false;
        }

        // Read sampler options
        uint8_t minFilterValue = reader.readUInt8();
        uint8_t magFilterValue = reader.readUInt8();
        uint8_t clampUValue = reader.readUInt8();
        uint8_t clampVValue = reader.readUInt8();
        uint8_t clampWValue = reader.readUInt8();
        bool hasMipmaps = reader.readBool();
        
        // Convert values to enums
        _samplerOptions.minFilter = (minFilterValue == 0) ? TextureFilter::Nearest : TextureFilter::Linear;
        _samplerOptions.magFilter = (magFilterValue == 0) ? TextureFilter::Nearest : TextureFilter::Linear;
        
        switch(clampUValue)
        {
            case 0: _samplerOptions.clampU = TextureClampMode::Repeat; break;
            case 1: _samplerOptions.clampU = TextureClampMode::ClampToEdge; break;
            case 2: _samplerOptions.clampU = TextureClampMode::MirroredRepeat; break;
            default: _samplerOptions.clampU = TextureClampMode::ClampToEdge; break;
        }
        
        switch(clampVValue)
        {
            case 0: _samplerOptions.clampV = TextureClampMode::Repeat; break;
            case 1: _samplerOptions.clampV = TextureClampMode::ClampToEdge; break;
            case 2: _samplerOptions.clampV = TextureClampMode::MirroredRepeat; break;
            default: _samplerOptions.clampV = TextureClampMode::ClampToEdge; break;
        }
        
        switch(clampWValue)
        {
            case 0: _samplerOptions.clampW = TextureClampMode::Repeat; break;
            case 1: _samplerOptions.clampW = TextureClampMode::ClampToEdge; break;
            case 2: _samplerOptions.clampW = TextureClampMode::MirroredRepeat; break;
            default: _samplerOptions.clampW = TextureClampMode::ClampToEdge; break;
        }

        // Handle texture with multiple mip levels
        if (hasMipmaps)
        {
            // Read number of mip levels
            uint32_t numMipLevels = reader.readUInt32();
            
            // For now, just read the base level and let GPU handle mipmaps
            // TODO: Upload all mip levels to GPU
            for (uint32_t level = 0; level < numMipLevels; ++level)
            {
                uint32_t mipWidth = reader.readUInt32();
                uint32_t mipHeight = reader.readUInt32();
                uint32_t mipDataSize = reader.readUInt32();
                
                if (level == 0)
                {
                    // Read base level
                    std::vector<uint8_t> pixelData = reader.readBytes(mipDataSize);
                    
                    // Create texture with mipmaps
                    int channels = (format == 1) ? 4 : 3;
                    return createFromMemory(device, pixelData.data(), width, height, channels, true);
                }
                else
                {
                    // Skip other mip levels for now
                    reader.readBytes(mipDataSize);
                }
            }
        }
        else
        {
            // Calculate data size
            int channels = (format == 1) ? 4 : 3; // RGBA or RGB
            size_t dataSize = width * height * channels;

            // Read pixel data
            std::vector<uint8_t> pixelData = reader.readBytes(dataSize);
            return createFromMemory(device, pixelData.data(), width, height, channels, false);
        }
        
        return false; // Should not reach here
    }
}
