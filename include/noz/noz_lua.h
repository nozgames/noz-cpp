//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::lua {

    struct ByteCode {
        u8* code;
        u32 size;
        const Name* name;
    };

    struct LuaObject;
    struct Script : Asset {};

    struct State {};

    extern void InitLua();
    extern ByteCode CompileLua(const char* code);
    extern void Update(State* state);

    extern State* CreateState(Allocator* allocator);
    extern void Load(State* state, Script* script);

    extern void SetGlobal(State* state, const char* name, Asset* asset);
}

extern noz::lua::Script** LUA;
extern int LUA_COUNT;
