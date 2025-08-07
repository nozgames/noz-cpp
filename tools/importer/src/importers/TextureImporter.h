#pragma once

#include "../ResourceImporter.h"

namespace noz
{
    namespace import
    {
        class TextureImporter : public ResourceImporter
        {
        public:
            TextureImporter(const ImportConfig::TextureConfig& config);
            
            bool canImport(const std::string& filePath) const override;
            bool import(const std::string& sourcePath, const std::string& outputDir) override;
            std::vector<std::string> getSupportedExtensions() const override;
            std::string getName() const override;
            
        private:
            ImportConfig::TextureConfig _config;
            
            // Load and process texture
            bool processTexture(const std::string& sourcePath, const std::string& outputPath);
            
            bool writeTexture(
				const std::string& outputPath,
				const void* data,
				size_t size, 
                int width,
				int height,
				int channels);
        };
    }
} 