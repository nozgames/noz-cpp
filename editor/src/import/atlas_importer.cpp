//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace fs = std::filesystem;

static void ImportAtlas(AssetData* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;

    AtlasData* atlas = static_cast<AtlasData*>(a);
    AtlasDataImpl* impl = atlas->impl;

    // Ensure atlas is up to date
    RegenerateAtlas(atlas);

    if (!impl->pixels) {
        // No pixel data - nothing to export
        return;
    }

    // Export as texture
    std::string filter = meta->GetString("atlas", "filter", "linear");
    std::string clamp = meta->GetString("atlas", "clamp", "clamp");

    TextureFilter filter_value = filter == "nearest" || filter == "point"
        ? TEXTURE_FILTER_NEAREST
        : TEXTURE_FILTER_LINEAR;

    TextureClamp clamp_value = clamp == "repeat" ?
        TEXTURE_CLAMP_REPEAT :
        TEXTURE_CLAMP_CLAMP;

    Stream* stream = CreateStream(ALLOCATOR_DEFAULT, impl->width * impl->height * 4 + 1024);

    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE;
    header.type = ASSET_TYPE_TEXTURE;  // Export as texture for runtime
    header.version = 1;
    header.flags = ASSET_FLAG_NONE;
    WriteAssetHeader(stream, &header);

    TextureFormat format = TEXTURE_FORMAT_RGBA8;
    WriteU8(stream, (u8)format);
    WriteU8(stream, (u8)filter_value);
    WriteU8(stream, (u8)clamp_value);
    WriteU32(stream, impl->width);
    WriteU32(stream, impl->height);
    WriteBytes(stream, impl->pixels, impl->width * impl->height * 4);

    SaveStream(stream, path);
    Free(stream);
}

static bool AtlasDependsOn(AssetData* atlas_asset, AssetData* dependency) {
    if (dependency->type != ASSET_TYPE_MESH) return false;

    AtlasData* atlas = static_cast<AtlasData*>(atlas_asset);
    AtlasDataImpl* impl = atlas->impl;

    // Check if this mesh is attached to the atlas
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
