//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <plutovg.h>

extern void ConvertARGBToRGBA(u8* dst, const u8* src, int width, int height);

namespace fs = std::filesystem;

static void ImportAtlas(AssetData* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;

    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    // Export as texture
    std::string filter = meta->GetString("atlas", "filter", "linear");
    std::string clamp = meta->GetString("atlas", "clamp", "clamp");

    TextureFilter filter_value = filter == "nearest" || filter == "point"
        ? TEXTURE_FILTER_NEAREST
        : TEXTURE_FILTER_LINEAR;

    TextureClamp clamp_value = clamp == "repeat" ?
        TEXTURE_CLAMP_REPEAT :
        TEXTURE_CLAMP_CLAMP;

    // Regenerate atlas to impl->pixels (ensures it's up to date)
    RegenerateAtlas(atlas);

    if (!impl->pixels) {
        return;
    }

    // Copy to temporary buffer for export processing
    u32 pixel_size = impl->width * impl->height * 4;
    u8* pixels = static_cast<u8*>(Alloc(ALLOCATOR_DEFAULT, pixel_size));
    memcpy(pixels, impl->pixels, pixel_size);

    // Convert from PlutoVG's premultiplied ARGB to premultiplied RGBA
    ConvertARGBToRGBA(pixels, pixels, impl->width, impl->height);

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, pixel_size + 1024);

    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE;
    header.type = ASSET_TYPE_ATLAS;
    header.version = 1;
    header.flags = ASSET_FLAG_NONE;
    WriteAssetHeader(stream, &header);

    TextureFormat format = TEXTURE_FORMAT_RGBA8;
    WriteU8(stream, (u8)format);
    WriteU8(stream, (u8)filter_value);
    WriteU8(stream, (u8)clamp_value);
    WriteU32(stream, impl->width);
    WriteU32(stream, impl->height);
    WriteBytes(stream, pixels, pixel_size);

    SaveStream(stream, path);
    Free(stream);
    Free(pixels);
}

static bool AtlasDependsOn(AssetData* atlas_asset, AssetData* dependency) {
    if (dependency->type != ASSET_TYPE_MESH) {
        return false;
    }

    AtlasData* atlas = static_cast<AtlasData*>(atlas_asset);
    AtlasDataImpl* impl = atlas->impl;

    // Check if this mesh/animated mesh is attached to the atlas
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name == dependency->name) {
            return true;
        }
    }

    return false;
}

AssetImporter GetAtlasImporter() {
    return {
        .type = ASSET_TYPE_ATLAS,
        .ext = ".atlas",
        .import_func = ImportAtlas,
        .does_depend_on = AtlasDependsOn
    };
}
