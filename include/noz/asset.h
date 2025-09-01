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
} ;

typedef Object* (*AssetLoaderFunc)(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name);

bool ReadAssetHeader(Stream* stream, AssetHeader* header);
bool WriteAssetHeader(Stream* stream, AssetHeader* header);
bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature);
type_t ToType(asset_signature_t signature);
const char* GetExtensionFromSignature(asset_signature_t signature);
Object* LoadAsset(Allocator* allocator, const name_t* asset_name, asset_signature_t signature, AssetLoaderFunc loader);


// @loaders
Object* LoadTexture(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name);
Object* LoadShader(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name);
Object* LoadFont(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name);
Object* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name);
Object* LoadStyleSheet(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name);
Object* LoadVfx(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name);

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
void ReloadAsset(const name_t* name, Object* asset);

#define NOZ_HOTLOAD_ASSET(name_var, asset_member) \
    if (incoming_name == name_var) \
    { \
        ReloadAsset(name_var, asset_member); \
        return; \
    }
#endif // _HOTLOAD