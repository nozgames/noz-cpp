#pragma once

#include "../GLTFLoader.h"

namespace noz::import
{
    struct Animation;
        
    class AnimationImporter : public AssetImporter
    {
    public:
        AnimationImporter(const ImportConfig::ModelConfig& config);
            
        bool canImport(const std::string& filePath) const override;
        bool import(const std::string& sourcePath, const std::string& outputDir) override;
        std::vector<std::string> getSupportedExtensions() const override;
        std::string getName() const override;
            
    private:
        
		ImportConfig::ModelConfig _config;
            
        bool importAnimation(const std::string& sourcePath, const std::string& outputDir);
        bool writeAnimation(
			const std::string& outputPath,
            const std::shared_ptr<GLTFLoader::Animation>& animation,
			const std::string& sourcePath);
    };
}
