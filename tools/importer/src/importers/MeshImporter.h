#pragma once

#include "../GLTFLoader.h"

namespace noz
{
    namespace import
    {
        class MeshImporter : public AssetImporter
        {
        public:
            MeshImporter(const ImportConfig::ModelConfig& config);
            
            bool canImport(const std::string& filePath) const override;
            void import(const std::string& sourcePath, const std::string& outputDir) override;
            std::vector<std::string> getSupportedExtensions() const override;
            std::string getName() const override;
            
        private:

			ImportConfig::ModelConfig _config;
            
            bool processMesh(const std::string& sourcePath, const std::string& outputPath);
            
            bool saveMeshData(const std::string& outputPath, const GLTFLoader::MeshData& meshData, const MetaFile& meta);

			void flatten(GLTFLoader::MeshData& meshData);
        };
    }
} 