//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// @STL

#include <filesystem>

bool IsValidSignature(AssetSignature signature) {
    return signature == ASSET_SIGNATURE_TEXTURE ||
           signature == ASSET_SIGNATURE_MESH ||
           signature == ASSET_SIGNATURE_FONT ||
           signature == ASSET_SIGNATURE_SOUND ||
           signature == ASSET_SIGNATURE_SKELETON ||
           signature == ASSET_SIGNATURE_ANIMATION ||
           signature == ASSET_SIGNATURE_VFX ||
           signature == ASSET_SIGNATURE_STYLE_SHEET;
}

bool ReadAssetHeader(Stream* stream, AssetHeader* header) {
    if (!stream || !header) return false;
    
    // Read header fields
    header->signature = ReadU32(stream);
    header->version = ReadU32(stream);
    header->flags = ReadU32(stream);
    header->names = ReadU32(stream);
    
    return !IsEOS(stream);
}

bool WriteAssetHeader(Stream* stream, AssetHeader* header, const Name** name_table) {
    if (!stream || !header) return false;
    
    // Write header fields
    WriteU32(stream, header->signature);
    WriteU32(stream, header->version);
    WriteU32(stream, header->flags);
    WriteU32(stream, header->names);

    if (header->names > 0) {
        assert(name_table);
        for (u32 i = 0; i < header->names; i++)
            WriteString(stream, name_table[i]->value);
    }

    return true;
}

bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature) {
    if (!header) return false;
    return header->signature == expected_signature;
}

const char* GetExtensionFromSignature(AssetSignature signature) {
    // Convert signature to 4 character string (little endian to big endian)
    static char ext[6];  // ".xxxx\0"
    ext[0] = '.';
    ext[1] = (char)tolower((signature >> 24) & 0xFF);
    ext[2] = (char)tolower((signature >> 16) & 0xFF);
    ext[3] = (char)tolower((signature >> 8) & 0xFF);
    ext[4] = (char)tolower(signature & 0xFF);
    ext[5] = '\0';
    
    return ext;
}

const char* GetTypeNameFromSignature(AssetSignature signature) {
    switch (signature) {
        case ASSET_SIGNATURE_TEXTURE: return "Texture";
        case ASSET_SIGNATURE_MESH: return "Mesh";
        case ASSET_SIGNATURE_FONT: return "Font";
        case ASSET_SIGNATURE_SOUND: return "Sound";
        case ASSET_SIGNATURE_SKELETON: return "Skeleton";
        case ASSET_SIGNATURE_ANIMATION: return "Animation";
        case ASSET_SIGNATURE_VFX: return "Vfx";
        case ASSET_SIGNATURE_SHADER: return "Shader";
        case ASSET_SIGNATURE_STYLE_SHEET: return "StyleSheet";
        default: return nullptr;
    }
}

AssetSignature GetSignatureFromExtension(const char* ext) {
    if (*ext == '.')
        ext++;

    if (Length(ext) != 4)
        return ASSET_SIGNATURE_UNKNOWN;

    // convert extension to signature (big endian to little endian)
    AssetSignature signature = 0;
    signature |= ((AssetSignature)toupper(ext[0]) << 24);
    signature |= ((AssetSignature)toupper(ext[1]) << 16);
    signature |= ((AssetSignature)toupper(ext[2]) << 8);
    signature |= ((AssetSignature)toupper(ext[3]) << 0);

    if (!IsValidSignature(signature))
        return ASSET_SIGNATURE_UNKNOWN;

    return signature;
}

static Stream* LoadAssetStream(Allocator* allocator, const Name* asset_name, AssetSignature signature) {
    assert(asset_name);

    std::filesystem::path asset_path = GetApplicationTraits()->assets_path;
    asset_path /= GetTypeNameFromSignature(signature);
    asset_path /= asset_name->value;

    std::string lowercase_path = asset_path.string();
    Lowercase(lowercase_path.data(), (u32)lowercase_path.size());

    if (Stream* stream = LoadStream(allocator, lowercase_path))
        return stream;

    asset_path = GetBinaryDirectory();
    asset_path /= "assets";
    asset_path /= GetTypeNameFromSignature(signature);
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

Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetSignature signature, AssetLoaderFunc loader, Stream* stream) {
    AssetHeader header = {};
    if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, signature)) {
        Free(stream);
        return nullptr;
    }

    const Name** name_table = ReadNameTable(header, stream);

    Asset* asset = loader(allocator, stream, &header, asset_name, name_table);
    asset->flags = header.flags;

    Free(name_table);

    return asset;
}

Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetSignature signature, AssetLoaderFunc loader) {
    Stream* stream = LoadAssetStream(ALLOCATOR_SCRATCH, asset_name, signature);
    if (!stream)
        return nullptr;

    Asset* asset = LoadAssetInternal(allocator, asset_name, signature, loader, stream);

    Free(stream);

    return asset;
}

Asset* LoadAsset(Allocator* allocator, const Name* asset_name, AssetSignature signature, AssetLoaderFunc loader) {
    if (!asset_name || !loader)
        return nullptr;

    PushScratch();
    Asset* asset = LoadAssetInternal(allocator, asset_name, signature, loader);
    PopScratch();

    return asset;
}

#ifdef NOZ_EDITOR

void ReloadAsset(const Name* name, AssetSignature signature, Asset* asset, void (*reload)(Asset*, Stream*, const AssetHeader& header, const Name** name_table)) {
    if (asset == nullptr)
        return;

    assert(name);

    Stream* stream = LoadAssetStream(ALLOCATOR_SCRATCH, name, signature);
    if (!stream)
        return;

    AssetHeader header = {};
    if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, signature)) {
        Free(stream);
        return;
    }

    const Name** name_table = ReadNameTable(header, stream);

    reload(asset, stream, header, name_table);

    Free(stream);
}

#endif