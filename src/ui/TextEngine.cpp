/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/ui/TextEngine.h>

namespace noz::ui
{
    bool TextRequest::operator==(const TextRequest& other) const
    {
        return text == other.text &&
               font == other.font &&
               fontSize == other.fontSize &&
               color == other.color &&
               outlineColor == other.outlineColor &&
               outlineWidth == other.outlineWidth;
    }

    size_t TextRequest::hash() const
    {
        size_t hash = std::hash<std::string>{}(text);
        hash ^= std::hash<std::shared_ptr<noz::renderer::Font>>{}(font) << 1;
        hash ^= std::hash<float>{}(fontSize) << 2;
        hash ^= std::hash<float>{}(color.r) << 3;
        hash ^= std::hash<float>{}(color.g) << 4;
        hash ^= std::hash<float>{}(color.b) << 5;
        hash ^= std::hash<float>{}(color.a) << 6;
        hash ^= std::hash<float>{}(outlineColor.r) << 7;
        hash ^= std::hash<float>{}(outlineColor.g) << 8;
        hash ^= std::hash<float>{}(outlineColor.b) << 9;
        hash ^= std::hash<float>{}(outlineColor.a) << 10;
        hash ^= std::hash<float>{}(outlineWidth) << 11;
        return hash;
    }

    TextEngine::TextEngine()
    {
    }

    TextEngine::~TextEngine()
    {
        clearCache();
    }

    std::shared_ptr<TextMesh> TextEngine::getTextMesh(const TextRequest& request)
    {
        size_t hash = request.hash();
        
        // Check if we have this text mesh cached
        auto it = _meshCache.find(hash);
        if (it != _meshCache.end())
        {
            return it->second;
        }
        
        // Generate new text mesh
        auto textMesh = generateTextMesh(request);
        if (textMesh)
        {
            _meshCache[hash] = textMesh;
        }
        
        return textMesh;
    }

    glm::vec2 TextEngine::measureText(const std::string& text, std::shared_ptr<noz::renderer::Font> font, float fontSize)
    {
        if (!font || !font->isLoaded())
            return glm::vec2(0.0f);
        
        // Calculate text size using glyph data with proper baseline and bearing
        float totalWidth = 0.0f;
        float minY = 0.0f;
        float maxY = 0.0f;
        
        for (size_t i = 0; i < text.length(); ++i)
        {
            char ch = text[i];
            noz::renderer::Font::Glyph glyph = font->glyph(ch);
            totalWidth += glyph.advance * fontSize;
            
            // Add kerning adjustment for the next character
            if (i + 1 < text.length())
            {
                float kerningAmount = font->kerning(ch, text[i + 1]) * fontSize;
                totalWidth += kerningAmount;
            }
            
            // Calculate glyph bounds including bearing
            float glyphMinY = (font->baseline() + glyph.bearing.y) * fontSize;
            float glyphMaxY = glyphMinY + glyph.size.y * fontSize;
            
            minY = std::min(minY, glyphMinY);
            maxY = std::max(maxY, glyphMaxY);
        }
        
        return glm::vec2(totalWidth, maxY - minY);
    }

    void TextEngine::clearCache()
    {
        _meshCache.clear();
    }

    std::shared_ptr<TextMesh> TextEngine::generateTextMesh(const TextRequest& request)
    {
        if (!request.font || !request.font->isLoaded())
            return nullptr;

        noz::renderer::MeshBuilder builder;
        
        // Simple single-line text rendering
        const std::string& text = request.text;
        
        // Calculate total bounds with proper baseline and bearing
        float totalWidth = 0.0f;
        float minY = 0.0f;
        float maxY = 0.0f;
        
        for (size_t i = 0; i < text.length(); ++i)
        {
            char ch = text[i];
            auto glyph = request.font->glyph(ch);
            totalWidth += glyph.advance * request.fontSize;
            
            // Add kerning adjustment for the next character
            if (i + 1 < text.length())
            {
                float kerningAmount = request.font->kerning(ch, text[i + 1]) * request.fontSize;
                totalWidth += kerningAmount;
            }
            
            // Calculate glyph bounds including bearing
            float glyphMinY = (request.font->baseline() + glyph.bearing.y) * request.fontSize;
            float glyphMaxY = glyphMinY + glyph.size.y * request.fontSize;
            
            minY = std::min(minY, glyphMinY);
            maxY = std::max(maxY, glyphMaxY);
        }
        
        float totalHeight = maxY - minY;
        
        // Generate vertices for the text
        float currentX = 0.0f;
        int vertexOffset = 0;
        
        // Calculate baseline position using font metrics
        float baselineY = request.font->baseline() * request.fontSize;
        
        // Get the first glyph to adjust starting position
        if (!text.empty())
        {
            auto firstGlyph = request.font->glyph(text[0]);
            currentX = -firstGlyph.bearing.x * request.fontSize; // Adjust for first glyph bearing
        }
        
        for (size_t i = 0; i < text.length(); ++i)
        {
            char ch = text[i];
            auto glyph = request.font->glyph(ch);
            addGlyphToMesh(builder, glyph, currentX, baselineY, request.fontSize, vertexOffset);
            currentX += glyph.advance * request.fontSize;
            
            // Apply kerning adjustment for the next character
            if (i + 1 < text.length())
            {
                float kerningAmount = request.font->kerning(ch, text[i + 1]) * request.fontSize;
                currentX += kerningAmount;
            }
        }
        
        // Create mesh from builder data
        auto mesh = std::make_shared<noz::renderer::Mesh>("text_mesh");
        mesh->positions() = builder.positions();
        mesh->normals() = builder.normals();
        mesh->uv0() = builder.uv0();
        mesh->boneIndices() = builder.boneIndices();
        mesh->indices() = builder.indices();
        
        if (mesh)
        {
            mesh->upload(true); // Upload to GPU and clear CPU memory
        }
        
        // Create TextMesh result
        auto textMesh = std::make_shared<TextMesh>();
        textMesh->mesh = mesh;
        textMesh->fontTexture = request.font->texture();
        textMesh->size = glm::vec2(totalWidth, totalHeight);
        textMesh->vertexCount = static_cast<int>(builder.positions().size());
        textMesh->indexCount = static_cast<int>(builder.indices().size());
        
        return textMesh;
    }



    void TextEngine::addGlyphToMesh(
		noz::renderer::MeshBuilder& builder,
		const noz::renderer::Font::Glyph& glyph, 
		float x,
		float y,
		float scale,
		int& vertexOffset)
    {
        auto glyphX = x + glyph.bearing.x * scale;
        auto glyphY = y + glyph.bearing.y * scale; // Negative because Y increases downward in screen space
        auto glyphWidth = glyph.size.x * scale;
        auto glyphHeight = glyph.size.y * scale;
        
        // Add vertices for this glyph quad
        builder.addVertex(glm::vec3(glyphX, glyphY, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(glyph.uvMin.x, glyph.uvMin.y));
        builder.addVertex(glm::vec3(glyphX + glyphWidth, glyphY, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(glyph.uvMax.x, glyph.uvMin.y));
        builder.addVertex(glm::vec3(glyphX + glyphWidth, glyphY + glyphHeight, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(glyph.uvMax.x, glyph.uvMax.y));
        builder.addVertex(glm::vec3(glyphX, glyphY + glyphHeight, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(glyph.uvMin.x, glyph.uvMax.y));
        
        // Add indices for this glyph quad
        builder.addTriangle(vertexOffset, vertexOffset + 1, vertexOffset + 2);
        builder.addTriangle(vertexOffset, vertexOffset + 2, vertexOffset + 3);
        
        vertexOffset += 4;
    }
} 