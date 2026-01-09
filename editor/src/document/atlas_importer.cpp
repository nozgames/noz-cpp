//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace fs = std::filesystem;

static void ImportAtlas(Document* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;

    AtlasDocument* atlas = static_cast<AtlasDocument*>(a);
    AtlasDataImpl* impl = atlas->impl;

    // Export as texture - filter from config [atlas] section, clamp always on
    TextureFilter filter_value = g_editor.atlas.filter;
    TextureClamp clamp_value = TEXTURE_CLAMP_CLAMP;

    // Regenerate atlas to impl->pixels (ensures it's up to date)
    RegenerateAtlas(atlas);

    if (!impl->pixels) {
        return;
    }

    u32 pixel_size = impl->size.x * impl->size.y * 4;

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
    WriteU32(stream, impl->size.x);
    WriteU32(stream, impl->size.y);
    WriteBytes(stream, impl->pixels, pixel_size);

    SaveStream(stream, path);
    Free(stream);
}

static bool AtlasDependsOn(Document* atlas_asset, Document* dependency) {
    if (dependency->type != ASSET_TYPE_MESH) {
        return false;
    }

    AtlasDocument* atlas = static_cast<AtlasDocument*>(atlas_asset);
    AtlasDataImpl* impl = atlas->impl;

    // Check if this mesh/animated mesh is attached to the atlas
    for (int i = 0; i < impl->rect_count; i++) {
        if (impl->rects[i].valid && impl->rects[i].mesh_name == dependency->name) {
            return true;
        }
    }

    return false;
}

DocumentImporter GetAtlasImporter() {
    return {
        .type = ASSET_TYPE_ATLAS,
        .ext = ".atlas",
        .import_func = ImportAtlas,
        .does_depend_on = AtlasDependsOn
    };
}
