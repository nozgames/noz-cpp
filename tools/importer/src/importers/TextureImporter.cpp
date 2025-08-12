/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "TextureImporter.h"
#include <SDL3_image/SDL_image.h>
#include "noz/StreamWriter.h"

namespace noz
{
    namespace import
    {
        // Convert sRGB color value to linear space
        float sRGBToLinear(float srgb)
        {
            if (srgb <= 0.04045f)
                return srgb / 12.92f;
            else
                return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
        }
        
        // Convert pixel data from sRGB to linear space
        void convertPixelsToLinear(uint8_t* pixels, int width, int height, int channels)
        {
            int totalPixels = width * height;
            for (int i = 0; i < totalPixels; ++i)
            {
                // Convert RGB channels (skip alpha)
                for (int c = 0; c < std::min(3, channels); ++c)
                {
                    uint8_t srgbValue = pixels[i * channels + c];
                    float srgbFloat = srgbValue / 255.0f;
                    float linearFloat = sRGBToLinear(srgbFloat);
                    pixels[i * channels + c] = static_cast<uint8_t>(std::round(linearFloat * 255.0f));
                }
                // Alpha channel (if present) stays unchanged
            }
        }
        TextureImporter::TextureImporter(const ImportConfig::TextureConfig& config)
            : _config(config)
        {
        }
        
        bool TextureImporter::canImport(const std::string& filePath) const
        {
            std::filesystem::path path(filePath);
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);            
            return extension == ".png";
        }
        
        void TextureImporter::import(const std::string& sourcePath, const std::string& outputDir)
        {
            std::filesystem::path source(sourcePath);
            std::filesystem::path output(outputDir);
                
            // Create output filename with .texture extension
            std::string outputName = source.stem().string() + ".texture";
            std::filesystem::path outputPath = output / outputName;
                
            // Process the texture
            processTexture(sourcePath, outputPath.string());
        }
        
        std::vector<std::string> TextureImporter::getSupportedExtensions() const
        {
            return {".png", ".jpg", ".jpeg", ".bmp", ".tga"};
        }
        
        std::string TextureImporter::getName() const
        {
            return "TextureImporter";
        }
        
