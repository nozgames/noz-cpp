//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma warning(push)
#pragma warning(disable: 4100) // unreferenced parameter
#include <Luau/Parser.h>
#pragma warning(pop)

namespace noz::editor {

    static void InitLuaDocument(LuaDocument* l);

    static void DrawLuaDocument(Document* a) {
        BindMaterial(g_view.editor_mesh_material);
        BindColor(COLOR_WHITE);
        DrawMesh(MESH_ASSET_ICON_LUA, Translate(a->position));
    }

    static void DestroyLoadDocument(Document* doc) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_LUA);
        LuaDocument* ldoc = static_cast<LuaDocument*>(doc);

        free(ldoc->byte_code.code);
        ldoc->byte_code.code = nullptr;
        ldoc->byte_code.size = 0;
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

    void LoadLuaDocument(Document* doc) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_LUA);
        LuaDocument* ldoc = static_cast<LuaDocument*>(doc);

        std::string contents = ReadAllText(ALLOCATOR_DEFAULT, doc->path.value);
        ldoc->byte_code = lua::CompileLua(contents.c_str());
        ldoc->script_type = ParseScriptType(contents.c_str());
    }

    LuaDocument* LoadLuaDocument(const std::filesystem::path& path) {
        LuaDocument* ldoc = static_cast<LuaDocument*>(CreateAssetData(path));
        assert(ldoc);
        InitLuaDocument(ldoc);
        LoadLuaDocument(ldoc);
        return ldoc;
    }

    static void ReloadLuaDocument(Document* doc) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_LUA);
        LuaDocument* ldoc = static_cast<LuaDocument*>(doc);

        free(ldoc->byte_code.code);
        ldoc->byte_code.code = nullptr;
        ldoc->byte_code.size = 0;
        LoadLuaDocument(doc);
    }

    static void CloneLuaDocument(Document* doc) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_LUA);
        LuaDocument* ldoc = static_cast<LuaDocument*>(doc);

        u8* code = static_cast<u8*>(malloc(ldoc->byte_code.size));
        memcpy(code, ldoc->byte_code.code, ldoc->byte_code.size);
        ldoc->byte_code.code = code;
        ldoc->script_type = ldoc->script_type;
    }

    static void ImportLuaDocument(Document* doc, const std::filesystem::path& path, Props* config, Props* meta) {
        assert(doc);
        assert(doc->type == ASSET_TYPE_LUA);
        LuaDocument* ldoc = static_cast<LuaDocument*>(doc);
        LoadLuaDocument(ldoc);

        AssetHeader header = {};
        header.signature = ASSET_SIGNATURE;
        header.type = ASSET_TYPE_LUA;
        header.version = 1;

        Stream* stream = CreateStream(nullptr, 4096);
        WriteAssetHeader(stream, &header);
        WriteU32(stream, ldoc->byte_code.size);
        WriteBytes(stream, ldoc->byte_code.code, ldoc->byte_code.size);
        SaveStream(stream, path);
        Free(stream);
    }

    static void InitLuaDocument(LuaDocument* l) {
        l->vtable = {
            .destructor = DestroyLoadDocument,
            .load = LoadLuaDocument,
            .reload = ReloadLuaDocument,
            .draw = DrawLuaDocument,
            .clone = CloneLuaDocument
        };
    }

    static void InitLuaDocument(Document* a) {
        InitLuaDocument(static_cast<LuaDocument*>(a));
    }

    void InitLuaDocumentDef() {
        InitDocumentDef({
            .type=ASSET_TYPE_LUA,
            .size=sizeof(LuaDocument),
            .ext=".lua",
            .init_func=InitLuaDocument,
            .import_func=ImportLuaDocument
        });
    }
}