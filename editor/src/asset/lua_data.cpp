//
//  NozEd - Copyright(c) 2025 NoZ Games, LLC
//

static void Init(LuaData* l);

extern Mesh* MESH_ASSET_ICON_LUA;

static void DrawLua(AssetData* a) {
    BindMaterial(g_view.shaded_material);
    BindColor(COLOR_WHITE);
    DrawMesh(MESH_ASSET_ICON_LUA, Translate(a->position));
}

static void FreeLuaData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_LUA);
    LuaData* l = static_cast<LuaData*>(a);

    free(l->byte_code.code);
    l->byte_code.code = nullptr;
    l->byte_code.size = 0;
}

void LoadLuaData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_LUA);
    LuaData* l = static_cast<LuaData*>(a);

    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, a->path);
    l->byte_code = noz::lua::CompileLua(contents.c_str());
}

LuaData* LoadLuaData(const std::filesystem::path& path) {
    LuaData* l = static_cast<LuaData*>(CreateAssetData(path));
    assert(l);
    Init(l);
    LoadLuaData(l);
    return l;
}

static void ReloadLuaData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_LUA);
    LuaData* l = static_cast<LuaData*>(a);

    free(l->byte_code.code);
    l->byte_code.code = nullptr;
    l->byte_code.size = 0;
    LoadLuaData(a);
}

static void Init(LuaData* l) {
    l->vtable = {
        .destructor = FreeLuaData,
        .load = LoadLuaData,
        .reload = ReloadLuaData,
        .draw = DrawLua
    };
}

void InitLuaData(AssetData* a) {
    Init(static_cast<LuaData*>(a));
}
