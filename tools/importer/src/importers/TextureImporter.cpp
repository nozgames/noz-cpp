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

            // Write texture data - always use 4 channels (RGBA)
            int channels = 4;
            size_t dataSize = surface->w * surface->h * channels;
            bool success = writeTexture(outputPath, surface->pixels, dataSize, 
                                       surface->w, surface->h, channels);

            SDL_DestroySurface(surface);
            return success;
        }
        
        bool TextureImporter::writeTexture(
			const std::string& outputPath,
			const void* data,
			size_t size,
            int width,
			int height,
			int channels)
        {
            std::ofstream file(outputPath, std::ios::binary);
            if (!file.is_open())
            {
                std::cerr << "Failed to create output file: " << outputPath << std::endl;
                return false;
            }

            // Write header
            // Format: [magic:4][version:4][format:4][width:4][height:4][data...]
            const uint32_t magic = 0x5A4F4E54; // "NZXT" in hex (NoZ TeXture)
            const uint32_t version = 1;
            const uint32_t format = (channels == 4) ? 1 : 0; // 0=RGB, 1=RGBA
            
            file.write(reinterpret_cast<const char*>(&magic), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&version), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&format), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&width), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&height), sizeof(uint32_t));
            
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
                      << (format == 1 ? "RGBA" : "RGB") << ")" << std::endl;
            return true;
        }
    }
}
