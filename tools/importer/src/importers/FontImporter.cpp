/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "FontImporter.h"
#include "../ttf/TrueTypeFont.h"
#include "../msdf/msdf.h"

namespace noz::import
{
	struct FontGlyph
	{
		const ttf::TrueTypeFont::Glyph* ttf;
		glm::ivec2 size;
		glm::dvec2 scale;
		glm::ivec2 packedSize;
		glm::ivec2 advance;
		RectPacker::BinRect packedRect;
		glm::ivec2 bearing;
		char ascii;
	};

    struct FontKerning
    {
		uint32_t first;
		uint32_t second;
        float amount;
    };

    struct FontMetrics
    {
		float ascent;
		float descent;
        float lineHeight;
        float baseline;
    };

	glm::ivec2 roundToNearest(const glm::vec2& v) 
	{
		return glm::ivec2((int)(v.x + 0.5f), (int)(v.y + 0.5f));
	}

	int roundToNearest(float v)
	{
		return (int)(v + 0.5f);
	}

	glm::ivec2 roundToNearest(const glm::dvec2& v)
	{
		return glm::ivec2((int)(v.x + 0.5f), (int)(v.y + 0.5f));
	}

    FontImporter::FontImporter(const ImportConfig::FontConfig& config)
        : _config(config)
    {
    }
        
    bool FontImporter::canImport(const std::string& filePath) const
    {
        std::filesystem::path path(filePath);
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            
        return extension == ".ttf" || extension == ".otf";
    }
        
    bool FontImporter::import(const std::string& sourcePath, const std::string& outputDir)
    {
        try
        {
            std::filesystem::path source(sourcePath);
            std::filesystem::path output(outputDir);
                
            // Create output filename
            std::string outputName = source.stem().string() + ".font";
            std::filesystem::path outputPath = output / outputName;
                
            std::cout << "Processing font: " << sourcePath << std::endl;
                
            return importFont(sourcePath, outputPath.string());
        }
        catch (const std::exception& e)
        {
            std::cerr << "Font import error: " << e.what() << std::endl;
            return false;
        }
    }
        
    std::vector<std::string> FontImporter::getSupportedExtensions() const
    {
        return {".ttf", ".otf"};
    }
        
    std::string FontImporter::getName() const
    {
        return "FontImporter";
    }
        
    bool FontImporter::importFont(const std::string& fontPath, const std::string& outputPath)
    {
        // Load meta file configuration for the font (if it exists)
        std::string metaPath = fontPath + ".meta";
        auto meta = noz::MetaFile::parse(metaPath);
                 
        _config.fontSize = meta.getInt("Font", "fontSize", _config.fontSize);
        _config.characters = meta.getString("Font", "characters", _config.characters);
        _config.sdfPadding = meta.getInt("Font", "sdfPadding", _config.sdfPadding);
                        
        // Load font file
        std::ifstream file(fontPath, std::ios::binary);
        if (!file.is_open())
        {
            std::cerr << "Failed to open font file: " << fontPath << std::endl;
            return false;
        }
            
        // Get file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
            
        // Read font data
        std::vector<unsigned char> fontData(fileSize);
        file.read(reinterpret_cast<char*>(fontData.data()), fileSize);
        file.close();

		StreamReader reader(fontData);
		return importFont(&reader, outputPath);
    }
        
