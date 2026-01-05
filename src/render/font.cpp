//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#define MAX_GLYPHS 256                        // Support extended ASCII
#define MAX_KERNING (MAX_GLYPHS * MAX_GLYPHS) // All possible kerning pairs

Font** FONT = nullptr;
int FONT_COUNT = 0;
Font* FONT_DEFAULT = nullptr;

struct FontImpl : Font {
    Material* material;
    Texture* texture;
    float baseline;
    uint32_t original_font_size;
    float descent;
    float ascent;
    float line_height;
    int atlas_width;
    int atlas_height;
    FontGlyph glyphs[MAX_GLYPHS];        // Fixed array for glyphs (advance == 0 means no glyph)
    uint16_t kerning_index[MAX_KERNING]; // Index into kerning_values array (0xFFFF = no kerning)
    float* kerning_values;               // Dynamic array of actual kerning values
    uint16_t kerning_count;              // Number of kerning pairs
};

struct TextBuffer {
    Color outline_color;
    float outline_width;
    float padding0;
    float padding1;
    float padding2;
};

// static SDL_GPUDevice* g_device = nullptr;

void FontDestructor(void* p) {
    FontImpl* impl = (FontImpl*)p;
    assert(impl);

    Free(impl->kerning_values);
    Free(impl->material);
    Free(impl->texture);
}

Asset* LoadFont(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)name_table;

    if (!stream || !header)
        return nullptr;

    // Header already validated by LoadAsset
    // Version is in header->version

    auto* impl = (FontImpl*)Alloc(allocator, sizeof(FontImpl));
    if (!impl)
        return nullptr;

    // Name no longer stored in font

    // Arrays are already zeroed by object_create
    // Initialize kerning index to 0xFFFF (no kerning)
    memset(impl->kerning_index, 0xFF, sizeof(impl->kerning_index));
    impl->kerning_values = nullptr;
    impl->kerning_count = 0;
    impl->name = name;
    impl->original_font_size = ReadU32(stream);
    impl->atlas_width = (int)ReadU32(stream);
    impl->atlas_height = (int)ReadU32(stream);
    impl->ascent = ReadFloat(stream);
    impl->descent = ReadFloat(stream);
    impl->line_height = ReadFloat(stream);
    impl->baseline = ReadFloat(stream);

    // Read glyph count and glyph data
    uint16_t glyph_count = ReadU16(stream);

    // New efficient format: codepoint, then font_glyph structure directly
    for (uint32_t i = 0; i < glyph_count; ++i)
    {
        uint32_t codepoint = ReadU32(stream);

        if (codepoint < MAX_GLYPHS)
        {
            // Read directly into the glyph structure
            FontGlyph* glyph = &impl->glyphs[codepoint];
            ReadBytes(stream, glyph, sizeof(FontGlyph));
        }
        else
        {
            // Skip this glyph's data
            SeekBegin(stream, GetPosition(stream) + (u32)sizeof(FontGlyph));
        }
    }

    // Read kerning count and kerning data
    impl->kerning_count = ReadU16(stream);

    if (impl->kerning_count > 0)
    {
        impl->kerning_values = (float*)Alloc(allocator, impl->kerning_count * sizeof(float));

        // Read all kerning pairs
        for (uint16_t i = 0; i < impl->kerning_count; ++i)
        {
            uint32_t first = ReadU32(stream);
            uint32_t second = ReadU32(stream);
            float amount = ReadFloat(stream);

            // Store in sparse representation
            if (first < MAX_GLYPHS && second < MAX_GLYPHS)
            {
                uint32_t index = first * MAX_GLYPHS + second;
                impl->kerning_index[index] = i;
                impl->kerning_values[i] = amount;
            }
        }
    }

    // Read atlas data
    uint32_t atlas_data_size = impl->atlas_width * impl->atlas_height; // R8 format
    uint8_t* atlas_data = (uint8_t*)Alloc(ALLOCATOR_SCRATCH, atlas_data_size);
    if (!atlas_data)
    {
        // todo: free without allocator, stuff allocator with destructor?
        // Free(impl);
        return nullptr;
    }

    ReadBytes(stream, atlas_data, atlas_data_size);

    impl->texture =
        CreateTexture(allocator, atlas_data, impl->atlas_width, impl->atlas_height, TEXTURE_FORMAT_R8, name);
    Free(atlas_data);

    if (!impl->texture)
    {
        Free(impl);
        return nullptr;
    }

    return impl;
}

const FontGlyph* GetGlyph(Font* font, char ch)
{
    FontImpl* impl = static_cast<FontImpl*>(font);

    // Check if glyph exists (advance > 0 means valid glyph)
    int index = (unsigned char)ch;
    if (index < MAX_GLYPHS && impl->glyphs[index].advance > 0.0f)
    {
        return &impl->glyphs[index];
    }

    // If character not found, try unknown glyph (ASCII DEL character)
    int unknown_ch = 0x7F;
    if (unknown_ch < MAX_GLYPHS && impl->glyphs[unknown_ch].advance > 0.0f)
    {
        return &impl->glyphs[unknown_ch];
    }

    // Return default glyph if nothing found
    static FontGlyph default_glyph = {
        {0.0f, 0.0f}, // uv_min
        {0.0f, 0.0f}, // uv_max
        {0.0f, 0.0f}, // size
        0.0f,         // advance
        {0.0f, 0.0f}, // bearing
    };

    return &default_glyph;
}

float GetKerning(Font* font, char first, char second)
{
    (void)font;
    (void)first;
    (void)second;

    FontImpl* impl = static_cast<FontImpl*>(font);
    u8 f = (u8)first;
    u8 s = (u8)second;

    uint32_t index = f * MAX_GLYPHS + s;
    uint16_t value_index = impl->kerning_index[index];
    if (value_index != 0xFFFF && value_index < impl->kerning_count)
        return impl->kerning_values[value_index];

    return 0.0f;
}

float GetBaseline(Font* font)
{
    return static_cast<FontImpl*>(font)->baseline;
}

float GetLineHeight(Font* font)
{
    return static_cast<FontImpl*>(font)->line_height;
}

Material* GetMaterial(Font* font) {
    FontImpl* impl = static_cast<FontImpl*>(font);
    if (impl->material == nullptr) {
        impl->material = CreateMaterial(GetAllocator(font), SHADER_TEXT);
        TextBuffer buffer = {
            .outline_color = COLOR_BLACK,
            .outline_width = 0.0f
        };
        SetFragmentData(impl->material, &buffer, sizeof(TextBuffer));
        SetTexture(impl->material, impl->texture);
    }

    return impl->material;
}

int GetFontIndex(const Name* name)
{
    assert(FONT);
    assert(name);

    for (int i = 0; FONT[i]; i++)
        if (FONT[i]->name == name)
            return i;

    return -1;
}

Material* CreateMaterial(Allocator* allocator, Font* font) {
    assert(font);
    Material* material = CreateMaterial(allocator, GetShader(GetMaterial(font)));
    FontImpl* impl = static_cast<FontImpl*>(font);
    SetTexture(material, impl->texture);
    return material;
}

Material* CreateMaterial(Allocator* allocator, Font* font, float outline_size, Color outline_color) {
    assert(font);
    FontImpl* impl = static_cast<FontImpl*>(font);
    Material* material = CreateMaterial(allocator, GetShader(GetMaterial(font)));
    TextBuffer buffer = {
        .outline_color = outline_color,
        .outline_width = outline_size
    };

    SetFragmentData(material, &buffer, sizeof(TextBuffer));
    SetTexture(material, impl->texture);
    return material;
}
