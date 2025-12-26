//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

static void ImportLua(AssetData* a, const std::filesystem::path& path, Props* config, Props* meta) {
    (void)config;
    (void)meta;

    assert(a);
    assert(a->type == ASSET_TYPE_LUA);
    LuaData* l = static_cast<LuaData*>(a);

    AssetHeader header = {};
    header.signature = ASSET_SIGNATURE;
    header.type = ASSET_TYPE_LUA;
    header.version = 1;

    Stream* stream = CreateStream(nullptr, 4096);
    WriteAssetHeader(stream, &header);
    WriteU32(stream, l->byte_code.size);
    WriteBytes(stream, l->byte_code.code, l->byte_code.size);
    SaveStream(stream, path);
    Free(stream);
}

AssetImporter GetLuaImporter() {
    return {
        .type = ASSET_TYPE_LUA,
        .ext = ".lua",
        .import_func = ImportLua
    };
}
