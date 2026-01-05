//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

enum LuaScriptType : u8 {
    LUA_SCRIPT_TYPE_CLIENT,
    LUA_SCRIPT_TYPE_SERVER,
    LUA_SCRIPT_TYPE_MODULE
};

struct LuaDataImpl {
    noz::lua::ByteCode byte_code;
    LuaScriptType script_type;
};

struct LuaData : AssetData {
    LuaDataImpl* impl;
};

extern AssetImporter GetLuaImporter();
extern void InitLuaData(AssetData* a);
