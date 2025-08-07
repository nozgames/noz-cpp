#pragma once

namespace noz
{
    namespace import
    {
        class ShaderImporter : public ResourceImporter
        {
        public:
            ShaderImporter(const ImportConfig::ShaderConfig& config);
            
            bool canImport(const std::string& filePath) const override;
            bool import(const std::string& sourcePath, const std::string& outputDir) override;
            std::vector<std::string> getSupportedExtensions() const override;
            std::string getName() const override;
            
        private:
            ImportConfig::ShaderConfig _config;
            
            // Process shader (preprocess, validate)
            bool processShader(const std::string& sourcePath, const std::string& outputPath);
            
            // Save processed shader
            bool writeShader(const std::string& outputPath, const std::string& vertexShader, const std::string& fragmentShader, const std::string& sourcePath);
            
            // Preprocessing methods
            std::string preprocessShader(const std::string& source, const std::string& sourcePath, const std::string& stage = "");
            std::string processIncludes(const std::string& source, const std::string& sourcePath);
            std::string processStageDirectives(const std::string& source, const std::string& stage);
            std::string resolveIncludePath(const std::string& includePath, const std::string& sourcePath);
            std::string loadIncludeFile(const std::string& includePath);
            std::string processDefines(const std::string& source, const std::string& additionalDefines = "");
            std::string removeComments(const std::string& source);
            
            // Validation
            bool validateShader(const std::string& shaderSource);
        };
    }
} 