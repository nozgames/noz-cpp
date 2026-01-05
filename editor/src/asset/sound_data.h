//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

struct SoundDataImpl {
    SoundHandle handle;
    Sound* sound;
};

struct SoundData : AssetData {
    SoundDataImpl* impl;
};

extern void InitSoundData(AssetData* a);