        void TextureImporter::processTexture(const std::string& sourcePath, const std::string& outputPath)
        {
            // Load image using SDL_image
            SDL_Surface* surface = IMG_Load(sourcePath.c_str());
            if (!surface)
                throw std::runtime_error(SDL_GetError());

            // Always convert to RGBA32 for simplicity - we can optimize later
            SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            if (!convertedSurface)
            {
                SDL_DestroySurface(surface);
                throw std::runtime_error(SDL_GetError());
            }
            
            SDL_DestroySurface(surface);
            surface = convertedSurface;

            // Parse meta file for sampler options
            auto meta = noz::MetaFile::parse(sourcePath + ".meta");
            
            // Parse filter options
            auto minFilter = meta.getString("Texture", "min_filter", "linear");
            auto magFilter = meta.getString("Texture", "mag_filter", "linear");
            
            // Parse clamp options
            auto clampU = meta.getString("Texture", "clamp_u", "clamp_to_edge");
            auto clampV = meta.getString("Texture", "clamp_v", "clamp_to_edge");
            auto clampW = meta.getString("Texture", "clamp_w", "clamp_to_edge");
            
            // Parse mipmap option
            auto generateMipmaps = meta.getBool("Texture", "mipmaps", false);
            
            // Parse sRGB option
            auto convertFromSRGB = meta.getBool("Texture", "srgb", false);
            
            // Convert from sRGB to linear space if requested
            if (convertFromSRGB)
                convertPixelsToLinear(static_cast<uint8_t*>(surface->pixels), surface->w, surface->h, 4);

            // Generate mipmaps if requested
            std::vector<std::vector<uint8_t>> mipLevels;
            std::vector<std::pair<int, int>> mipDimensions;
            
            if (generateMipmaps)
            {
                // Add base level
                int channels = 4;
                std::vector<uint8_t> baseData;
                baseData.reserve(surface->w * surface->h * channels);
                
                // Copy base level pixel data row by row to ensure proper RGBA format
                uint8_t* srcPixels = (uint8_t*)surface->pixels;
                for (int y = 0; y < surface->h; ++y)
                {
                    uint8_t* rowStart = srcPixels + (y * surface->pitch);
                    for (int x = 0; x < surface->w; ++x)
                    {
                        uint8_t* pixel = rowStart + (x * 4); // 4 bytes per RGBA pixel
                        baseData.push_back(pixel[0]); // R
                        baseData.push_back(pixel[1]); // G
                        baseData.push_back(pixel[2]); // B
                        baseData.push_back(pixel[3]); // A
                    }
                }
                
                mipLevels.push_back(std::move(baseData));
                mipDimensions.push_back({surface->w, surface->h});
                
                // Generate additional mip levels
                SDL_Surface* currentSurface = surface;
                int currentWidth = surface->w;
                int currentHeight = surface->h;
                
                while (currentWidth > 1 || currentHeight > 1)
                {
                    int nextWidth = std::max(1, currentWidth / 2);
                    int nextHeight = std::max(1, currentHeight / 2);
                    
                    // Create scaled surface for this mip level
                    SDL_Surface* scaledSurface = SDL_CreateSurface(nextWidth, nextHeight, SDL_PIXELFORMAT_RGBA32);
                    if (!scaledSurface)
                        break;
                    
                    // Scale from the previous level (not always the original)
                    SDL_BlitSurfaceScaled(currentSurface, nullptr, scaledSurface, nullptr, SDL_SCALEMODE_LINEAR);
                    
                    // Store this mip level - use the surface's actual pitch
                    size_t mipSize = scaledSurface->pitch * nextHeight;
                    std::vector<uint8_t> mipData;
                    mipData.reserve(nextWidth * nextHeight * channels);
                    
                    // Copy pixel data row by row to ensure proper RGBA format
                    uint8_t* srcPixels = (uint8_t*)scaledSurface->pixels;
                    for (int y = 0; y < nextHeight; ++y)
                    {
                        uint8_t* rowStart = srcPixels + (y * scaledSurface->pitch);
                        for (int x = 0; x < nextWidth; ++x)
                        {
                            uint8_t* pixel = rowStart + (x * 4); // 4 bytes per RGBA pixel
                            mipData.push_back(pixel[0]); // R
                            mipData.push_back(pixel[1]); // G
                            mipData.push_back(pixel[2]); // B
                            mipData.push_back(pixel[3]); // A
                        }
                    }
                    
                    mipLevels.push_back(std::move(mipData));
                    mipDimensions.push_back({nextWidth, nextHeight});
                    
                    // Clean up the previous level if it's not the original
                    if (currentSurface != surface)
                    {
                        SDL_DestroySurface(currentSurface);
                    }
                    
                    currentSurface = scaledSurface;
                    currentWidth = nextWidth;
                    currentHeight = nextHeight;
                }
                
                // Clean up the last mip surface if it's not the original
                if (currentSurface != surface)
                {
                    SDL_DestroySurface(currentSurface);
                }
                
                // Write texture with all mip levels
                writeTextureWithMips(
                    outputPath,
                    mipLevels,
                    mipDimensions,
                    minFilter,
                    magFilter,
                    clampU,
                    clampV,
                    clampW);
                SDL_DestroySurface(surface);
            }
            else
            {
                // Write texture data without mipmaps - always use 4 channels (RGBA)
                int channels = 4;
                size_t dataSize = surface->w * surface->h * channels;
                writeTexture(
                    outputPath,
                    surface->pixels,
                    dataSize, 
                    surface->w,
                    surface->h,
                    channels,
                    minFilter,
                    magFilter,
                    clampU,
                    clampV,
                    clampW,
                    false);
                SDL_DestroySurface(surface);
            }
        }
        
