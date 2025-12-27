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

static int LuaLoadMesh(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const Name* name = GetName(path);
    Mesh* mesh = (Mesh*)LoadAsset(ALLOCATOR_DEFAULT, name, ASSET_TYPE_MESH, LoadMesh);
    if (mesh) {
        Wrap(L, (Asset*)mesh);
        return 1;
    }
    return 0;
}

static int LuaLoadTexture(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const Name* name = GetName(path);
    Texture* texture = (Texture*)LoadAsset(ALLOCATOR_DEFAULT, name, ASSET_TYPE_TEXTURE, LoadTexture);
    if (texture) {
        Wrap(L, (Asset*)texture);
        return 1;
    }
    return 0;
}

static int LuaLoadShader(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const Name* name = GetName(path);
    Shader* shader = (Shader*)LoadAsset(ALLOCATOR_DEFAULT, name, ASSET_TYPE_SHADER, LoadShader);
    if (shader) {
        Wrap(L, (Asset*)shader);
        return 1;
    }
    return 0;
}

static int LuaLoadFont(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const Name* name = GetName(path);
    Font* font = (Font*)LoadAsset(ALLOCATOR_DEFAULT, name, ASSET_TYPE_FONT, LoadFont);
    if (font) {
        Wrap(L, (Asset*)font);
        return 1;
    }
    return 0;
}

static int LuaLoadSound(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const Name* name = GetName(path);
    Sound* sound = (Sound*)LoadAsset(ALLOCATOR_DEFAULT, name, ASSET_TYPE_SOUND, LoadSound);
    if (sound) {
        Wrap(L, (Asset*)sound);
        return 1;
    }
    return 0;
}

static int LuaLoadSkeleton(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const Name* name = GetName(path);
    Skeleton* skeleton = (Skeleton*)LoadAsset(ALLOCATOR_DEFAULT, name, ASSET_TYPE_SKELETON, LoadSkeleton);
    if (skeleton) {
        Wrap(L, (Asset*)skeleton);
        return 1;
    }
    return 0;
}

static int LuaLoadAnimation(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const Name* name = GetName(path);
    Animation* animation = (Animation*)LoadAsset(ALLOCATOR_DEFAULT, name, ASSET_TYPE_ANIMATION, LoadAnimation);
    if (animation) {
        Wrap(L, (Asset*)animation);
        return 1;
    }
    return 0;
}

static int LuaLoadAnimatedMesh(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const Name* name = GetName(path);
    AnimatedMesh* animated_mesh = (AnimatedMesh*)LoadAsset(ALLOCATOR_DEFAULT, name, ASSET_TYPE_ANIMATED_MESH, LoadAnimatedMesh);
    if (animated_mesh) {
        Wrap(L, (Asset*)animated_mesh);
        return 1;
    }
    return 0;
}

static int LuaLoadVfx(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const Name* name = GetName(path);
    Vfx* vfx = (Vfx*)LoadAsset(ALLOCATOR_DEFAULT, name, ASSET_TYPE_VFX, LoadVfx);
    if (vfx) {
        Wrap(L, (Asset*)vfx);
        return 1;
    }
    return 0;
}

void noz::lua::InitLuaAsset(lua_State* L) {
    luaL_Reg funcs[] = {
        { "LoadMesh", LuaLoadMesh },
        { "LoadTexture", LuaLoadTexture },
        { "LoadShader", LuaLoadShader },
        { "LoadFont", LuaLoadFont },
        { "LoadSound", LuaLoadSound },
        { "LoadSkeleton", LuaLoadSkeleton },
        { "LoadAnimation", LuaLoadAnimation },
        { "LoadAnimatedMesh", LuaLoadAnimatedMesh },
        { "LoadVfx", LuaLoadVfx },
        { nullptr, nullptr }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, nullptr, funcs);
    lua_pop(L, 1);
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
