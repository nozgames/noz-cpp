//
//  NozEd - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct LuaData : AssetData {
    noz::lua::ByteCode byte_code;
};

extern AssetImporter GetLuaImporter();
extern void InitLuaData(AssetData* a);
