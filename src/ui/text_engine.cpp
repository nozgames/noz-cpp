//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct TextMeshImpl
{
    OBJECT_BASE;
    Mesh* mesh;
    Material* material;
    ivec2 size;
};

static TextMeshImpl* Impl(TextMesh* tm) { return (TextMeshImpl*)Cast(tm, TYPE_TEXT_MESH); }

vec2 MeasureText(const text_t& text, Font* font, float font_size)
{
    assert(font);

    // Calculate text size using glyph data
    float total_width = 0.0f;
    float max_ascent = 0.0f;
    float max_descent = 0.0f;

    for (size_t i = 0; i < text.length; ++i)
    {
        char ch = text.value[i];
        auto glyph = GetGlyph(font, ch);
        total_width += glyph->advance * font_size;

        // Add kerning adjustment for the next character
        if (i + 1 < text.length)
            total_width += GetKerning(font, ch, text.value[i + 1]) * font_size;

        // Calculate glyph extents from baseline
        // bearing.y is now distance from baseline to glyph bottom
        float glyph_bottom = -glyph->bearing.y * font_size;
        float glyph_top = (glyph->size.y - glyph->bearing.y) * font_size;

        max_ascent = max(max_ascent, glyph_top);
        max_descent = min(max_descent, glyph_bottom);
    }

    return vec2(total_width, max_ascent - max_descent);
}

static void AddGlyph(
    MeshBuilder* builder,
    const FontGlyph* glyph,
    float x,
    float y,
    float scale,
    int& vertex_offset)
{
    auto glyph_x = x + glyph->bearing.x * scale;
    // Y increases downward, so glyph_y is the TOP of the glyph
    // bearing.y is distance from baseline to glyph bottom
    // To align baseline at y, glyph top should be at: y + bearing.y - glyph_height
    auto glyph_y = y + glyph->bearing.y * scale - glyph->size.y * scale;
    auto glyph_width = glyph->size.x * scale;
    auto glyph_height = glyph->size.y * scale;
    

    // Add vertices for this glyph quad
    AddVertex(builder, vec3(glyph_x, glyph_y, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(glyph->uv_min.x, glyph->uv_min.y));
    AddVertex(builder, vec3(glyph_x + glyph_width, glyph_y, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(glyph->uv_max.x, glyph->uv_min.y));
    AddVertex(builder, vec3(glyph_x + glyph_width, glyph_y + glyph_height, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(glyph->uv_max.x, glyph->uv_max.y));
    AddVertex(builder, vec3(glyph_x, glyph_y + glyph_height, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec2(glyph->uv_min.x, glyph->uv_max.y));

    // Add indices for this glyph quad
    AddTriangle(builder, vertex_offset, vertex_offset + 1, vertex_offset + 2);
    AddTriangle(builder, vertex_offset, vertex_offset + 2, vertex_offset + 3);

    vertex_offset += 4;
}

static void CreateTextMesh(Allocator* allocator, TextMeshImpl* impl, const TextRequest& request)
{
    auto& text = request.text;

    // Calculate total bounds
    float total_width = 0.0f;
    float max_ascent = 0.0f;
    float max_descent = 0.0f;
    float font_size = (float)request.font_size;

    for (size_t i = 0; i < text.length; ++i)
    {
        char ch = text.value[i];
        auto glyph = GetGlyph(request.font, ch);

        total_width += glyph->advance * font_size;

        if (glyph->uv_max.x <= glyph->uv_min.x || glyph->uv_max.y <= glyph->uv_min.y)
            continue;

        // Add kerning adjustment for the next character
        if (i + 1 < text.length)
            total_width += GetKerning(request.font, ch, text.value[i + 1]) * font_size;

        // Calculate glyph extents from baseline
        // bearing.y is now distance from baseline to glyph bottom
        float glyph_bottom = -glyph->bearing.y * font_size;
        float glyph_top = (glyph->size.y - glyph->bearing.y) * font_size;

        max_ascent = max(max_ascent, glyph_top);
        max_descent = min(max_descent, glyph_bottom);
    }

    float total_height = max_ascent - max_descent;


    // Generate vertices for the text
    float current_x = 0.0f;
    int vertex_offset = 0;

    // Position baseline so that text fits within bounds [0, total_height]
    // Text extends max_ascent above and max_descent below baseline
    // To fit in [0, total_height], baseline should be at max_ascent
    float baseline_y = max_ascent;

    // Get the first glyph to adjust starting position
    if (!IsEmpty(text))
        current_x = -GetGlyph(request.font, text.value[0])->bearing.x * font_size;

    MeshBuilder* builder = CreateMeshBuilder(allocator, (int)text.length * 4, (int)text.length * 6);
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
    auto mesh = CreateMesh(allocator, builder, GetName(text.value));

    impl->mesh = mesh;
    impl->material = GetMaterial(request.font);
    impl->size = { total_width, total_height };
}

Mesh* GetMesh(TextMesh* tm)
{
    return Impl(tm)->mesh;
}

ivec2 GetSize(TextMesh* tm)
{
    return Impl(tm)->size;
}

Material* GetMaterial(TextMesh* tm)
{
    return Impl(tm)->material;
}

void RenderTextMesh(TextMesh* tm)
{
    auto impl = Impl(tm);
    BindMaterial(impl->material);
    DrawMesh(impl->mesh);
}

TextMesh* CreateTextMesh(Allocator* allocator, const TextRequest& request)
{
    auto tm = (TextMesh*)CreateObject(allocator, sizeof(TextMeshImpl), TYPE_TEXT_MESH);
    auto impl = Impl(tm);

    PushScratch();
    CreateTextMesh(ALLOCATOR_SCRATCH, impl, request);
    PopScratch();
    return tm;
}
