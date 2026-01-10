//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#if defined(NOZ_BUILTIN_ASSETS)
#define ReloadMesh nullptr
#define ReloadVfx nullptr
#define ReloadTexture nullptr
#define ReloadShader nullptr
#define ReloadLuaScript nullptr
#endif

namespace noz {
    // @asset_registry
    static AssetDef g_asset_defs[ASSET_TYPE_COUNT] = {};

    void InitAssetDef(const AssetDef& info) {        
        g_asset_defs[info.type] = info;
    }

    const AssetDef* GetAssetDef(AssetType type) {
        return &g_asset_defs[type];
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
        const AssetDef* info = GetAssetDef(asset_type);
        return info ? info->name : nullptr;
    }

    const char* ToTypeString(AssetType asset_type) {
        if (asset_type == ASSET_TYPE_LUA) return "noz::lua::Script";
        return ToString(asset_type);
    }

    const char* ToShortString(AssetType asset_type) {
        const AssetDef* info = GetAssetDef(asset_type);
        if (info && info->short_name) return info->short_name;
        return ToString(asset_type);
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
            name_table = (const Name**)Alloc(ALLOCATOR_DEFAULT, sizeof(Name*) * header.names);
            for (u32 i = 0; i < header.names; i++)
                name_table[i] = ReadName(stream);
        }

        return name_table;
    }

    Asset* LoadAssetInternal(Allocator* allocator, const Name* asset_name, AssetType asset_type, AssetLoaderFunc loader, Stream* stream) {
        AssetHeader header = {};
        if (!ReadAssetHeader(stream, &header) || !ValidateAssetHeader(&header, asset_type)) {
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
            ? LoadStream(ALLOCATOR_DEFAULT, data, data_size)
            : LoadAssetStream(ALLOCATOR_DEFAULT, asset_name, asset_type);

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

        Stream* stream = LoadAssetStream(ALLOCATOR_DEFAULT, name, asset_type);
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
        InitAssetDef({ASSET_TYPE_MESH, "Mesh", "Mesh", LoadMesh, ReloadMesh});
        InitAssetDef({ASSET_TYPE_VFX, "Vfx", "Vfx", LoadVfx, ReloadVfx});
        InitAssetDef({ASSET_TYPE_SKELETON, "Skeleton", "Skeleton", LoadSkeleton, nullptr});
        InitAssetDef({ASSET_TYPE_ANIMATION, "Animation", "Animation", LoadAnimation, nullptr});
        InitAssetDef({ASSET_TYPE_SOUND, "Sound", "Sound", LoadSound, nullptr});
        InitAssetDef({ASSET_TYPE_TEXTURE, "Texture", "Texture", LoadTexture, ReloadTexture});
        InitAssetDef({ASSET_TYPE_FONT, "Font", "Font", LoadFont, nullptr});
        InitAssetDef({ASSET_TYPE_SHADER, "Shader", "Shader", LoadShader, ReloadShader});
        InitAssetDef({ASSET_TYPE_EVENT, "Event", "Event", nullptr, nullptr});
        InitAssetDef({ASSET_TYPE_BIN, "Bin", "Bin", LoadBin, nullptr});
        InitAssetDef({ASSET_TYPE_ATLAS, "Atlas", "Atlas", LoadTexture, nullptr});
        InitAssetDef({ASSET_TYPE_SPRITE, "Sprite", "Sprite", LoadSprite, nullptr});

#if defined(NOZ_LUA)
        InitAssetDef({ASSET_TYPE_LUA, "Script", "Lua", LoadLuaScript, ReloadLuaScript});
#endif

#if !defined(NDEBUG)
        for (int i = 0; i < ASSET_TYPE_COUNT; i++) {
            assert(g_asset_defs[i].type == (AssetType)i);
            assert(g_asset_defs[i].name != nullptr);
        }
#endif
    }
}
