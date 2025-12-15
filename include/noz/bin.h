//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Bin : Asset {};

extern u32 GetSize(Bin* bin);
extern const u8* GetData(Bin* bin);

extern Bin** BIN;
//extern int BIN_COUNT;
