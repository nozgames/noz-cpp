//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

namespace noz::lua {

    struct ByteCode {
        u8* code;
        u32 size;
        const Name* name;
    };

    struct Script : Asset {};

    struct State {};

    extern void InitLua();
    extern ByteCode CompileLua(const char* code);

    extern State* CreateState(Allocator* allocator);
    extern void Load(State* state, Script* script);
}

extern noz::lua::Script** LUA;
extern int LUA_COUNT;
