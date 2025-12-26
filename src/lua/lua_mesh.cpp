//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

using namespace noz::lua;

LuaAsset* noz::lua::Wrap(lua_State* L, Asset* asset) {
    LuaAsset* lua_asset = static_cast<LuaAsset*>(lua_newuserdata(L, sizeof(LuaAsset)));

    if (asset->type == ASSET_TYPE_MESH)
        lua_asset->type = LUA_OBJECT_TYPE_MESH;
    else
        lua_asset->type = LUA_OBJECT_TYPE_UNKNOWN;

    lua_asset->asset = asset;
    return lua_asset;
}
