//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// @STL

#include <filesystem>

bool IsValidAssetType(AssetType asset_type) {
    return ToString(asset_type) != nullptr;
}

bool ReadAssetHeader(Stream* stream, AssetHeader* header) {
    if (!stream || !header) return false;
    
    ReadBytes(stream, header, sizeof(AssetHeader));

    return !IsEOS(stream);
}

bool WriteAssetHeader(Stream* stream, AssetHeader* header, const Name** name_table) {
    if (!stream || !header) return false;
    
    // Write header fields
    WriteBytes(stream, header, sizeof(AssetHeader));

    if (header->names > 0) {
        assert(name_table);
        for (u32 i = 0; i < header->names; i++)
            WriteString(stream, name_table[i]->value);
    }

    return true;
}

bool ValidateAssetHeader(AssetHeader* header, AssetType expected_asset_type) {
    if (!header) return false;
    if (header->signature != ASSET_SIGNATURE) return false;
    if (header->type != expected_asset_type) return false;
    return true;
}

const char* ToString(AssetType asset_type) {
    switch (asset_type) {
        case ASSET_TYPE_TEXTURE: return "Texture";
        case ASSET_TYPE_MESH: return "Mesh";
        case ASSET_TYPE_FONT: return "Font";
        case ASSET_TYPE_SOUND: return "Sound";
        case ASSET_TYPE_SKELETON: return "Skeleton";
        case ASSET_TYPE_ANIMATION: return "Animation";
        case ASSET_TYPE_VFX: return "Vfx";
        case ASSET_TYPE_SHADER: return "Shader";
        default: return nullptr;
    }
}

static Stream* LoadAssetStream(Allocator* allocator, const Name* asset_name, AssetType asset_type) {
    assert(asset_name);

    std::filesystem::path asset_path = GetApplicationTraits()->assets_path;
    asset_path /= ToString(asset_type);
    asset_path /= asset_name->value;

    std::string lowercase_path = asset_path.string();
    Lowercase(lowercase_path.data(), (u32)lowercase_path.size());

    if (Stream* stream = LoadStream(allocator, lowercase_path))
        return stream;

    asset_path = GetBinaryDirectory();
    asset_path /= "assets";
    asset_path /= ToString(asset_type);
    asset_path /= asset_name->value;

    return LoadStream(allocator, asset_path);
}

const Name** ReadNameTable(const AssetHeader& header, Stream* stream) {
    const Name** name_table = nullptr;
    if (header.names > 0)
    {
        name_table = (const Name**)Alloc(ALLOCATOR_SCRATCH, sizeof(Name*) * header.names);
        for (u32 i = 0; i < header.names; i++)
            name_table[i] = ReadName(stream);
    }

    return name_table;
}

Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader, Stream* stream) {
    AssetHeader header = {};
    if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, asset_type)) {
        Free(stream);
        return nullptr;
    }

    const Name** name_table = ReadNameTable(header, stream);

    Asset* asset = loader(allocator, stream, &header, asset_name, name_table);
    asset->flags = header.flags;

    Free(name_table);

    return asset;
}

Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader, const u8* data, u32 data_size) {
    Stream* stream = data != nullptr
        ? LoadStream(ALLOCATOR_SCRATCH, data, data_size)
        : LoadAssetStream(ALLOCATOR_SCRATCH, asset_name, asset_type);

    if (!stream)
        return nullptr;

    Asset* asset = LoadAssetInternal(allocator, asset_name, asset_type, loader, stream);

    Free(stream);

    return asset;
}

Asset* LoadAsset(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader, const u8* data, u32 data_size) {
    if (!asset_name || !loader)
        return nullptr;

    PushScratch();
    Asset* asset = LoadAssetInternal(allocator, asset_name, asset_type, loader, data, data_size);
    PopScratch();

    return asset;
}

#ifdef NOZ_EDITOR

void ReloadAsset(const Name* name, AssetType asset_type, Asset* asset, void (*reload)(Asset*, Stream*, const AssetHeader& header, const Name** name_table)) {
    if (asset == nullptr)
        return;

    assert(name);

    Stream* stream = LoadAssetStream(ALLOCATOR_SCRATCH, name, asset_type);
    if (!stream)
        return;

    AssetHeader header = {};
    if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, asset_type)) {
        Free(stream);
        return;
    }

    const Name** name_table = ReadNameTable(header, stream);

    reload(asset, stream, header, name_table);

    Free(stream);
}

#endif