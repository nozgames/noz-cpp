/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::ui
{
    struct TextMesh
    {
        std::shared_ptr<noz::renderer::Mesh> mesh;
        std::shared_ptr<noz::renderer::Texture> fontTexture;
        glm::vec2 size;
        int vertexCount;
        int indexCount;
    };

    struct TextRequest
    {
        std::string text;
        std::shared_ptr<noz::renderer::Font> font;
        float fontSize;
        glm::vec4 color;
        glm::vec4 outlineColor;
        float outlineWidth;
        
        // For caching
        bool operator==(const TextRequest& other) const;
        size_t hash() const;
    };

	class TextEngine : public ISingleton<TextEngine>
    {
    public:
        ~TextEngine();

        // Generate or retrieve cached text mesh
        std::shared_ptr<TextMesh> getTextMesh(const TextRequest& request);
        
        // Measure text size
        glm::vec2 measureText(const std::string& text, std::shared_ptr<noz::renderer::Font> font, float fontSize);
        
        // Clear cache
        void clearCache();
        
        // Get cache statistics
        size_t getCacheSize() const { return _meshCache.size(); }

    private:

		friend class ISingleton<TextEngine>;

		TextEngine();

        // Generate a new text mesh
        std::shared_ptr<TextMesh> generateTextMesh(const TextRequest& request);
        
        // Helper methods for text processing
        void addGlyphToMesh(noz::renderer::MeshBuilder& builder, const noz::renderer::Font::Glyph& glyph, 
                           float x, float y, float scale, int& vertexOffset);

    private:
        std::unordered_map<size_t, std::shared_ptr<TextMesh>> _meshCache;
    };
} 