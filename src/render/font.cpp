//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#define MAX_GLYPHS 256  // Support extended ASCII
#define MAX_KERNING (MAX_GLYPHS * MAX_GLYPHS)  // All possible kerning pairs

struct FontImpl
{
    OBJECT_BASE;
    const name_t* name;
    Material* material;
    Texture* texture;
    float baseline;
    uint32_t original_font_size;
    float descent;
    float ascent;
    float line_height;
    int atlas_width;
    int atlas_height;
    FontGlyph glyphs[MAX_GLYPHS];     // Fixed array for glyphs (advance == 0 means no glyph)
    uint16_t kerning_index[MAX_KERNING];  // Index into kerning_values array (0xFFFF = no kerning)
    float* kerning_values;                 // Dynamic array of actual kerning values
    uint16_t kerning_count;                // Number of kerning pairs
};

static SDL_GPUDevice* g_device = nullptr;

inline FontImpl* Impl(Font* f) { return (FontImpl*)Cast(f, TYPE_FONT); }

#if 0
static void font_destroy_impl(FontImpl* impl)
{
    assert(impl);
    // Free dynamic kerning values array
    if (impl->kerning_values) {
        free(impl->kerning_values);
        impl->kerning_values = nullptr;
    }
}
#endif

Object* LoadFont(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name)
{
    if (!stream || !header)
        return nullptr;
        
    // Header already validated by LoadAsset
    // Version is in header->version

    auto* impl = (FontImpl*)CreateObject(allocator, sizeof(FontImpl), TYPE_FONT);
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

        if (codepoint < MAX_GLYPHS) {
            // Read directly into the glyph structure
            FontGlyph* glyph = &impl->glyphs[codepoint];
            ReadBytes(stream, glyph, sizeof(FontGlyph));
            
        }
        else {
            // Skip this glyph's data
            SeekBegin(stream, GetPosition(stream) + sizeof(FontGlyph));
        }
    }

    // Read kerning count and kerning data
    impl->kerning_count = ReadU16(stream);

    if (impl->kerning_count > 0) {
        // Allocate kerning values array
        impl->kerning_values = (float*)malloc(impl->kerning_count * sizeof(float));
        if (!impl->kerning_values) {
            impl->kerning_count = 0;
        }
        else {
            // Read all kerning pairs
            for (uint16_t i = 0; i < impl->kerning_count; ++i)
            {
                uint32_t first = ReadU32(stream);
                uint32_t second = ReadU32(stream);
                float amount = ReadFloat(stream);

                // Store in sparse representation
                if (first < MAX_GLYPHS && second < MAX_GLYPHS) {
                    uint32_t index = first * MAX_GLYPHS + second;
                    impl->kerning_index[index] = i;
                    impl->kerning_values[i] = amount;
                }
            }
        }
    }

    // Read atlas data
    uint32_t atlas_data_size = impl->atlas_width * impl->atlas_height; // R8 format
    uint8_t* atlas_data = (uint8_t*)malloc(atlas_data_size);
    if (!atlas_data)
    {
        Destroy((Font*)impl);
        return nullptr;
    }

    ReadBytes(stream, atlas_data, atlas_data_size);
    // Note: stream destruction handled by caller

    impl->texture = CreateTexture(allocator, atlas_data, impl->atlas_width, impl->atlas_height, TEXTURE_FORMAT_R8, name);
    free(atlas_data);

    if (!impl->texture)
    {
        Destroy((Font*)impl);
        return nullptr;
    }

    return (Object*)impl;
}

const FontGlyph* GetGlyph(Font* font, char ch)
{
    FontImpl* impl = Impl(font);
    
    // Check if glyph exists (advance > 0 means valid glyph)
    unsigned char index = (unsigned char)ch;
    if (index < MAX_GLYPHS && impl->glyphs[index].advance > 0.0f) 
    {
        return &impl->glyphs[index];
    }

    // If character not found, try unknown glyph (ASCII DEL character)
    unsigned char unknown_ch = 0x7F;
    if (unknown_ch < MAX_GLYPHS && impl->glyphs[unknown_ch].advance > 0.0f) 
    {
        return &impl->glyphs[unknown_ch];
    }

    // Return default glyph if nothing found
    static FontGlyph default_glyph = {
        {0.0f, 0.0f},   // uv_min
        {0.0f, 0.0f},   // uv_max
        {0.0f, 0.0f},   // size
        0.0f,           // advance
        {0.0f, 0.0f},   // bearing
        {0.0f, 0.0f}    // sdf_offset
    };

    return &default_glyph;
}

float GetKerning(Font* font, char first, char second)
{
    auto* impl = Impl(font);
    
    auto f = (unsigned char)first;
    auto s = (unsigned char)second;
    if (f < MAX_GLYPHS && s < MAX_GLYPHS)
    {
        uint32_t index = f * MAX_GLYPHS + s;
        uint16_t value_index = impl->kerning_index[index];
        if (value_index != 0xFFFF && value_index < impl->kerning_count) 
            return impl->kerning_values[value_index];
    }

    return 0.0f;
}

float GetBaseline(Font* font)
{
    return Impl(font)->baseline;
}

Material* GetMaterial(Font* font)
{
    auto impl = Impl(font);
    if (impl->material == nullptr)
    {
        impl->material = CreateMaterial(GetAllocator(font), CoreAssets.shaders.text);
        SetTexture(impl->material, impl->texture);
    }

    return impl->material;
}

void InitFont(RendererTraits* traits, SDL_GPUDevice* device)
{
    g_device = device;
}

void ShutdownFont()
{
    g_device = nullptr;
}

