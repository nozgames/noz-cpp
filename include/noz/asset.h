//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr u32 ASSET_SIGNATURE = FourCC('N', 'O', 'Z', 'A');

enum AssetType {
    ASSET_TYPE_UNKNOWN = -1,
    ASSET_TYPE_MESH,
    ASSET_TYPE_VFX,
    ASSET_TYPE_SKELETON,
    ASSET_TYPE_ANIMATION,
    ASSET_TYPE_SOUND,
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_FONT,
    ASSET_TYPE_SHADER,
    ASSET_TYPE_COUNT,
};

typedef u32 AssetFlags;
constexpr AssetFlags ASSET_FLAG_NONE        = 0;

struct AssetHeader {
    u32 signature;
    AssetType type;
    u32 version;
    u32 flags;
    u32 names;
};

struct Asset {
    const Name* name;
    u32 flags;
};

typedef Asset* (*AssetLoaderFunc)(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);

extern bool ReadAssetHeader(Stream* stream, AssetHeader* header);
extern bool WriteAssetHeader(Stream* stream, AssetHeader* header, const Name** name_table = nullptr);
extern bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature);
extern const char* GetExtensionFromAssetType(AssetType asset_type);
extern const char* ToString(AssetType asset_type);
extern Asset* LoadAsset(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader);
extern const Name** ReadNameTable(const AssetHeader& header, Stream* stream);
extern bool IsValidAssetType(AssetType asset_type);
extern Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader, Stream* stream);
extern Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader);
inline const Name* GetName(Asset* asset) { return asset->name; }

// @loaders
Asset* LoadTexture(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadShader(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadFont(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadVfx(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadSound(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadSkeleton(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadAnimation(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);

// @macros
#define NOZ_LOAD_SHADER(allocator, path, member) \
    member = (Shader*)LoadAsset(allocator, path, ASSET_TYPE_SHADER, LoadShader);

#define NOZ_LOAD_TEXTURE(allocator, path, member) \
    member = (Texture*)LoadAsset(allocator, path, ASSET_TYPE_TEXTURE, LoadTexture);

#define NOZ_LOAD_MESH(allocator, path, member) \
    member = (Mesh*)LoadAsset(allocator, path, ASSET_TYPE_MESH, LoadMesh);

#define NOZ_LOAD_FONT(allocator, path, member) \
    member = (Font*)LoadAsset(allocator, path, ASSET_TYPE_FONT, LoadFont);

#define NOZ_LOAD_VFX(allocator, path, member) \
    member = (Vfx*)LoadAsset(allocator, path, ASSET_TYPE_VFX, LoadVfx);

#define NOZ_LOAD_SOUND(allocator, path, member) \
    member = (Sound*)LoadAsset(allocator, path, ASSET_TYPE_SOUND, LoadSound);

#define NOZ_LOAD_SKELETON(allocator, path, member) \
    member = (Skeleton*)LoadAsset(allocator, path, ASSET_TYPE_SKELETON, LoadSkeleton);

#define NOZ_LOAD_ANIMATION(allocator, path, member) \
    member = (Animation*)LoadAsset(allocator, path, ASSET_TYPE_ANIMATION, LoadAnimation);


#define NOZ_RELOAD_FONT(asset_name, asset)
#define NOZ_RELOAD_SOUND(asset_name, asset)
#define NOZ_RELOAD_SKELETON(asset_name, asset)
#define NOZ_RELOAD_ANIMATION(asset_name, asset)

#ifdef NOZ_EDITOR
void ReloadAsset(const Name* name, AssetType asset_type, Asset* asset, void (*reload)(Asset*, Stream*, const AssetHeader& header, const Name** name_table));
void ReloadVfx(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table);
void ReloadMesh(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table);
void ReloadShader(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table);
void ReloadTexture(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table);

#define NOZ_RELOAD_VFX(asset_name, asset) \
    if(asset_name == incoming_name && incoming_type == ASSET_TYPE_VFX) { \
        ReloadAsset(asset_name, ASSET_TYPE_VFX, asset, ReloadVfx); return; }

#define NOZ_RELOAD_MESH(asset_name, asset) \
    if(asset_name == incoming_name && incoming_type == ASSET_TYPE_MESH) { \
        ReloadAsset(asset_name, ASSET_TYPE_MESH, asset, ReloadMesh); return; }

#define NOZ_RELOAD_SHADER(asset_name, asset) \
    if(asset_name == incoming_name && incoming_type == ASSET_TYPE_SHADER) { \
        ReloadAsset(asset_name, ASSET_TYPE_SHADER, asset, ReloadShader); return; }

#define NOZ_RELOAD_TEXTURE(asset_name, asset) \
    if(asset_name == incoming_name && incoming_type == ASSET_TYPE_TEXTURE) { \
        ReloadAsset(asset_name, ASSET_TYPE_TEXTURE, asset, ReloadTexture); return; }

#else
#define NOZ_RELOAD_VFX(asset_name, asset)
#define NOZ_RELOAD_MESH(asset_name, asset)
#define NOZ_RELOAD_SHADER(asset_name, asset)
#define NOZ_RELOAD_TEXTURE(asset_name, asset)
#endif