        void TextureImporter::writeTexture(
			const std::string& outputPath,
			const void* data,
			size_t size,
            int width,
			int height,
			int channels,
            const std::string& minFilter,
            const std::string& magFilter,
            const std::string& clampU,
            const std::string& clampV,
            const std::string& clampW,
            bool generateMipmaps)
        {
            noz::StreamWriter writer;

            // Write file signature
            writer.writeFileSignature("NZXT");
            
            // Write version
            writer.writeUInt32(1); // Version 1
            
            // Convert string options to enum values
            uint8_t minFilterValue = (minFilter == "nearest" || minFilter == "point") ? 0 : 1; // 0=Nearest, 1=Linear
            uint8_t magFilterValue = (magFilter == "nearest" || magFilter == "point") ? 0 : 1; // 0=Nearest, 1=Linear
            
            uint8_t clampUValue = 1; // Default to ClampToEdge
            if (clampU == "repeat") clampUValue = 0;
            else if (clampU == "clamp_to_edge") clampUValue = 1;
            else if (clampU == "mirrored_repeat") clampUValue = 2;
            else if (clampU == "clamp_to_border") clampUValue = 3;
            
            uint8_t clampVValue = 1; // Default to ClampToEdge
            if (clampV == "repeat") clampVValue = 0;
            else if (clampV == "clamp_to_edge") clampVValue = 1;
            else if (clampV == "mirrored_repeat") clampVValue = 2;
            else if (clampV == "clamp_to_border") clampVValue = 3;
            
            uint8_t clampWValue = 1; // Default to ClampToEdge
            if (clampW == "repeat") clampWValue = 0;
            else if (clampW == "clamp_to_edge") clampWValue = 1;
            else if (clampW == "mirrored_repeat") clampWValue = 2;
            else if (clampW == "clamp_to_border") clampWValue = 3;

            // Write texture data
            uint32_t format = (channels == 4) ? 1 : 0; // 0=RGB, 1=RGBA
            writer.writeUInt32(format);
            writer.writeUInt32(width);
            writer.writeUInt32(height);
            
            // Write sampler options
            writer.writeUInt8(minFilterValue);
            writer.writeUInt8(magFilterValue);
            writer.writeUInt8(clampUValue);
            writer.writeUInt8(clampVValue);
            writer.writeUInt8(clampWValue);
            writer.writeBool(generateMipmaps);
            
            // Write pixel data
            writer.write(data, size);
            
            // Write to file
            if (!writer.writeToFile(outputPath))
				throw std::runtime_error("failed to write");
        }
        
        void TextureImporter::writeTextureWithMips(
            const std::string& outputPath,
            const std::vector<std::vector<uint8_t>>& mipLevels,
            const std::vector<std::pair<int, int>>& mipDimensions,
            const std::string& minFilter,
            const std::string& magFilter,
            const std::string& clampU,
            const std::string& clampV,
            const std::string& clampW)
        {
            if (mipLevels.empty() || mipDimensions.empty())
                throw std::runtime_error("no mip levels to write");
            
            noz::StreamWriter writer;
            
            // Write file signature
            writer.writeFileSignature("NZXT");
            
            // Write version
            writer.writeUInt32(1); // Version 1

            // Convert string options to enum values
            uint8_t minFilterValue = (minFilter == "nearest" || minFilter == "point") ? 0 : 1;
            uint8_t magFilterValue = (magFilter == "nearest" || magFilter == "point") ? 0 : 1;
            
            uint8_t clampUValue = 1;
            if (clampU == "repeat") clampUValue = 0;
            else if (clampU == "clamp_to_edge") clampUValue = 1;
            else if (clampU == "mirrored_repeat") clampUValue = 2;
            else if (clampU == "clamp_to_border") clampUValue = 3;
            
            uint8_t clampVValue = 1;
            if (clampV == "repeat") clampVValue = 0;
            else if (clampV == "clamp_to_edge") clampVValue = 1;
            else if (clampV == "mirrored_repeat") clampVValue = 2;
            else if (clampV == "clamp_to_border") clampVValue = 3;
            
            uint8_t clampWValue = 1;
            if (clampW == "repeat") clampWValue = 0;
            else if (clampW == "clamp_to_edge") clampWValue = 1;
            else if (clampW == "mirrored_repeat") clampWValue = 2;
            else if (clampW == "clamp_to_border") clampWValue = 3;

            // Write texture data
            uint32_t format = 1; // RGBA
            uint32_t width = mipDimensions[0].first;
            uint32_t height = mipDimensions[0].second;
            uint32_t numMipLevels = static_cast<uint32_t>(mipLevels.size());
            
            writer.writeUInt32(format);
            writer.writeUInt32(width);
            writer.writeUInt32(height);
            
            // Write sampler options
            writer.writeUInt8(minFilterValue);
            writer.writeUInt8(magFilterValue);
            writer.writeUInt8(clampUValue);
            writer.writeUInt8(clampVValue);
            writer.writeUInt8(clampWValue);
            writer.writeBool(true); // Has mipmaps
            
            // Write number of mip levels
            writer.writeUInt32(numMipLevels);
            
            // Write each mip level
            for (size_t i = 0; i < mipLevels.size(); ++i)
            {
                // Write mip level dimensions
                writer.writeUInt32(mipDimensions[i].first);
                writer.writeUInt32(mipDimensions[i].second);
                
                // Write mip level data
                writer.writeUInt32(static_cast<uint32_t>(mipLevels[i].size()));
                writer.write(mipLevels[i].data(), mipLevels[i].size());
            }
            
            // Write to file
            if (!writer.writeToFile(outputPath))
                throw std::runtime_error("failed to write");;            
        }
    }
}
