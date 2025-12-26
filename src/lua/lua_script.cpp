//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

using namespace noz::lua;

Script** LUA = nullptr;
int LUA_COUNT = 0;

Script* LoadLuaScript(Allocator* allocator, Stream* stream, const Name* name) {
    (void)allocator;
    (void)stream;
    (void)name;

    LuaScriptImpl* impl = static_cast<LuaScriptImpl*>(Alloc(allocator, sizeof(LuaScriptImpl)));
    impl->name = name;
    impl->byte_code.size = ReadU32(stream);
    impl->byte_code.code = static_cast<u8*>(Alloc(allocator, impl->byte_code.size));
    ReadBytes(stream, impl->byte_code.code, impl->byte_code.size);
    return impl;
}

Asset* LoadLuaScript(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)header;
    (void)name_table;
    return LoadLuaScript(allocator, stream, name);
}
