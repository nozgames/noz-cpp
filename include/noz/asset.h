//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#define MAKE_FOURCC(a, b, c, d) \
    ((u32)(d) | ((u32)(c) << 8) | ((u32)(b) << 16) | ((u32)(a) << 24))

typedef u32 asset_signature_t;

constexpr asset_signature_t ASSET_SIGNATURE_TEXTURE     = MAKE_FOURCC('N', 'Z', 'T', 'X');
constexpr asset_signature_t ASSET_SIGNATURE_MESH        = MAKE_FOURCC('N', 'Z', 'M', 'S');
constexpr asset_signature_t ASSET_SIGNATURE_SOUND       = MAKE_FOURCC('N', 'Z', 'S', 'N');
constexpr asset_signature_t ASSET_SIGNATURE_SHADER      = MAKE_FOURCC('N', 'Z', 'S', 'H');
constexpr asset_signature_t ASSET_SIGNATURE_MATERIAL    = MAKE_FOURCC('N', 'Z', 'M', 'T');
constexpr asset_signature_t ASSET_SIGNATURE_FONT        = MAKE_FOURCC('N', 'Z', 'F', 'T');
constexpr asset_signature_t ASSET_SIGNATURE_STYLE_SHEET = MAKE_FOURCC('N', 'Z', 'S', 'T');
constexpr asset_signature_t ASSET_SIGNATURE_VFX         = MAKE_FOURCC('N', 'Z', 'F', 'X');
constexpr asset_signature_t ASSET_SIGNATURE_UNKNOWN     = 0xF00DF00D;

struct AssetHeader
{
    asset_signature_t signature;
    uint32_t version;
    uint32_t flags;
};

struct Asset
{
    const Name* name;
};

typedef Asset* (*AssetLoaderFunc)(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name);

bool ReadAssetHeader(Stream* stream, AssetHeader* header);
bool WriteAssetHeader(Stream* stream, AssetHeader* header);
bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature);
const char* GetExtensionFromSignature(asset_signature_t signature);
Asset* LoadAsset(Allocator* allocator, const Name* asset_name, asset_signature_t signature, AssetLoaderFunc loader);

inline const Name* GetName(Asset* asset) { return asset->name; }


// @loaders
Asset* LoadTexture(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name);
Asset* LoadShader(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name);
Asset* LoadFont(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name);
Asset* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name);
Asset* LoadStyleSheet(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name);
Asset* LoadVfx(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name);

// @macros
#define NOZ_LOAD_SHADER(allocator, path, member) \
    member = (Shader*)LoadAsset(allocator, path, ASSET_SIGNATURE_SHADER, LoadShader);

#define NOZ_LOAD_TEXTURE(allocator, path, member) \
    member = (Texture*)LoadAsset(allocator, path, ASSET_SIGNATURE_TEXTURE, LoadTexture);

#define NOZ_LOAD_STYLE_SHEET(allocator, path, member) \
    member = (StyleSheet*)LoadAsset(allocator, path, ASSET_SIGNATURE_STYLE_SHEET, LoadStyleSheet);

#define NOZ_LOAD_MESH(allocator, path, member) \
    member = (Mesh*)LoadAsset(allocator, path, ASSET_SIGNATURE_MESH, LoadMesh);

#define NOZ_LOAD_FONT(allocator, path, member) \
    member = (Font*)LoadAsset(allocator, path, ASSET_SIGNATURE_FONT, LoadFont);

#define NOZ_LOAD_VFX(allocator, path, member) \
    member = (Vfx*)LoadAsset(allocator, path, ASSET_SIGNATURE_VFX, LoadVfx);

// @hotload
#ifdef _HOTLOAD
void ReloadAsset(const Name* name, Asset* asset);

#define NOZ_HOTLOAD_ASSET(name_var, asset_member) \
    if (incoming_name == name_var) \
    { \
        ReloadAsset(name_var, asset_member); \
        return; \
    }
#endif // _HOTLOAD