//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::lua {
    void InitLuaRender(lua_State*);

    struct LuaStateImpl : State {
        lua_State* L;
    };
}

using namespace noz::lua;

static const luaL_Reg g_lua_libs[] = {
    {"", luaopen_base},
    {LUA_TABLIBNAME, luaopen_table},
    {LUA_OSLIBNAME, luaopen_os},
    {LUA_STRLIBNAME, luaopen_string},
    {LUA_MATHLIBNAME, luaopen_math},
    {LUA_DBLIBNAME, luaopen_debug},
    {LUA_UTF8LIBNAME, luaopen_utf8},
    {LUA_BITLIBNAME, luaopen_bit32},
    {LUA_BUFFERLIBNAME, luaopen_buffer},
    {nullptr, nullptr}
};

ByteCode noz::lua::CompileLua(const char* code) {
    size_t size = 0;
    u8* bytes = reinterpret_cast<u8*>(luau_compile(code, strlen(code), nullptr, &size));
    ByteCode result = {};
    result.name = nullptr;
    result.code = bytes;
    result.size = static_cast<u32>(size);
    return result;
}

static int Print(lua_State* L) {
    int nargs = lua_gettop(L);
    std::string out;
    for (int i = 1; i <= nargs; ++i) {
        auto s = lua_tostring(L, i);
        if (s != nullptr) {
            if (i > 1)
                out += " ";

            out += s;
        }
    }

    LogInfo(out.c_str());

    return 0;
}

void noz::lua::InitLua() {
    auto L = luaL_newstate();
    const luaL_Reg* lib = g_lua_libs;
    for (; lib->func; lib++) {
        lua_pushcfunction(L, lib->func, nullptr);
        lua_pushstring(L, lib->name);
        lua_call(L, 1, 0);
    }

    InitLuaRender(L);

    luaL_Reg statics[] = {
        { "print", Print },
        { nullptr, nullptr }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, nullptr, statics);

    // auto bytes = CompileLua("print('Hello from Lua!')");
    // if (luau_load(L, "test.lua", reinterpret_cast<char*>(bytes->code), bytes->size, -1)) {
    //     size_t len;
    //     const char* msg = lua_tolstring(L, -1, &len);
    //     std::string error(msg, len);
    //     LogError("Lua compile error: %s", error.c_str());
    //     lua_pop(L, 1);
    //     return;
    // }

    //lua_pcall(L, 0, 0, 0);
}



State* noz::lua::CreateState(Allocator* allocator) {
    LuaStateImpl* state = static_cast<LuaStateImpl*>(Alloc(allocator, sizeof(State)));
    state->L = luaL_newstate();

    const luaL_Reg* lib = g_lua_libs;
    for (; lib->func; lib++) {
        lua_pushcfunction(state->L, lib->func, nullptr);
        lua_pushstring(state->L, lib->name);
        lua_call(state->L, 1, 0);
    }

    InitLuaRender(state->L);

    luaL_Reg statics[] = {
        { "print", Print },
        { nullptr, nullptr }
    };

    lua_pushvalue(state->L, LUA_GLOBALSINDEX);
    luaL_register(state->L, nullptr, statics);

    return state;
}

void noz::lua::Load(State* state, Script* script) {
    LuaStateImpl* state_impl = static_cast<LuaStateImpl*>(state);
    LuaScriptImpl* script_impl = static_cast<LuaScriptImpl*>(script);

    if (luau_load(state_impl->L, GetName(script)->value, reinterpret_cast<char*>(script_impl->byte_code.code), script_impl->byte_code.size, -1)) {
        size_t len;
        const char* msg = lua_tolstring(state_impl->L, -1, &len);
        std::string error(msg, len);
        LogError("lua: %s", error.c_str());
        lua_pop(state_impl->L, 1);
        return;
    }

    lua_pcall(state_impl->L, 0, 0, 0);
}
