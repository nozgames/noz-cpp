/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::renderer
{
    Font::Font(const std::string& path) : Asset(path)
    {
    }

    Font::~Font()
    {
    }

    std::shared_ptr<Font> Font::load(const std::string& path)
    {
        auto font = std::make_shared<Font>(path);
		font->loadInternal();
        return font;
    }

    void Font::loadInternal()
    {
        std::string fullPath = AssetDatabase::getFullPath(name(), "font");
        
        StreamReader reader;
        if (!reader.loadFromFile(fullPath))
			throw std::runtime_error("invalid file");

        // Read header
        if (!reader.readFileSignature("FONT"))
			throw std::runtime_error("invalid format");
            
        uint32_t version = reader.readUInt32();
        if (version != 1)
			throw std::runtime_error("invalid version");
            
        auto originalFontSize = reader.readUInt32();
        auto atlasWidth = reader.readUInt32();
		auto atlasHeight = reader.readUInt32();
		auto ascent = reader.readFloat();
		auto descent = reader.readFloat();
		auto lineHeight = reader.readFloat();
        _baseline = reader.readFloat();
            
        // Read glyph count and glyph data
        auto glyphCount = reader.readUInt16();
        for (uint32_t i = 0; i < glyphCount; ++i)
        {
			auto codepoint = reader.readUInt32();
			auto uvx = reader.readFloat();
			auto uvy = reader.readFloat();
			auto stx = reader.readFloat();
			auto sty = reader.readFloat();
			auto width = reader.readFloat();
			auto height = reader.readFloat();
			auto advanceX = reader.readFloat();
			auto advanceY = reader.readFloat();
			auto bearingX = reader.readFloat();
			auto bearingY = reader.readFloat();
			auto sdfDistance = reader.readFloat();

            // Convert to our internal Glyph format
            Glyph glyph;
            glyph.uvMin = glm::vec2(uvx, uvy);
			glyph.uvMax = glm::vec2(stx, sty);
            glyph.size = glm::vec2(width, height);
            glyph.advance = advanceX;
            glyph.bearing = glm::vec2(bearingX, bearingY);
            glyph.sdfOffset = glm::vec2(sdfDistance, sdfDistance);

            _glyphs[static_cast<char>(codepoint)] = glyph;
        }
            
        // Read kerning count and kerning data
		auto kerningCount = reader.readUInt16();
		_kerning.reserve(kerningCount);
        for (uint32_t i = 0; i < kerningCount; ++i)
        {
            auto first = reader.readUInt32();
			auto second = reader.readUInt32();
			auto amount = reader.readFloat();
			uint64_t key = (static_cast<uint64_t>(first) << 32) | static_cast<uint64_t>(second);
			_kerning.insert({ key, amount });
        }

        // Read atlas texture data
        std::vector<uint8_t> atlasData = reader.readBytes();

        // Get GPU device from renderer
		assert(Renderer::instance());
		assert(Renderer::instance()->GetGPUDevice());

        // Create texture from atlas data using actual dimensions
        _texture = std::make_shared<Texture>("font_atlas");
        if (!_texture->createFromMemory(Renderer::instance()->GetGPUDevice(), atlasData.data(), atlasWidth, atlasHeight, 1))
			throw std::runtime_error("Failed to create font texture from atlas data");
    }

    Font::Glyph Font::glyph(char ch) const
    {
        // Check if we have atlas data for this character
        auto it = _glyphs.find(ch);
        if (it != _glyphs.end())
        {
            return it->second;
        }
        
        // If character not found, use unknown glyph (ASCII DEL character)
        auto unknownIt = _glyphs.find(0x7F);
        if (unknownIt != _glyphs.end())
        {
            return unknownIt->second;
        }
        
        // Last resort: provide default glyph data
        Glyph defaultGlyph;
        defaultGlyph.uvMin = glm::vec2(0.0f, 0.0f);
        defaultGlyph.uvMax = glm::vec2(1.0f, 1.0f);
        defaultGlyph.size = glm::vec2(1.0f, 1.0f);
        defaultGlyph.advance = 1.0f;
        defaultGlyph.bearing = glm::vec2(0.0f, 0.0f);
        defaultGlyph.sdfOffset = glm::vec2(0.0f, 0.0f);
        return defaultGlyph;
    }
    
    float Font::kerning(char first, char second) const
    {
        uint64_t key = (static_cast<uint64_t>(first) << 32) | static_cast<uint64_t>(second);
        auto it = _kerning.find(key);
		if (it == _kerning.end())
			return 0.0f;
		
		return it->second;
    }
} 
