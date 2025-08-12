#pragma once

namespace noz::import
{
    class TextureImporter : public AssetImporter
    {
    public:
        TextureImporter(const ImportConfig::TextureConfig& config);

        bool canImport(const std::string& filePath) const override;
        void import(const std::string& sourcePath, const std::string& outputDir) override;
        std::vector<std::string> getSupportedExtensions() const override;
        std::string getName() const override;

    private:
        ImportConfig::TextureConfig _config;

        // Load and process texture
        bool processTexture(const std::string& sourcePath, const std::string& outputPath);

        void writeTexture(
            const std::string& outputPath,
            const void* data,
            size_t size,
            int width,
            int height,
            int channels,
            const std::string& minFilter = "linear",
            const std::string& magFilter = "linear",
            const std::string& clampU = "clamp_to_edge",
            const std::string& clampV = "clamp_to_edge",
            const std::string& clampW = "clamp_to_edge",
            bool generateMipmaps = false);

        void writeTextureWithMips(
            const std::string& outputPath,
            const std::vector<std::vector<uint8_t>>& mipLevels,
            const std::vector<std::pair<int, int>>& mipDimensions,
            const std::string& minFilter,
            const std::string& magFilter,
            const std::string& clampU,
            const std::string& clampV,
            const std::string& clampW);
    };
} 
