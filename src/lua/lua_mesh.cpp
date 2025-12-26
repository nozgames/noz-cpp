//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

using namespace noz::lua;

static int LuaAsset_tostring(lua_State* L) {
    LuaAsset* lua_asset = static_cast<LuaAsset*>(lua_touserdata(L, 1));
    if (lua_asset && lua_asset->asset && lua_asset->asset->name) {
        lua_pushstring(L, lua_asset->asset->name->value);
    } else {
        lua_pushstring(L, "unknown asset");
    }
    return 1;
}

LuaAsset* noz::lua::Wrap(lua_State* L, Asset* asset) {
    LuaAsset* lua_asset = static_cast<LuaAsset*>(lua_newuserdata(L, sizeof(LuaAsset)));

    if (asset->type == ASSET_TYPE_MESH)
        lua_asset->type = LUA_OBJECT_TYPE_MESH;
    else
        lua_asset->type = LUA_OBJECT_TYPE_UNKNOWN;

    lua_asset->asset = asset;

    lua_newtable(L);
    lua_pushcfunction(L, LuaAsset_tostring, "LuaAsset_tostring");
    lua_setfield(L, -2, "__tostring");
    lua_setmetatable(L, -2);

    return lua_asset;
}