    bool FontImporter::saveFontData(
		const ttf::TrueTypeFont* ttf,
		const std::string& outputPath,
		const std::vector<unsigned char>& atlasData,
		const glm::ivec2& atlasSize,
		const std::vector<FontGlyph>& glyphs,
		const FontMetrics& metrics,
		int fontSize)
    {
        StreamWriter writer;
            
        // Write header
        writer.writeFileSignature("FONT");
        writer.writeUInt32(1); // Version
            
        // Write font size (this is important for runtime scaling)
        writer.writeUInt32(static_cast<uint32_t>(fontSize));
            
        // Write atlas dimensions
        int atlasWidth = atlasSize.x;
        int atlasHeight = atlasSize.y;
        writer.writeUInt32(static_cast<uint32_t>(atlasWidth));
        writer.writeUInt32(static_cast<uint32_t>(atlasHeight));
            
        // Write font metrics
        writer.writeFloat(float(ttf->ascent()) / fontSize);
        writer.writeFloat(float(ttf->descent()) / fontSize);
        writer.writeFloat(float(ttf->height()) / fontSize);
        writer.writeFloat(float(ttf->ascent()) / fontSize);
            
        // Write glyph count and glyph data
        writer.writeUInt16(static_cast<uint16_t>(glyphs.size()));
        for (const auto& glyph : glyphs)
        {
            writer.writeUInt32(glyph.ascii);
            writer.writeFloat(glyph.packedRect.x / float(atlasWidth));
            writer.writeFloat(glyph.packedRect.y / float(atlasHeight));
            writer.writeFloat((glyph.packedRect.x + glyph.packedRect.w) / float(atlasWidth));
            writer.writeFloat((glyph.packedRect.y + glyph.packedRect.h) / float(atlasHeight));
            writer.writeFloat(float(glyph.size.x) / fontSize);
            writer.writeFloat(float(glyph.size.y) / fontSize);
            writer.writeFloat(float(glyph.advance.x) / fontSize);
            writer.writeFloat(0.0f);
            writer.writeFloat(float(glyph.bearing.x) / fontSize);
            writer.writeFloat(float(-glyph.bearing.y) / fontSize);
            writer.writeFloat(0.0f);
        }
            
        // Write kerning count and kerning data
        writer.writeUInt16(static_cast<uint16_t>(ttf->kerning().size()));
        for (const auto& k : ttf->kerning())
        {
            writer.writeUInt32(k.left);
            writer.writeUInt32(k.right);
            writer.writeFloat(k.value);
        }
            
        writer.writeBytes(atlasData);
            
        return writer.writeToFile(outputPath);
    }

	bool FontImporter::importFont(StreamReader* source, const std::string& outputPath)
	{
		auto ttf = std::shared_ptr<ttf::TrueTypeFont>(ttf::TrueTypeFont::load(source, _config.fontSize, _config.characters));

		// Build the imported glyph list.
		std::vector<FontGlyph> glyphs;
		for (size_t i = 0; i < _config.characters.size(); i++)
		{
			auto ttfGlyph = ttf->glyph(_config.characters[i]);
			if (ttfGlyph == nullptr)
				continue;

			FontGlyph iglyph{};
			iglyph.ascii = _config.characters[i];
			iglyph.ttf = ttfGlyph;

			//if (!iglyph.ttf->contours.empty())
			{
				iglyph.size = roundToNearest(ttfGlyph->size + glm::dvec2(_config.sdfPadding * 2));
				iglyph.scale = glm::dvec2(iglyph.size.x, iglyph.size.y) / ttfGlyph->size;
				iglyph.packedSize = iglyph.size + (_config.padding + _config.sdfPadding) * 2;
				iglyph.bearing = roundToNearest(ttfGlyph->bearing);
				iglyph.advance.x = roundToNearest((float)ttfGlyph->advance);
			}
			
			glyphs.push_back(iglyph);
		}

		// Pack the glyphs
		int minHeight = (int)math::nextPowerOf2((uint32_t)(_config.fontSize + 2 + _config.sdfPadding * 2 + _config.padding * 2));
		RectPacker packer (minHeight, minHeight);

		while (packer.empty())
		{
			for(auto& glyph : glyphs)
			{
				if (glyph.ttf->contours.size() == 0)
					continue;

				if (-1 == packer.Insert(glyph.packedSize, RectPacker::Method::BestLongSideFit, glyph.packedRect))
				{
					RectPacker::BinSize size = packer.size();
					if (size.w <= size.h)
						size.w <<= 1;
					else
						size.h <<= 1;

					packer.Resize(size.w, size.h);
					break;
				}
			}
		}

		if (!packer.validate())
			throw std::runtime_error("RectPacker validation failed");

		auto imageSize = glm::ivec2(packer.size().w, packer.size().h);
		std::vector<uint8_t> image;
		image.resize(imageSize.x * imageSize.y, 0);

		for (size_t i = 0; i < glyphs.size(); i++)
		{
			auto glyph = glyphs[i];
			if (glyph.ttf->contours.size() == 0)
				continue;

			msdf::renderGlyph(
				glyph.ttf,
				image,
				imageSize.x,
				glm::ivec2(
					glyph.packedRect.x + _config.padding,
					glyph.packedRect.y + _config.padding
				),
				glm::ivec2(
					glyph.packedRect.w - _config.padding * 2,
					glyph.packedRect.h - _config.padding * 2
				),
				_config.sdfPadding,
				glyph.scale,
				glm::dvec2(
					-glyph.ttf->bearing.x + _config.sdfPadding,
					(glyph.ttf->size.y - glyph.ttf->bearing.y) + _config.sdfPadding
				)
			);
		}

		FontMetrics metrics{};
		return saveFontData(
			ttf.get(),
			outputPath,
			image,
			imageSize,
			glyphs,
			metrics,
			_config.fontSize);
	}
} 