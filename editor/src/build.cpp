//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace fs = std::filesystem;

struct BuildAssetEntry {
    AssetData* asset;
    std::string var_name;
};

struct BuildCollector {
    std::vector<BuildAssetEntry> assets;
    AssetType type;
};

static bool CollectBuildAsset(u32, void* item_data, void* user_data) {
    BuildCollector* collector = static_cast<BuildCollector*>(user_data);
    AssetData* a = static_cast<AssetData*>(item_data);

    if (a->editor_only)
        return true;

    if (a->type != collector->type)
        return true;

    std::string type_upper = ToString(a->type);
    Upper(type_upper.data(), (u32)type_upper.size());

    std::string name_upper = a->name->value;
    Upper(name_upper.data(), (u32)name_upper.size());

    std::string var_name = type_upper + "_" + name_upper;

    // Skip if asset with same var_name already exists from an earlier source path
    for (auto& existing : collector->assets) {
        if (existing.var_name == var_name) {
            // Keep the one from the earlier source path (lower asset_path_index)
            if (a->asset_path_index < existing.asset->asset_path_index) {
                existing.asset = a;
            }
            return true;
        }
    }

    collector->assets.push_back({
        .asset = a,
        .var_name = var_name
    });

    return true;
}

static void WriteBuildAsset(FILE* file, AssetData* a, const char* extension, const char* suffix) {
    std::string type_upper = ToString(a->type);
    Upper(type_upper.data(), (u32)type_upper.size());

    std::string name_upper = a->name->value;
    Upper(name_upper.data(), (u32)name_upper.size());

    std::string suffix_upper;
    if (suffix) {
        suffix_upper = suffix;
        Upper(suffix_upper.data(), (u32)suffix_upper.size());
    }

    fprintf(file, "static u8 %s_%s%s_DATA[] = {", type_upper.c_str(), name_upper.c_str(), suffix_upper.c_str());

    fs::path asset_path = GetTargetPath(a);
    if (extension)
        asset_path += extension;

    FILE* asset_file = fopen(asset_path.string().c_str(), "rb");
    if (asset_file) {
        char buffer[1024];
        size_t bytes_read;
        bool first = true;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), asset_file)) > 0) {
            for (size_t i = 0; i < bytes_read; i++) {
                fprintf(file, first ? "%u" : ",%u", static_cast<unsigned char>(buffer[i]));
                first = false;
            }
        }
        fclose(asset_file);
    }

    fprintf(file, "};\n\n");
}

void Build() {
    const fs::path& manifest_path = GetManifestCppPath();
    fs::path build_path = manifest_path;
    build_path.replace_extension("");
    build_path = build_path.string() + "_build.cpp";

    fs::path header_path = manifest_path.filename();
    header_path.replace_extension(".h");

    FILE* file = fopen(build_path.string().c_str(), "wt");

    fprintf(file, "#include \"%s\"\n\n", header_path.string().c_str());
    fprintf(file, "#if !defined(DEBUG)\n\n");

    try
    {
        std::filesystem::create_directory(manifest_path.parent_path());
    }
    catch (...)
    {
    }

    // Iterate over all asset types
    for (int type = 0; type < ASSET_TYPE_COUNT; type++) {
        AssetType asset_type = static_cast<AssetType>(type);

        // Collect unique assets for this type
        BuildCollector collector = { .type = asset_type };
        Enumerate(g_editor.asset_allocator, CollectBuildAsset, &collector);

        if (asset_type == ASSET_TYPE_SHADER) {
            fprintf(file, "#ifdef NOZ_PLATFORM_GLES\n\n");
            for (auto& entry : collector.assets)
                WriteBuildAsset(file, entry.asset, ".gles", nullptr);

            fprintf(file, "#elif NOZ_PLATFORM_GL\n\n");
            for (auto& entry : collector.assets)
                WriteBuildAsset(file, entry.asset, ".glsl", nullptr);

            fprintf(file, "#else\n\n");
            for (auto& entry : collector.assets)
                WriteBuildAsset(file, entry.asset, nullptr, nullptr);
            fprintf(file, "#endif\n\n");

        } else {
            for (auto& entry : collector.assets)
                WriteBuildAsset(file, entry.asset, nullptr, nullptr);
        }
    }

    fprintf(file, "\n#endif\n");
    fclose(file);
}
