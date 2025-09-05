//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// @STL

#include <filesystem>

LoadedCoreAssets g_core_assets = {};

bool ReadAssetHeader(Stream* stream, AssetHeader* header)
{
    if (!stream || !header) return false;
    
    // Read header fields
    header->signature = ReadU32(stream);
    header->version = ReadU32(stream);
    header->flags = ReadU32(stream);
    
    return !IsEOS(stream);
}

bool WriteAssetHeader(Stream* stream, AssetHeader* header)
{
    if (!stream || !header) return false;
    
    // Write header fields
    WriteU32(stream, header->signature);
    WriteU32(stream, header->version);
    WriteU32(stream, header->flags);
    
    return true;
}

bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature)
{
    if (!header) return false;
    return header->signature == expected_signature;
}

const char* GetExtensionFromSignature(AssetSignature signature)
{
    // Convert signature to 4 character string (little endian to big endian)
    static char ext[6];  // ".xxxx\0"
    ext[0] = '.';
    ext[1] = tolower((signature >> 24) & 0xFF);
    ext[2] = tolower((signature >> 16) & 0xFF);
    ext[3] = tolower((signature >> 8) & 0xFF);
    ext[4] = tolower(signature & 0xFF);
    ext[5] = '\0';
    
    return ext;
}

Stream* LoadAssetStream(Allocator* allocator, const Name* asset_name, AssetSignature signature)
{
    assert(asset_name);

    std::filesystem::path asset_path = "assets";
    asset_path /= asset_name->value;
    asset_path += GetExtensionFromSignature(signature);

    return LoadStream(allocator, asset_path);
}

Asset* LoadAsset(Allocator* allocator, const Name* asset_name, AssetSignature signature, AssetLoaderFunc loader)
{
    if (!asset_name || !loader)
        return nullptr;

    Stream* stream = LoadAssetStream(allocator, asset_name, signature);
    if (!stream)
        return nullptr;
    
    AssetHeader header = {};
    if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, signature))
    {
        Free(stream);
        return nullptr;
    }

    auto asset = loader(allocator, stream, &header, asset_name);
    Free(stream);
    
    return asset;
}

#ifdef NOZ_EDITOR

void ReloadAsset(const Name* name, AssetSignature signature, Asset* asset, void (*reload)(Asset*, Stream*))
{
    assert(name);

    Stream* stream = LoadAssetStream(ALLOCATOR_SCRATCH, name, signature);
    if (!stream)
        return;

    AssetHeader header = {};
    if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, signature))
    {
        Free(stream);
        return;
    }

    reload(asset, stream);

    Free(stream);
}

#endif