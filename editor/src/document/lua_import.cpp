//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

extern void LoadLuaData(Document* a);

DocumentImporter GetLuaImporter() {
    return {
        .type = ASSET_TYPE_LUA,
        .ext = ".lua",
        .import_func = ImportLua
    };
}
