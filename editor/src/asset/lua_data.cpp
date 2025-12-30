//
//  NozEd - Copyright(c) 2025 NoZ Games, LLC
//

#pragma warning(push)
#pragma warning(disable: 4100) // unreferenced parameter
#include <Luau/Parser.h>
#pragma warning(pop)

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

static LuaScriptType ParseScriptType(const char* contents) {
    Luau::ParseOptions options;
    Luau::Allocator allocator;
    Luau::AstNameTable names(allocator);

    Luau::ParseResult result = Luau::Parser::parse(contents, strlen(contents), names, allocator, options);

    LuaScriptType script_type = LUA_SCRIPT_TYPE_MODULE;

    // Look for --!Type(...) in hot comments
    for (const Luau::HotComment& comment : result.hotcomments) {
        // content will be something like "Type(Client)"
        if (comment.content.compare(0, 5, "Type(") == 0) {
            size_t end = comment.content.find(')', 5);
            if (end != std::string::npos) {
                std::string type_value = comment.content.substr(5, end - 5);
                if (type_value == "Client") {
                    script_type = LUA_SCRIPT_TYPE_CLIENT;
                } else if (type_value == "Server") {
                    script_type = LUA_SCRIPT_TYPE_SERVER;
                } else if (type_value == "UI") {
                    script_type = LUA_SCRIPT_TYPE_MODULE;
                }
            }
            break;
        }
    }

    return script_type;
}

void LoadLuaData(AssetData* a) {
    assert(a);
    assert(a->type == ASSET_TYPE_LUA);
    LuaData* l = static_cast<LuaData*>(a);

    std::string contents = ReadAllText(ALLOCATOR_DEFAULT, a->path);
    l->byte_code = noz::lua::CompileLua(contents.c_str());
    l->script_type = ParseScriptType(contents.c_str());
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
