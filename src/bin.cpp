//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

Bin** BIN = nullptr;
int BIN_COUNT = 0;

struct BinImpl : Bin {
    u32 length;
    u8* data;
};

Asset* LoadBin(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)name_table;
    (void)header;
    (void)name;

    assert(stream);
    assert(name);
    assert(header);

    u32 data_size = ReadU32(stream);

    BinImpl* impl = static_cast<BinImpl*>(Alloc(allocator, sizeof(BinImpl) + data_size));
    impl->data = reinterpret_cast<u8 *>(impl + 1);
    impl->length = data_size;
    ReadBytes(stream, impl->data, data_size);

    return impl;
}

u32 GetSize(Bin* bin) {
    BinImpl* impl = static_cast<BinImpl*>(bin);
    return impl->length;
}

const u8* GetData(Bin* bin) {
    BinImpl* impl = static_cast<BinImpl*>(bin);
    return impl->data;
}

Stream* CreateStream(Allocator* allocator, Bin* bin) {
    BinImpl* impl = static_cast<BinImpl*>(bin);
    return CreateStream(allocator, impl->data, impl->length);
}
