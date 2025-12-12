//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct TextMeshImpl : TextMesh {
    Mesh* mesh;
    Material* material;
    Vec2 size;
};

Vec2 MeasureText(const Text& text, Font* font, float font_size) {
    assert(font);

    // Calculate text width using glyph data
    float total_width = 0.0f;

    for (size_t i = 0; i < text.length; ++i)
    {
        char ch = text.value[i];
        auto glyph = GetGlyph(font, ch);
        total_width += glyph->advance * font_size;

        // Add kerning adjustment for the next character
        if (i + 1 < text.length)
            total_width += GetKerning(font, ch, text.value[i + 1]) * font_size;
    }

    // Use consistent line height for all text meshes
    float total_height = GetLineHeight(font) * font_size;

    return Vec2{total_width, total_height};
}

Bounds2 MeasureText(const Text& text, Font* font, float font_size, int start, int end) {
    assert(font);

    float xmax = 0.0f;
    float xmin = 0.0f;

    for (int i = 0; i < text.length && i < end; ++i) {
        if (i == start) xmin = xmax;

        char ch = text.value[i];
        auto glyph = GetGlyph(font, ch);
        xmax += glyph->advance * font_size;

        // Kerning
        if (i + 1 < text.length)
            xmax += GetKerning(font, ch, text.value[i + 1]) * font_size;
    }

    if (start == text.length) xmin = xmax;

    return Bounds2{Vec2{xmin, 0.0f}, Vec2{xmax, GetLineHeight(font) * font_size}};
}

static void AddGlyph(
    MeshBuilder* builder,
    const FontGlyph* glyph,
    float x,
    float y,
    float scale,
    int& vertex_offset) {
    auto glyph_x = x + glyph->bearing.x * scale;
    // Y increases downward, so glyph_y is the TOP of the glyph
    // bearing.y is distance from baseline to glyph bottom
    // To align baseline at y, glyph top should be at: y + bearing.y - glyph_height
    auto glyph_y = y + glyph->bearing.y * scale - glyph->size.y * scale;
    auto glyph_width = glyph->size.x * scale;
    auto glyph_height = glyph->size.y * scale;

    // Add vertices for this glyph quad
    AddVertex(builder, Vec2{glyph_x, glyph_y}, {glyph->uv_min.x, glyph->uv_min.y});
    AddVertex(builder, Vec2{glyph_x + glyph_width, glyph_y}, {glyph->uv_max.x, glyph->uv_min.y});
    AddVertex(builder, Vec2{glyph_x + glyph_width, glyph_y + glyph_height}, {glyph->uv_max.x, glyph->uv_max.y});
    AddVertex(builder, Vec2{glyph_x, glyph_y + glyph_height}, {glyph->uv_min.x, glyph->uv_max.y});

    // Add indices for this glyph quad
    AddTriangle(builder, (u16)vertex_offset, (u16)vertex_offset + 1, (u16)vertex_offset + 2);
    AddTriangle(builder, (u16)vertex_offset, (u16)vertex_offset + 2, (u16)vertex_offset + 3);

    vertex_offset += 4;
}

static void CreateTextMesh(Allocator* allocator, TextMeshImpl* impl, const TextRequest& request) {
    auto& text = request.text;
    float font_size = (float)request.font_size;

    // Calculate total width
    float total_width = 0.0f;
    for (size_t i = 0; i < text.length; ++i) {
        char ch = text.value[i];
        auto glyph = GetGlyph(request.font, ch);
        total_width += glyph->advance * font_size;

        // Add kerning adjustment for the next character
        if (i + 1 < text.length)
            total_width += GetKerning(request.font, ch, text.value[i + 1]) * font_size;
    }

    // Use consistent line height for all text meshes
    float total_height = GetLineHeight(request.font) * font_size;


    // Generate vertices for the text
    float current_x = 0.0f;
    int vertex_offset = 0;

    // Position baseline within the line height bounds
    // The baseline should be positioned from the top of the text bounds
    // so that text renders within the [0, total_height] range
    float baseline_y = total_height - GetBaseline(request.font) * font_size;

    // Get the first glyph to adjust starting position
    // if (!IsEmpty(text))
    //     current_x = -GetGlyph(request.font, text.value[0])->bearing.x * font_size;

    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, (u16)text.length * 4, (u16)text.length * 6);
    for (size_t i = 0; i < text.length; ++i)
    {
        char ch = text.value[i];
        auto glyph = GetGlyph(request.font, ch);

        if (glyph->uv_max.x > glyph->uv_min.x && glyph->uv_max.y > glyph->uv_min.y)
            AddGlyph(builder, glyph, current_x, baseline_y, font_size, vertex_offset);

        current_x += glyph->advance * font_size;

        // Apply kerning adjustment for the next character
        if (i + 1 < text.length)
            current_x += GetKerning(request.font, ch, text.value[i + 1]) * font_size;
    }

    // Create mesh from builder data
    auto mesh = CreateMesh(allocator, builder, nullptr);

    impl->mesh = mesh;
    impl->material = GetMaterial(request.font);
    impl->size = { total_width, total_height };
}

Mesh* GetMesh(TextMesh* tm)
{
    return static_cast<TextMeshImpl*>(tm)->mesh;
}

Vec2 GetSize(TextMesh* tm)
{
    return static_cast<TextMeshImpl*>(tm)->size;
}

Material* GetMaterial(TextMesh* tm)
{
    return static_cast<TextMeshImpl*>(tm)->material;
}

TextMesh* CreateTextMesh(Allocator* allocator, const TextRequest& request)
{
    if (!request.font)
        return nullptr;

    auto tm = (TextMeshImpl*)Alloc(allocator, sizeof(TextMeshImpl));
    auto impl = static_cast<TextMeshImpl*>(tm);

    PushScratch();
    CreateTextMesh(allocator, impl, request);
    PopScratch();
    return tm;
}
