//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

namespace noz {
    struct Bin : Asset {};

    extern Stream* CreateStream(Allocator* allocator, Bin* bin);
    extern u32 GetSize(Bin* bin);
    extern const u8* GetData(Bin* bin);

    extern Bin** BIN;
}