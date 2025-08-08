#pragma once

#include "../AssetImporter.h"

namespace noz::import
{
    class AnimationBlendTree2dImporter : public AssetImporter
    {
    public:
		AnimationBlendTree2dImporter();
        
        bool canImport(const std::string& filePath) const override;
        bool import(const std::string& sourcePath, const std::string& outputDir) override;
        std::vector<std::string> getSupportedExtensions() const override;
        std::string getName() const override;
        
    private:
        bool importBlendTree(const std::string& sourcePath, const std::string& outputDir);
        bool writeBlendTree(const std::string& outputPath, const std::string& sourcePath);
    };
}