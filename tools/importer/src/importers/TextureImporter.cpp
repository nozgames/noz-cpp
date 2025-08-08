/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "TextureImporter.h"
#include <SDL3_image/SDL_image.h>
#include <fstream>

namespace noz
{
    namespace import
    {
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
        
        bool TextureImporter::import(const std::string& sourcePath, const std::string& outputDir)
        {
            try
            {
                std::filesystem::path source(sourcePath);
                std::filesystem::path output(outputDir);
                
                // Create output filename with .texture extension
                std::string outputName = source.stem().string() + ".texture";
                std::filesystem::path outputPath = output / outputName;
                
                // Process the texture
                return processTexture(sourcePath, outputPath.string());
            }
            catch (const std::exception& e)
            {
                std::cerr << "Texture import error: " << e.what() << std::endl;
                return false;
            }
        }
        
        std::vector<std::string> TextureImporter::getSupportedExtensions() const
        {
            return {".png", ".jpg", ".jpeg", ".bmp", ".tga"};
        }
        
        std::string TextureImporter::getName() const
        {
            return "TextureImporter";
        }
        
        bool TextureImporter::processTexture(const std::string& sourcePath, const std::string& outputPath)
        {
            // Load image using SDL_image
            SDL_Surface* surface = IMG_Load(sourcePath.c_str());
            if (!surface)
            {
                std::cerr << "Failed to load image: " << sourcePath << " - " << SDL_GetError() << std::endl;
                return false;
            }

            // Always convert to RGBA32 for simplicity - we can optimize later
            SDL_Surface* convertedSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            if (!convertedSurface)
            {
                std::cerr << "Failed to convert surface to RGBA32: " << SDL_GetError() << std::endl;
                SDL_DestroySurface(surface);
                return false;
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

            // Write texture data - always use 4 channels (RGBA)
            int channels = 4;
            size_t dataSize = surface->w * surface->h * channels;
            bool success = writeTexture(outputPath, surface->pixels, dataSize, 
                                       surface->w, surface->h, channels,
                                       minFilter, magFilter, clampU, clampV, clampW);

            SDL_DestroySurface(surface);
            return success;
        }
        
        bool TextureImporter::writeTexture(
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
            const std::string& clampW)
        {
            std::ofstream file(outputPath, std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "Failed to create output file: " << outputPath << std::endl;
                return false;
            }

            // Convert string options to enum values
            uint8_t minFilterValue = (minFilter == "nearest") ? 0 : 1; // 0=Nearest, 1=Linear
            uint8_t magFilterValue = (magFilter == "nearest") ? 0 : 1; // 0=Nearest, 1=Linear
            
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

            // Write header
            // Format: [magic:4][version:4][format:4][width:4][height:4][samplerOptions:5][data...]
            const uint32_t magic = 0x5A4F4E54; // "NZXT" in hex (NoZ TeXture)
            const uint32_t version = 2; // Version 2 includes sampler options
            const uint32_t format = (channels == 4) ? 1 : 0; // 0=RGB, 1=RGBA
            
            file.write(reinterpret_cast<const char*>(&magic), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&version), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&format), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&width), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&height), sizeof(uint32_t));
            
            // Write sampler options (5 bytes)
            file.write(reinterpret_cast<const char*>(&minFilterValue), sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&magFilterValue), sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&clampUValue), sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&clampVValue), sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&clampWValue), sizeof(uint8_t));
            
            // Write pixel data
            file.write(reinterpret_cast<const char*>(data), size);
            
            if (!file.good())
            {
                std::cerr << "Failed to write texture data to: " << outputPath << std::endl;
                return false;
            }
            
            file.close();
            std::cout << "Wrote texture: " << outputPath 
                      << " (" << width << "x" << height << ", " 
                      << (format == 1 ? "RGBA" : "RGB") << ")" 
                      << " [Filter: " << minFilter << "/" << magFilter 
                      << ", Clamp: " << clampU << "/" << clampV << "/" << clampW << "]" << std::endl;
            return true;
        }
    }
}
