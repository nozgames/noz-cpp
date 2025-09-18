//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#define MAKE_FOURCC(a, b, c, d) \
    ((u32)(d) | ((u32)(c) << 8) | ((u32)(b) << 16) | ((u32)(a) << 24))

typedef u32 AssetSignature;

constexpr AssetSignature ASSET_SIGNATURE_TEXTURE     = MAKE_FOURCC('N', 'Z', 'T', 'X');
constexpr AssetSignature ASSET_SIGNATURE_MESH        = MAKE_FOURCC('N', 'Z', 'M', 'S');
constexpr AssetSignature ASSET_SIGNATURE_SOUND       = MAKE_FOURCC('N', 'Z', 'S', 'N');
constexpr AssetSignature ASSET_SIGNATURE_SHADER      = MAKE_FOURCC('N', 'Z', 'S', 'H');
constexpr AssetSignature ASSET_SIGNATURE_MATERIAL    = MAKE_FOURCC('N', 'Z', 'M', 'T');
constexpr AssetSignature ASSET_SIGNATURE_FONT        = MAKE_FOURCC('N', 'Z', 'F', 'T');
constexpr AssetSignature ASSET_SIGNATURE_STYLE_SHEET = MAKE_FOURCC('N', 'Z', 'S', 'T');
constexpr AssetSignature ASSET_SIGNATURE_VFX         = MAKE_FOURCC('N', 'Z', 'F', 'X');
constexpr AssetSignature ASSET_SIGNATURE_SKELETON    = MAKE_FOURCC('N', 'Z', 'S', 'K');
constexpr AssetSignature ASSET_SIGNATURE_ANIMATION   = MAKE_FOURCC('N', 'Z', 'A', 'N');
constexpr AssetSignature ASSET_SIGNATURE_UNKNOWN     = 0xF00DF00D;

struct AssetHeader
{
    AssetSignature signature;
    u32 version;
    u32 flags;
    u32 names;
};

struct Asset
{
    const Name* name;
};

typedef Asset* (*AssetLoaderFunc)(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);

extern bool ReadAssetHeader(Stream* stream, AssetHeader* header);
extern bool WriteAssetHeader(Stream* stream, AssetHeader* header, const Name** name_table = nullptr);
extern bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature);
extern AssetSignature GetSignatureFromExtension(const char* ext);
extern const char* GetExtensionFromSignature(AssetSignature signature);
extern const char* GetTypeNameFromSignature(AssetSignature signature);
extern Asset* LoadAsset(Allocator* allocator, const Name* asset_name, AssetSignature signature, AssetLoaderFunc loader);
extern const Name** ReadNameTable(const AssetHeader& header, Stream* stream);
inline const Name* GetName(Asset* asset) { return asset->name; }
extern bool IsValidSignature(AssetSignature signature);

// @loaders
Asset* LoadTexture(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadShader(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadFont(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadStyleSheet(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadVfx(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadSound(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadSkeleton(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadAnimation(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);

// @macros
#define NOZ_LOAD_SHADER(allocator, path, member) \
    member = (Shader*)LoadAsset(allocator, path, ASSET_SIGNATURE_SHADER, LoadShader);

#define NOZ_LOAD_TEXTURE(allocator, path, member) \
    member = (Texture*)LoadAsset(allocator, path, ASSET_SIGNATURE_TEXTURE, LoadTexture);

#define NOZ_LOAD_STYLESHEET(allocator, path, member) \
    member = (StyleSheet*)LoadAsset(allocator, path, ASSET_SIGNATURE_STYLE_SHEET, LoadStyleSheet);

#define NOZ_LOAD_MESH(allocator, path, member) \
    member = (Mesh*)LoadAsset(allocator, path, ASSET_SIGNATURE_MESH, LoadMesh);

#define NOZ_LOAD_FONT(allocator, path, member) \
    member = (Font*)LoadAsset(allocator, path, ASSET_SIGNATURE_FONT, LoadFont);

#define NOZ_LOAD_VFX(allocator, path, member) \
    member = (Vfx*)LoadAsset(allocator, path, ASSET_SIGNATURE_VFX, LoadVfx);

#define NOZ_LOAD_SOUND(allocator, path, member) \
    member = (Sound*)LoadAsset(allocator, path, ASSET_SIGNATURE_SOUND, LoadSound);

#define NOZ_LOAD_SKELETON(allocator, path, member) \
    member = (Skeleton*)LoadAsset(allocator, path, ASSET_SIGNATURE_SKELETON, LoadSkeleton);

#define NOZ_LOAD_ANIMATION(allocator, path, member) \
    member = (Animation*)LoadAsset(allocator, path, ASSET_SIGNATURE_ANIMATION, LoadAnimation);


#define NOZ_RELOAD_TEXTURE(asset_name, asset)
#define NOZ_RELOAD_FONT(asset_name, asset)
#define NOZ_RELOAD_SOUND(asset_name, asset)
#define NOZ_RELOAD_SKELETON(asset_name, asset)
#define NOZ_RELOAD_ANIMATION(asset_name, asset)

#ifdef NOZ_EDITOR
void ReloadAsset(const Name* name, AssetSignature signature, Asset* asset, void (*reload)(Asset*, Stream*, const AssetHeader& header, const Name** name_table));
void ReloadStyleSheet(Asset* sheet, Stream* stream, const AssetHeader& header, const Name** name_table);
void ReloadVfx(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table);
void ReloadMesh(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table);
void ReloadShader(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table);

#define NOZ_RELOAD_STYLESHEET(asset_name, asset) \
    if(asset_name == incoming_name) { ReloadAsset(asset_name, ASSET_SIGNATURE_STYLE_SHEET, asset, ReloadStyleSheet); return; }

#define NOZ_RELOAD_VFX(asset_name, asset) \
    if(asset_name == incoming_name) { ReloadAsset(asset_name, ASSET_SIGNATURE_VFX, asset, ReloadVfx); return; }

#define NOZ_RELOAD_MESH(asset_name, asset) \
    if(asset_name == incoming_name) { ReloadAsset(asset_name, ASSET_SIGNATURE_MESH, asset, ReloadMesh); return; }

#define NOZ_RELOAD_SHADER(asset_name, asset) \
    if(asset_name == incoming_name) { ReloadAsset(asset_name, ASSET_SIGNATURE_SHADER, asset, ReloadShader); return; }

#else
#define NOZ_RELOAD_STYLESHEET(asset)
#define NOZ_RELOAD_VFX(asset_name, asset)
#define NOZ_RELOAD_MESH(asset_name, asset)
#define NOZ_RELOAD_SHADER(asset_name, asset)
#endif
