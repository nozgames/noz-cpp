//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../platform.h"

namespace noz {

    Sound** SOUND = nullptr;
    int SOUND_COUNT = 0;

    struct SoundHeader {
        u32 sample_rate;
        u32 channels;
        u32 bits_per_sample;
        u32 data_size;
    };

    struct SoundImpl : Sound {
        SoundHeader header;
        PlatformSound* platform;
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
        sound->platform = PlatformCreateSound(
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
        return PlatformIsSoundPlaying({handle.value});
    }

    float GetSoundVolume(const SoundHandle& handle)
    {
        return PlatformGetSoundVolume({handle.value});
    }

    float GetSoundPitch(const SoundHandle& handle)
    {
        return PlatformGetSoundVolume({handle.value});
    }

    void SetSoundVolume(const SoundHandle& handle, float volume)
    {
        PlatformSetSoundVolume({handle.value}, volume);
    }

    SoundHandle Play(Sound** sounds, int count, float volume, float pitch, bool loop) {
        return Play(sounds[RandomInt(0, count - 1)], volume, pitch, loop);
    }

    SoundHandle Play(Sound* sound, float volume, float pitch, bool loop) {
        SoundImpl* impl = (SoundImpl*)sound;
        PlatformSoundHandle handle = PlatformPlaySound(impl->platform, volume, pitch, loop);
        return { handle.value };
    }

    void PlayMusicInternal(Sound* sound) {
        if (!sound) {
            LogWarning("[AUDIO] PlayMusicInternal: sound is null");
            return;
        }
        SoundImpl* impl = static_cast<SoundImpl*>(sound);
        if (!impl->platform) {
            LogWarning("[AUDIO] PlayMusicInternal: platform sound is null for '%s'", sound->name ? sound->name->value : "unnamed");
            return;
        }
        PlatformPlayMusic(impl->platform);
    }
}
