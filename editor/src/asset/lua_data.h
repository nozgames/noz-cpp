//
//  NozEd - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

enum LuaScriptType : u8 {
    LUA_SCRIPT_TYPE_CLIENT,
    LUA_SCRIPT_TYPE_SERVER,
    LUA_SCRIPT_TYPE_MODULE
};

struct LuaData : AssetData {
    noz::lua::ByteCode byte_code;
    LuaScriptType script_type;
};

extern AssetImporter GetLuaImporter();
extern void InitLuaData(AssetData* a);
