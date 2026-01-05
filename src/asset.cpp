//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

// @STL

#include <filesystem>
#include <cstring>

// @asset_registry
static AssetTypeInfo g_asset_types[MAX_ASSET_TYPES] = {};
static int g_asset_type_count = 0;

void RegisterAssetType(const AssetTypeInfo& info) {
    assert(g_asset_type_count < MAX_ASSET_TYPES);

    // Check for duplicate type_id
    for (int i = 0; i < g_asset_type_count; i++) {
        if (g_asset_types[i].type_id == info.type_id) {
            assert(false && "Duplicate asset type_id registered");
            return;
        }
    }

    g_asset_types[g_asset_type_count++] = info;
}

const AssetTypeInfo* GetAssetTypeInfo(int type_id) {
    for (int i = 0; i < g_asset_type_count; i++) {
        if (g_asset_types[i].type_id == type_id)
            return &g_asset_types[i];
    }
    return nullptr;
}

const AssetTypeInfo* FindAssetTypeByExtension(const char* ext) {
    if (!ext) return nullptr;
    for (int i = 0; i < g_asset_type_count; i++) {
        if (g_asset_types[i].extension && strcmp(g_asset_types[i].extension, ext) == 0)
            return &g_asset_types[i];
    }
    return nullptr;
}

int GetRegisteredAssetTypeCount() {
    return g_asset_type_count;
}

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
    if (!header)
        return false;
    if (header->signature != ASSET_SIGNATURE)
        return false;
    if (header->type != expected_asset_type)
        return false;
    return true;
}

const char* ToString(AssetType asset_type) {
#if 0
    // Built-in types (for backward compatibility and performance)
    switch (asset_type) {
        case ASSET_TYPE_TEXTURE: return "Texture";
        case ASSET_TYPE_MESH: return "Mesh";
        case ASSET_TYPE_FONT: return "Font";
        case ASSET_TYPE_SOUND: return "Sound";
        case ASSET_TYPE_SKELETON: return "Skeleton";
        case ASSET_TYPE_ANIMATION: return "Animation";
        case ASSET_TYPE_VFX: return "Vfx";
        case ASSET_TYPE_SHADER: return "Shader";
        case ASSET_TYPE_ANIMATED_MESH: return "AnimatedMesh";
        case ASSET_TYPE_EVENT: return "Event";
        case ASSET_TYPE_BIN: return "Bin";
        case ASSET_TYPE_LUA: return "Script";
        default: break;
    }
#endif

    const AssetTypeInfo* info = GetAssetTypeInfo(asset_type);
    return info ? info->name : nullptr;
}

const char* ToTypeString(AssetType asset_type) {
    if (asset_type == ASSET_TYPE_LUA) return "noz::lua::Script";
    return ToString(asset_type);
}

const char* ToShortString(AssetType asset_type) {
    const AssetTypeInfo* info = GetAssetTypeInfo(asset_type);
    if (info && info->short_name) return info->short_name;
    return ToString(asset_type);
}

const char* GetExtensionFromAssetType(AssetType asset_type) {
    const AssetTypeInfo* info = GetAssetTypeInfo(asset_type);
    if (info) return info->extension;
    return nullptr;
}

static Stream* LoadAssetStream(Allocator* allocator, const Name* asset_name, AssetType asset_type) {
    assert(asset_name);

    std::filesystem::path asset_name_path = std::filesystem::path(ToString(asset_type)) / asset_name->value;

#ifdef NOZ_PLATFORM_GLES
    if (asset_type == ASSET_TYPE_SHADER) {
        asset_name_path += ".gles";
    }
#elif NOZ_PLATFORM_GL
    if (asset_type == ASSET_TYPE_SHADER) {
        asset_name_path += ".glsl";
    }
#endif

    std::string lower_asset_name_path = asset_name_path.string();
    Lower(lower_asset_name_path.data(), (u32)lower_asset_name_path.size());

    // Search through all asset paths in order
    const char** asset_paths = GetApplicationTraits()->asset_paths;
    if (asset_paths) {
        for (int i = 0; asset_paths[i] != nullptr; i++) {
            std::filesystem::path asset_path = std::filesystem::path(asset_paths[i]) / lower_asset_name_path;
            Stream* stream = LoadStream(allocator, asset_path);
            if (stream)
                return stream;
        }
    }

    return nullptr;
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
    asset->type = header.type;

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

#if !defined(NOZ_BUILTIN_ASSETS)

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

void InitAssets() {
    // Register all built-in asset types with the registry
    RegisterAssetType({ASSET_TYPE_MESH, "Mesh", "Mesh", ".mesh", LoadMesh,
#if !defined(NOZ_BUILTIN_ASSETS)
        ReloadMesh
#else
        nullptr
#endif
    });
    RegisterAssetType({ASSET_TYPE_VFX, "Vfx", "Vfx", ".vfx", LoadVfx,
#if !defined(NOZ_BUILTIN_ASSETS)
        ReloadVfx
#else
        nullptr
#endif
    });
    RegisterAssetType({ASSET_TYPE_SKELETON, "Skeleton", "Skeleton", ".skeleton", LoadSkeleton, nullptr});
    RegisterAssetType({ASSET_TYPE_ANIMATION, "Animation", "Animation", ".animation", LoadAnimation, nullptr});
    RegisterAssetType({ASSET_TYPE_SOUND, "Sound", "Sound", ".sound", LoadSound, nullptr});
    RegisterAssetType({ASSET_TYPE_TEXTURE, "Texture", "Texture", ".texture", LoadTexture,
#if !defined(NOZ_BUILTIN_ASSETS)
        ReloadTexture
#else
        nullptr
#endif
    });
    RegisterAssetType({ASSET_TYPE_FONT, "Font", "Font", ".font", LoadFont, nullptr});
    RegisterAssetType({ASSET_TYPE_SHADER, "Shader", "Shader", ".shader", LoadShader,
#if !defined(NOZ_BUILTIN_ASSETS)
        ReloadShader
#else
        nullptr
#endif
    });
    RegisterAssetType({ASSET_TYPE_ANIMATED_MESH, "AnimatedMesh", "AnimMesh", ".animatedmesh", LoadAnimatedMesh, nullptr});
    RegisterAssetType({ASSET_TYPE_EVENT, "Event", "Event", ".event", nullptr, nullptr});
    RegisterAssetType({ASSET_TYPE_BIN, "Bin", "Bin", ".bin", LoadBin, nullptr});
    RegisterAssetType({ASSET_TYPE_ATLAS, "Atlas", "Atlas", ".atlas", nullptr, nullptr});

#if defined(NOZ_LUA)
    RegisterAssetType({ASSET_TYPE_LUA, "Script", "Lua", ".lua", LoadLuaScript,
#if !defined(NOZ_BUILTIN_ASSETS)
        ReloadLuaScript
#else
        nullptr
#endif
    });
#endif
}