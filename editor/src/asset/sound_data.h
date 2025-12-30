//
//  NozEd - Copyright(c) 2025 NoZ Games, LLC
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
