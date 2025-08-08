#pragma once

// Forward declarations for FreeType
struct FT_LibraryRec_;
struct FT_FaceRec_;
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;

namespace noz::import
{
    struct FontGlyph;
    struct FontKerning;
    struct FontMetrics;

	namespace ttf
	{
		class TrueTypeFont;
	}	

    class FontImporter : public AssetImporter
    {
    public:
        FontImporter(const ImportConfig::FontConfig& config);
            
        bool canImport(const std::string& filePath) const override;
        bool import(const std::string& sourcePath, const std::string& outputDir) override;
        std::vector<std::string> getSupportedExtensions() const override;
        std::string getName() const override;
            
    private:
            
		ImportConfig::FontConfig _config;
            
		bool importFont(StreamReader* source, const std::string& outputPath);
        bool importFont(const std::string& fontPath, const std::string& outputPath);
            
        bool saveFontData(
			const ttf::TrueTypeFont* ttf,
			const std::string& outputPath,
			const std::vector<unsigned char>& atlasData,
			const glm::ivec2& atlasSize,
            const std::vector<FontGlyph>& glyphs,
			const FontMetrics& metrics,
			int fontSize);
    };
}
