//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::lua {
    extern void InitLuaColor(lua_State* L);
}

using namespace noz::lua;

namespace noz {
    static Color* LuaCheckColor(lua_State* L, int idx) {
        return static_cast<Color*>(lua_touserdata(L, idx));
    }

    static int LuaColor_new(lua_State* L) {
        float r = static_cast<float>(luaL_optnumber(L, 1, 0.0f));
        float g = static_cast<float>(luaL_optnumber(L, 2, 0.0f));
        float b = static_cast<float>(luaL_optnumber(L, 3, 0.0f));
        float a = static_cast<float>(luaL_optnumber(L, 4, 1.0f));

        Color* color = static_cast<Color*>(lua_newuserdata(L, sizeof(Color)));
        *color = { r, g, b, a };

        luaL_getmetatable(L, "Color");
        lua_setmetatable(L, -2);

        return 1;
    }

    static int LuaColor_index(lua_State* L) {
        Color* color = LuaCheckColor(L, 1);
        const char* key = lua_tostring(L, 2);

        if (strcmp(key, "r") == 0) lua_pushnumber(L, color->r);
        else if (strcmp(key, "g") == 0) lua_pushnumber(L, color->g);
        else if (strcmp(key, "b") == 0) lua_pushnumber(L, color->b);
        else if (strcmp(key, "a") == 0) lua_pushnumber(L, color->a);
        else lua_pushnil(L);

        return 1;
    }

    static int LuaColor_newindex(lua_State* L) {
        Color* color = LuaCheckColor(L, 1);
        const char* key = lua_tostring(L, 2);
        float value = static_cast<float>(lua_tonumber(L, 3));

        if (key[1] != 0) return 0;
        if (key[0] == 'r') color->r = value;
        if (key[0] == 'g') color->g = value;
        if (key[0] == 'b') color->b = value;
        if (key[0] == 'a') color->a = value;

        return 0;
    }

    static int LuaColor_tostring(lua_State* L) {
        Color* color = LuaCheckColor(L, 1);
        lua_pushfstring(L, "Color(%f, %f, %f, %f)", color->r, color->g, color->b, color->a);
        return 1;
    }

    static int LuaColor_mul(lua_State* L) {
        Color* a = LuaCheckColor(L, 1);

        if (lua_isnumber(L, 2)) {
            float scalar = static_cast<float>(lua_tonumber(L, 2));
            Color* result = static_cast<Color*>(lua_newuserdata(L, sizeof(Color)));
            *result = { a->r * scalar, a->g * scalar, a->b * scalar, a->a * scalar };
            luaL_getmetatable(L, "Color");
            lua_setmetatable(L, -2);
        } else {
            Color* b = LuaCheckColor(L, 2);
            Color* result = static_cast<Color*>(lua_newuserdata(L, sizeof(Color)));
            *result = { a->r * b->r, a->g * b->g, a->b * b->b, a->a * b->a };
            luaL_getmetatable(L, "Color");
            lua_setmetatable(L, -2);
        }

        return 1;
    }

    void noz::lua::InitLuaColor(lua_State* L) {
        luaL_newmetatable(L, "Color");

        lua_pushcfunction(L, LuaColor_index, "Color_index");
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, LuaColor_newindex, "Color_newindex");
        lua_setfield(L, -2, "__newindex");

        lua_pushcfunction(L, LuaColor_tostring, "Color_tostring");
        lua_setfield(L, -2, "__tostring");

        lua_pushcfunction(L, LuaColor_mul, "Color_mul");
        lua_setfield(L, -2, "__mul");

        lua_pop(L, 1);

        lua_newtable(L);
        lua_pushcfunction(L, LuaColor_new, "Color_new");
        lua_setfield(L, -2, "new");
        lua_setglobal(L, "Color");
    }
}
