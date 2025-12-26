//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

float noz::lua::GetNumberField(lua_State* L, int index, const char* field_name, float default_value) {
    lua_getfield(L, index, field_name);
    float value = default_value;
    if (!lua_isnil(L, -1)) value = static_cast<float>(lua_tonumber(L, -1));
    lua_pop(L, 1);
    return value;
}

Align noz::lua::GetAlignField(lua_State* L, int index, const char* field_name, Align default_value) {
    lua_getfield(L, index, field_name);
    Align value = default_value;
    if (!lua_isnil(L, -1)) value = static_cast<Align>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    return value;
}

Color noz::lua::GetColorField(lua_State* L, int index, const char* field_name, const Color& default_value) {
    lua_getfield(L, index, field_name);
    Color value = default_value;
    if (lua_isuserdata(L, -1)) value = *static_cast<Color*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return value;
}

u8 noz::lua::GetU8Field(lua_State* L, int index, const char* field_name, u8 default_value) {
    lua_getfield(L, index, field_name);
    u8 value = default_value;
    if (!lua_isnil(L, -1)) value = static_cast<u8>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    return value;
}

i32 noz::lua::GetIntField(lua_State* L, int index, const char* field_name, int default_value) {
    lua_getfield(L, index, field_name);
    int value = default_value;
    if (!lua_isnil(L, -1)) value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return value;
}

Asset* noz::lua::GetAssetField(lua_State* L, int index, const char* field_name, Asset* default_value) {
    lua_getfield(L, index, field_name);
    Asset* value = default_value;
    if (lua_isuserdata(L, -1)) {
        LuaAsset* lua_asset = static_cast<LuaAsset*>(lua_touserdata(L, -1));
        if (lua_asset) value = lua_asset->asset;
    }
    lua_pop(L, 1);
    return value;
}
