//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz::lua {
    extern void InitLuaRender(lua_State*);
    extern void InitLuaUI(lua_State*);
    extern void InitLuaColor(lua_State*);

    struct StateImpl : State {
        lua_State* L;
        int update_ref = LUA_NOREF;
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
}

static int RegisterUpdate(lua_State* L) {
    StateImpl* state_impl = static_cast<StateImpl*>(lua_getthreaddata(L));

    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1);

    state_impl->update_ref = lua_ref(L, 1);

    return 0;
}


State* noz::lua::CreateState(Allocator* allocator) {
    StateImpl* state = static_cast<StateImpl*>(Alloc(allocator, sizeof(State)));

    auto L = luaL_newstate();
    const luaL_Reg* lib = g_lua_libs;
    for (; lib->func; lib++) {
        lua_pushcfunction(L, lib->func, nullptr);
        lua_pushstring(L, lib->name);
        lua_call(L, 1, 0);
    }

    InitLuaRender(L);
    InitLuaColor(L);
    InitLuaUI(L);

    luaL_Reg statics[] = {
        { "print", Print },
        { "RegisterUpdate", RegisterUpdate },
        { nullptr, nullptr }
    };

    lua_pushvalue(L, LUA_GLOBALSINDEX);
    luaL_register(L, nullptr, statics);
    lua_setthreaddata(L, state);

    state->L = L;

    return state;
}

void noz::lua::Update(State* state) {
    StateImpl* state_impl = static_cast<StateImpl*>(state);
    if (state_impl->update_ref == LUA_NOREF)
        return;

    lua_getref(state_impl->L, state_impl->update_ref);
    if (lua_pcall(state_impl->L, 0, 0, 0) != 0) {
        const char* msg = lua_tostring(state_impl->L, -1);
        LogError("lua update: %s", msg ? msg : "unknown error");
        lua_pop(state_impl->L, 1);
    }
}

void noz::lua::Load(State* state, Script* script) {
    StateImpl* state_impl = static_cast<StateImpl*>(state);
    ScriptImpl* script_impl = static_cast<ScriptImpl*>(script);

    if (luau_load(state_impl->L, GetName(script)->value, reinterpret_cast<char*>(script_impl->byte_code.code), script_impl->byte_code.size, -1)) {
        size_t len;
        const char* msg = lua_tolstring(state_impl->L, -1, &len);
        std::string error(msg, len);
        LogError("lua: %s", error.c_str());
        lua_pop(state_impl->L, 1);
        return;
    }

    if (lua_pcall(state_impl->L, 0, 0, 0) != 0) {
        const char* msg = lua_tostring(state_impl->L, -1);
        LogError("lua load: %s", msg ? msg : "unknown error");
        lua_pop(state_impl->L, 1);
    }
}

void noz::lua::SetGlobal(State* state, const char* name, Asset* asset) {
    StateImpl* state_impl = static_cast<StateImpl*>(state);
    Wrap(state_impl->L, asset);
    lua_setglobal(state_impl->L, name);
}
