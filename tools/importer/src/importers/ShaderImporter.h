#pragma once

namespace noz::import
{
    class ShaderImporter : public AssetImporter
    {
    public:
        
        ShaderImporter(const ImportConfig::ShaderConfig& config);
            
        bool canImport(const std::string& filePath) const override;
        void import(const std::string& sourcePath, const std::string& outputDir) override;
        std::vector<std::string> getSupportedExtensions() const override;
        std::string getName() const override;
            
    private:

        struct ShaderInfo
        {
            std::string source;
            bool uniformBuffers[16];
            bool samplers[16];
        };

        ImportConfig::ShaderConfig _config;
            
        void import(const std::string& sourcePath, const std::string& outputPath, const MetaFile& meta);
            
        void writeShader(
            const ShaderInfo& vertexShader,
            const ShaderInfo& fragmentShader,
            const MetaFile& meta,
            const std::string& includeDir,
            StreamWriter& writer);
            
        ShaderInfo parseShader(const std::string& source, const std::string& sourcePath, const std::string& stage = "");
        std::string preprocessIncludes(const std::string& source, const std::string& sourcePath);
        std::string preprocessStageDirectives(const std::string& source, const std::string& stage);
        std::string resolveIncludePath(const std::string& includePath, const std::string& sourcePath);
        std::string loadIncludeFile(const std::string& includePath);
            
        void validateVertexShader(const ShaderInfo& shader) const;
        void validateFragmentShader(const ShaderInfo& shader) const;
    };
} 
