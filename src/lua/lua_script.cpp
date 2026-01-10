//
//  NoZ Game Engine - Copyright(c) 2026 NoZ Games, LLC
//

using namespace noz::lua;

namespace noz {
    Script** LUA = nullptr;
    int LUA_COUNT = 0;

    Script* LoadLuaScript(Allocator* allocator, Stream* stream, const Name* name) {
        (void)allocator;
        (void)stream;
        (void)name;

        ScriptImpl* impl = static_cast<ScriptImpl*>(Alloc(allocator, sizeof(ScriptImpl)));
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

    void ReloadLuaScript(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table) {
        (void)header;
        (void)name_table;

        assert(asset);
        assert(stream);
        ScriptImpl* impl = static_cast<ScriptImpl*>(asset);
        Free(impl->byte_code.code);

        impl->byte_code.size = ReadU32(stream);
        impl->byte_code.code = static_cast<u8*>(Alloc(ALLOCATOR_DEFAULT, impl->byte_code.size));
        ReadBytes(stream, impl->byte_code.code, impl->byte_code.size);
    }
}
