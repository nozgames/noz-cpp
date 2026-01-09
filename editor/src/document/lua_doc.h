//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz::editor {

    enum LuaScriptType : u8 {
        LUA_SCRIPT_TYPE_CLIENT,
        LUA_SCRIPT_TYPE_SERVER,
        LUA_SCRIPT_TYPE_MODULE
    };

    struct LuaDocument : Document {
        lua::ByteCode byte_code;
        LuaScriptType script_type;
    };
}

