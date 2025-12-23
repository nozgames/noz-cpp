//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#if defined(NOZ_PLATFORM_WEB) || defined(NDEBUG)
#define NOZ_BUILTIN_ASSETS
#endif

constexpr u32 ASSET_SIGNATURE = FourCC('N', 'O', 'Z', 'A');

enum AssetType {
    ASSET_TYPE_CUSTOM = -2,
    ASSET_TYPE_UNKNOWN = -1,
    ASSET_TYPE_MESH,
    ASSET_TYPE_VFX,
    ASSET_TYPE_SKELETON,
    ASSET_TYPE_ANIMATION,
    ASSET_TYPE_SOUND,
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_FONT,
    ASSET_TYPE_SHADER,
    ASSET_TYPE_ANIMATED_MESH,
    ASSET_TYPE_EVENT,
    ASSET_TYPE_BIN,
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
    AssetType type;
    int custom_type_id;
};

typedef Asset* (*AssetLoaderFunc)(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);

extern bool ReadAssetHeader(Stream* stream, AssetHeader* header);
extern bool WriteAssetHeader(Stream* stream, AssetHeader* header, const Name** name_table = nullptr);
extern bool ValidateAssetHeader(AssetHeader* header, uint32_t expected_signature);
extern const char* GetExtensionFromAssetType(AssetType asset_type);
extern const char* ToString(AssetType asset_type);
extern Asset* LoadAsset(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader, const u8* data=nullptr, u32 data_size=0);
extern const Name** ReadNameTable(const AssetHeader& header, Stream* stream);
extern bool IsValidAssetType(AssetType asset_type);
extern Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader, Stream* stream);
extern Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader, const u8* data=nullptr, u32 data_size=0);
inline const Name* GetName(Asset* asset) { return asset->name; }
inline AssetType GetType(Asset* asset) { return asset->type; }
inline bool IsCustomType(Asset* asset) { return asset->type == ASSET_TYPE_CUSTOM; }
inline int GetCustomTypeId(Asset* asset) { return asset->custom_type_id; }

// @loaders
Asset* LoadTexture(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadShader(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadFont(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadVfx(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadSound(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadSkeleton(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadAnimation(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadAnimatedMesh(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);
Asset* LoadBin(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table);

#ifdef NOZ_BUILTIN_ASSETS
#define NOZ_ASSET_DATA(name) name ## _DATA
#define NOZ_ASSET_DATA_SIZE(name) (u32)(sizeof(NOZ_ASSET_DATA(name)))
#else
#define NOZ_ASSET_DATA(name) nullptr
#define NOZ_ASSET_DATA_SIZE(name) 0
#endif

// @macros
#define NOZ_LOAD_SHADER(allocator, path, member) \
    member = (Shader*)LoadAsset(allocator, path, ASSET_TYPE_SHADER, LoadShader, NOZ_ASSET_DATA(member), NOZ_ASSET_DATA_SIZE(member));

#define NOZ_LOAD_TEXTURE(allocator, path, member) \
    member = (Texture*)LoadAsset(allocator, path, ASSET_TYPE_TEXTURE, LoadTexture, NOZ_ASSET_DATA(member), NOZ_ASSET_DATA_SIZE(member));

#define NOZ_LOAD_MESH(allocator, path, member) \
    member = (Mesh*)LoadAsset(allocator, path, ASSET_TYPE_MESH, LoadMesh, NOZ_ASSET_DATA(member), NOZ_ASSET_DATA_SIZE(member));

#define NOZ_LOAD_FONT(allocator, path, member) \
    member = (Font*)LoadAsset(allocator, path, ASSET_TYPE_FONT, LoadFont, NOZ_ASSET_DATA(member), NOZ_ASSET_DATA_SIZE(member));

#define NOZ_LOAD_VFX(allocator, path, member) \
    member = (Vfx*)LoadAsset(allocator, path, ASSET_TYPE_VFX, LoadVfx, NOZ_ASSET_DATA(member), NOZ_ASSET_DATA_SIZE(member));

#define NOZ_LOAD_SOUND(allocator, path, member) \
    member = (Sound*)LoadAsset(allocator, path, ASSET_TYPE_SOUND, LoadSound, NOZ_ASSET_DATA(member), NOZ_ASSET_DATA_SIZE(member));

#define NOZ_LOAD_SKELETON(allocator, path, member) \
    member = (Skeleton*)LoadAsset(allocator, path, ASSET_TYPE_SKELETON, LoadSkeleton, NOZ_ASSET_DATA(member), NOZ_ASSET_DATA_SIZE(member));

#define NOZ_LOAD_ANIMATION(allocator, path, member) \
    member = (Animation*)LoadAsset(allocator, path, ASSET_TYPE_ANIMATION, LoadAnimation, NOZ_ASSET_DATA(member), NOZ_ASSET_DATA_SIZE(member));

#define NOZ_LOAD_ANIMATEDMESH(allocator, path, member) \
    member = static_cast<AnimatedMesh*>(LoadAsset(allocator, path, ASSET_TYPE_ANIMATED_MESH, LoadAnimatedMesh));

#define NOZ_LOAD_BIN(allocator, path, member) \
    member = static_cast<Bin*>(LoadAsset(allocator, path, ASSET_TYPE_BIN, LoadBin,  NOZ_ASSET_DATA(member), NOZ_ASSET_DATA_SIZE(member)));

#define NOZ_RELOAD_FONT(asset_name, asset)
#define NOZ_RELOAD_SOUND(asset_name, asset)
#define NOZ_RELOAD_SKELETON(asset_name, asset)
#define NOZ_RELOAD_ANIMATION(asset_name, asset)
#define NOZ_RELOAD_ANIMATEDMESH(asset_name, asset)
#define NOZ_RELOAD_BIN(asset_name, asset)

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
