//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

// @STL

#include <filesystem>

#ifdef _HOTLOAD
void ReloadStyleSheet(Object* asset, Stream* stream, const AssetHeader* header, const name_t* name);
#endif

LoadedCoreAssets CoreAssets = {};

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

type_t ToType(asset_signature_t signature)
{
    switch (signature)
    {
        case ASSET_SIGNATURE_TEXTURE:  return TYPE_TEXTURE;
        case ASSET_SIGNATURE_MESH:     return TYPE_MESH;
        case ASSET_SIGNATURE_SOUND:    return TYPE_SOUND;
        case ASSET_SIGNATURE_SHADER:   return TYPE_SHADER;
        case ASSET_SIGNATURE_MATERIAL: return TYPE_MATERIAL;
        case ASSET_SIGNATURE_FONT:     return TYPE_FONT;
        case ASSET_SIGNATURE_STYLE_SHEET:     return TYPE_STYLE_SHEET;
        case ASSET_SIGNATURE_VFX: return TYPE_VFX;
        default:                       return TYPE_UNKNOWN;
    }
}

static asset_signature_t AssetSignatureFromType(type_t type)
{
    switch (type)
    {
    case TYPE_TEXTURE:  return ASSET_SIGNATURE_TEXTURE;
    case TYPE_MESH:     return ASSET_SIGNATURE_MESH;
    case TYPE_SOUND:    return ASSET_SIGNATURE_SOUND;
    case TYPE_SHADER:   return ASSET_SIGNATURE_SHADER;
    case TYPE_MATERIAL: return ASSET_SIGNATURE_MATERIAL;
    case TYPE_FONT:     return ASSET_SIGNATURE_FONT;
    case TYPE_VFX: return ASSET_SIGNATURE_VFX;
    case TYPE_STYLE_SHEET:     return ASSET_SIGNATURE_STYLE_SHEET;
    default:
        return ASSET_SIGNATURE_UNKNOWN;
    }
}


const char* GetExtensionFromSignature(asset_signature_t signature)
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

Stream* LoadAssetStream(Allocator* allocator, const name_t* asset_name, asset_signature_t signature)
{
    assert(asset_name);

    const char* base_path = SDL_GetBasePath();
    std::filesystem::path asset_path;
    
    if (!base_path)
    {
        asset_path = "assets";
    }
    else
    {
        asset_path = base_path;
        asset_path /= "assets";
    }
    
    asset_path /= asset_name->value;
    asset_path += GetExtensionFromSignature(signature);

    return LoadStream(allocator, asset_path);
}

Object* LoadAsset(Allocator* allocator, const name_t* asset_name, asset_signature_t signature, AssetLoaderFunc loader)
{
    if (!asset_name || !loader)
        return nullptr;

    Stream* stream = LoadAssetStream(allocator, asset_name, signature);
    if (!stream)
        return nullptr;
    
    AssetHeader header = {};
    if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, signature))
    {
        Destroy(stream);
        return nullptr;
    }

    auto asset = loader(allocator, stream, &header, asset_name);
    Destroy(stream);
    
    return asset;
}


#ifdef _HOTLOAD
void ReloadAsset(const name_t* name, Object* asset)
{
    assert(name);
    assert(asset);
    auto type = GetType(asset);
    auto signature = AssetSignatureFromType(type);

    if (signature == ASSET_SIGNATURE_UNKNOWN)
        return;

    // only certain types support hotload right now
    if (type != TYPE_STYLE_SHEET)
        return;

    auto stream = LoadAssetStream(GetAllocator(asset), name, signature);
    if (!stream)
        return;

    AssetHeader header = {};
    if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, signature))
    {
        Destroy(stream);
        return;
    }

    switch (type)
    {
    case TYPE_STYLE_SHEET:
        ReloadStyleSheet(asset, stream, &header, name);
        break;

    default:
        break;
    }
}
#endif