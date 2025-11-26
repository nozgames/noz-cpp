//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"
#include "noz/noz.h"

Sound** SOUND = nullptr;

struct SoundHeader {
    u32 sample_rate;
    u32 channels;
    u32 bits_per_sample;
    u32 data_size;
};

struct SoundImpl : Sound {
    SoundHeader header;
    platform::Sound* platform;
};


Asset* LoadSound(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table) {
    (void)name_table;
    (void)header;

    assert(stream);
    assert(name);
    assert(header);

    SoundImpl* sound = (SoundImpl*)Alloc(allocator, sizeof(SoundImpl));
    sound->name = name;
    sound->header.sample_rate = ReadU32(stream);
    sound->header.channels = ReadU32(stream);
    sound->header.bits_per_sample = ReadU32(stream);
    sound->header.data_size = ReadU32(stream);

    void* data = Alloc(ALLOCATOR_SCRATCH, sound->header.data_size);
    ReadBytes(stream, data, sound->header.data_size);
    sound->platform = platform::CreateSound(
        data,
        sound->header.data_size,
        sound->header.sample_rate,
        sound->header.channels,
        sound->header.bits_per_sample);
    Free(data);

    return sound;
}

bool IsPlaying(const SoundHandle& handle)
{
    return platform::IsSoundPlaying({handle.value});
}

float GetSoundVolume(const SoundHandle& handle)
{
    return platform::GetSoundVolume({handle.value});
}

float GetSoundPitch(const SoundHandle& handle)
{
    return platform::GetSoundVolume({handle.value});
}

void SetSoundVolume(const SoundHandle& handle, float volume)
{
    platform::SetSoundVolume({handle.value}, volume);
}

SoundHandle Play(Sound** sounds, int count, float volume, float pitch, bool loop) {
    return Play(sounds[RandomInt(0, count - 1)], volume, pitch, loop);
}

SoundHandle Play(Sound* sound, float volume, float pitch, bool loop) {
    SoundImpl* impl = (SoundImpl*)sound;
    platform::SoundHandle handle = platform::PlaySound(impl->platform, volume, pitch, loop);
    return { handle.value };
}

void PlayMusicInternal(Sound* sound) {
    platform::PlayMusic(static_cast<SoundImpl*>(sound)->platform);
}
