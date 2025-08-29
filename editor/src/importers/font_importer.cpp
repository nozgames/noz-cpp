//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
// @STL

#include <noz/asset.h>
#include <noz/noz.h>
#include <noz/noz_math.h>
#include "../rect_packer.h"
#include "../ttf/TrueTypeFont.h"
#include "../msdf/msdf.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace noz;

struct ImportFontGlyph
{
    const ttf::TrueTypeFont::Glyph* ttf;
    ivec2 size;
    dvec2 scale;
    ivec2 packedSize;
    ivec2 advance;
    rect_packer::BinRect packedRect;
    ivec2 bearing;
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

static void WriteFontData(
    Stream* stream,
    const ttf::TrueTypeFont* ttf,
    const std::vector<unsigned char>& atlasData,
    const ivec2& atlasSize,
    const std::vector<ImportFontGlyph>& glyphs,
    int font_size)
{
    // Write asset header
    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE_FONT;
    header.version = 1;
    header.flags = 0;
    WriteAssetHeader(stream, &header);

    // Write font size (this is important for runtime scaling)
    WriteU32(stream, static_cast<uint32_t>(font_size));

    // Write atlas dimensions
    WriteU32(stream, static_cast<uint32_t>(atlasSize.x));
    WriteU32(stream, static_cast<uint32_t>(atlasSize.y));

    // Write font metrics
    auto font_size_inv = 1.0f / (float)font_size;
    WriteFloat(stream, (float)ttf->ascent() * font_size_inv);
    WriteFloat(stream, (float)ttf->descent() * font_size_inv);
    WriteFloat(stream, (float)ttf->height() * font_size_inv);
    WriteFloat(stream, 0.0f);  // Baseline should be 0 (reference point)

    // Write glyph count and glyph data
    WriteU16(stream, static_cast<uint16_t>(glyphs.size()));
    for (const auto& glyph : glyphs)
    {
        WriteU32(stream, glyph.ascii);
        WriteFloat(stream, (float)glyph.packedRect.x / (float)atlasSize.x);
        WriteFloat(stream, (float)glyph.packedRect.y / (float)atlasSize.y);
        WriteFloat(stream, (float)(glyph.packedRect.x + glyph.packedRect.w) / (float)atlasSize.x);
        WriteFloat(stream, (float)(glyph.packedRect.y + glyph.packedRect.h) / (float)atlasSize.y);
        WriteFloat(stream, (float)glyph.size.x * font_size_inv);
        WriteFloat(stream, (float)glyph.size.y * font_size_inv);
        WriteFloat(stream, (float)glyph.advance.x * font_size_inv);
        float bearing_x = (float)glyph.bearing.x * font_size_inv;
        // Use original TTF size for bearing calculation, not SDF padded size
        float bearing_y = (float)(glyph.ttf->size.y - glyph.ttf->bearing.y) * font_size_inv;
        
               
        WriteFloat(stream, bearing_x);
        WriteFloat(stream, bearing_y);
        WriteFloat(stream, 0.0f);  // sdf_offset.x
        WriteFloat(stream, 0.0f);  // sdf_offset.y
    }

    // Write kerning count and kerning data
    WriteU16(stream, static_cast<uint16_t>(ttf->kerning().size()));
    for (const auto& k : ttf->kerning())
    {
        WriteU32(stream, k.left);
        WriteU32(stream, k.right);
        WriteFloat(stream, k.value);
    }

    WriteBytes(stream, (void*)atlasData.data(), atlasData.size());
}
void ImportFont(const fs::path& source_path, Stream* output_stream, Props* config, Props* meta)
{
    // Parse font properties from meta props (with defaults)
    int font_size = meta->GetInt("font", "size", 48);
    std::string characters = meta->GetString("font", "characters", " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
    int sdf_padding = meta->GetInt("sdf", "padding", 8) / 2;
    int padding = meta->GetInt("font", "padding", 1);

    // Load font file
    std::ifstream file(source_path, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open font file");

    // Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read font data
    std::vector<unsigned char> fontData(file_size);
    file.read(reinterpret_cast<char*>(fontData.data()), file_size);
    file.close();

    // Create stream for font loading
    Stream* stream = LoadStream(nullptr, fontData.data(), fontData.size());
    auto ttf = std::shared_ptr<ttf::TrueTypeFont>(ttf::TrueTypeFont::load(stream, font_size, characters));

    // Build the imported glyph list
    std::vector<ImportFontGlyph> glyphs;
    for (size_t i = 0; i < characters.size(); i++)
    {
        auto ttfGlyph = ttf->glyph(characters[i]);
        if (ttfGlyph == nullptr)
            continue;

        ImportFontGlyph iglyph{};
        iglyph.ascii = characters[i];
        iglyph.ttf = ttfGlyph;

        iglyph.size = RoundToNearest(ttfGlyph->size + dvec2(sdf_padding * 2));
        iglyph.scale = dvec2(iglyph.size.x, iglyph.size.y) / ttfGlyph->size;
        iglyph.packedSize = iglyph.size + (padding + sdf_padding) * 2;
        iglyph.bearing = RoundToNearest(ttfGlyph->bearing);
        iglyph.advance.x = RoundToNearest((float)ttfGlyph->advance);
        

        glyphs.push_back(iglyph);
    }

    // Pack the glyphs
    int minHeight = (int)noz::NextPowerOf2((uint32_t)(font_size + 2 + sdf_padding * 2 + padding * 2));
    rect_packer packer(minHeight, minHeight);

    while (packer.empty())
    {
        for(auto& glyph : glyphs)
        {
            if (glyph.ttf->contours.size() == 0)
                continue;

            if (-1 == packer.Insert(glyph.packedSize, rect_packer::method::BestLongSideFit, glyph.packedRect))
            {
                rect_packer::BinSize size = packer.size();
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

    auto imageSize = ivec2(packer.size().w, packer.size().h);
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
            ivec2(
                glyph.packedRect.x + padding,
                glyph.packedRect.y + padding
            ),
            ivec2(
                glyph.packedRect.w - padding * 2,
                glyph.packedRect.h - padding * 2
            ),
            sdf_padding,
            glyph.scale,
            dvec2(
                -glyph.ttf->bearing.x + sdf_padding,
                (glyph.ttf->size.y - glyph.ttf->bearing.y) + sdf_padding
            )
        );
    }

    WriteFontData(output_stream, ttf.get(), image, imageSize, glyphs, font_size);
}

bool DoesFontDependOn(const fs::path& source_path, const fs::path& dependency_path)
{
    return fs::path(source_path.string() + ".meta") == dependency_path;
}

static const char* g_font_extensions[] = {
    ".ttf",
    ".otf",
    nullptr
};

static AssetImporterTraits g_font_importer_traits = {
    .type_name = "Font",
    .type = TYPE_FONT,
    .signature = ASSET_SIGNATURE_FONT,
    .file_extensions = g_font_extensions,
    .import_func = ImportFont,
    .does_depend_on = DoesFontDependOn
};

AssetImporterTraits* GetFontImporterTraits()
{
    return &g_font_importer_traits;
}
