//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "noz/noz.h"

struct SoundHeader
{
    u32 sample_rate;
    u32 channels;
    u32 bits_per_sample;
    u32 data_size;
};

struct Sound : Asset
{
    SoundHeader header;
    void* data;
    
    Sound() = default;
    ~Sound() = default;
};

Sound* GetSound(Allocator* allocator, const Name* name)
{
    return (Sound*)LoadAsset(allocator, name, ASSET_SIGNATURE_SOUND, LoadSound);
}

Asset* LoadSound(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    assert(stream);
    assert(name);
    assert(header);

    Sound* sound = (Sound*)Alloc(allocator, sizeof(Sound));
    sound->name = name;
    sound->header.sample_rate = ReadU32(stream);
    sound->header.channels = ReadU32(stream);
    sound->header.bits_per_sample = ReadU32(stream);
    sound->header.data_size = ReadU32(stream);
    sound->data = Alloc(allocator, sound->header.data_size);
    ReadBytes(stream, sound->data, sound->header.data_size);

    return sound;
}

u32 GetSoundSampleRate(Sound* sound)
{
    return sound ? sound->header.sample_rate : 0;
}

u32 GetSoundChannels(Sound* sound)
{
    return sound ? sound->header.channels : 0;
}

u32 GetSoundBitsPerSample(Sound* sound)
{
    return sound ? sound->header.bits_per_sample : 0;
}

u32 GetSoundDataSize(Sound* sound)
{
    return sound ? sound->header.data_size : 0;
}

void* GetSoundData(Sound* sound)
{
    return sound ? sound->data : nullptr;
}