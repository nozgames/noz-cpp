//
//  NozEd - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

enum class LuaScriptType : u8 {
    Client,
    Server,
    Module
};

struct LuaData : AssetData {
    noz::lua::ByteCode byte_code;
    LuaScriptType script_type;
};

extern AssetImporter GetLuaImporter();
extern void InitLuaData(AssetData* a);
