//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::lua {
    extern void InitLuaRender(lua_State*);
}

using namespace noz::lua;

static int LuaDrawMesh(lua_State* L) {
    LuaAsset* mesh = static_cast<LuaAsset*>(lua_touserdata(L, 0));
    if (!mesh) return 0;

    DrawMesh(static_cast<Mesh*>(mesh->asset));

    return 0;
}

void noz::lua::InitLuaRender(lua_State* L) {
    luaL_Reg statics[] = {
        { "DrawMesh", LuaDrawMesh },
        { nullptr, nullptr }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, nullptr, statics);
}
