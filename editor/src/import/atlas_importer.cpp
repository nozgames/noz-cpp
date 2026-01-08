//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace fs = std::filesystem;

static void ImportAtlas(AssetData* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;

    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    // Export as texture - filter from config [atlas] section, clamp always on
    TextureFilter filter_value = g_editor.atlas.filter;
    TextureClamp clamp_value = TEXTURE_CLAMP_CLAMP;

    // Regenerate atlas to impl->pixels (ensures it's up to date)
    RegenerateAtlas(atlas);

    if (!impl->pixels) {
        return;
    }

    u32 pixel_size = impl->width * impl->height * 4;

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
    WriteBytes(stream, impl->pixels, pixel_size);

    SaveStream(stream, path);
    Free(stream);
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
