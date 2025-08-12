/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::renderer
{
    class Font : public noz::Asset
    {
    public:

        NOZ_DECLARE_TYPEID(Font, Asset)

        ~Font();
        
        std::shared_ptr<Texture> texture() const;
        std::shared_ptr<Material> material() const;

        // Utility methods
        bool isLoaded() const { return _texture != nullptr; }
        int size() const { return 16; } // Default size, could be stored if needed
        const std::string& path() const { return name(); }
        
        // Font metrics
        float baseline() const { return _baseline; }
        
        // Glyph information
        struct Glyph
        {
            glm::vec2 uvMin;
            glm::vec2 uvMax;
            glm::vec2 size;
            float advance;
            glm::vec2 bearing;
            glm::vec2 sdfOffset; // SDF distance field offset
        };
        
        Glyph glyph(char ch) const;
        
        // Get kerning between two characters
        float kerning(char first, char second) const;

    private:

		friend class AssetDatabase;

        static std::shared_ptr<Font> load(const std::string& name);

        Font();

		void loadInternal();

		std::shared_ptr<Material> _material;
		std::shared_ptr<Texture> _texture;
        float _baseline = 0.0f;
        std::unordered_map<char, Glyph> _glyphs;
		std::unordered_map<uint64_t, float> _kerning;
    };

    inline std::shared_ptr<Texture> Font::texture() const { return _texture; }
    inline std::shared_ptr<Material> Font::material() const { return _material; }
}
