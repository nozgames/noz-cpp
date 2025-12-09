//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include <AudioToolbox/AudioToolbox.h>
#include <vector>

constexpr int MAX_SOURCES = 32;
constexpr int SAMPLE_RATE = 44100;
constexpr int CHANNELS = 1;
constexpr int BITS_PER_SAMPLE = 16;

struct MacOSAudioSource
{
    AudioQueueRef queue;
    AudioQueueBufferRef buffer;
    bool is_playing;
    bool is_looping;
    float volume;
    float pitch;
    u32 generation;
    const i16* audio_data;
    u32 audio_data_size;
    u32 playback_position;
};

struct MacOSAudio
{
    MacOSAudioSource sources[MAX_SOURCES];
    u32 next_source_id;
    float master_volume;
};

struct MacOSSound
{
    u32 buffer_size;
};

static MacOSAudio g_mac_audio = {};

static u32 GetSourceIndex(const SoundHandle& handle)
{
    return (u32)handle.value & 0xFFFFFFFF;
}

static u32 GetSourceGeneration(const SoundHandle& handle)
{
    return (u32)(handle.value >> 32);
}

static SoundHandle MakeSoundHandle(u32 index, u32 generation)
{
    SoundHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

static MacOSAudioSource* GetAudioSource(const SoundHandle& handle)
{
    u32 source_index = GetSourceIndex(handle);
    u32 source_generation = GetSourceGeneration(handle);
    assert(source_index < MAX_SOURCES);

    MacOSAudioSource& source = g_mac_audio.sources[source_index];
    if (source.generation != source_generation)
        return nullptr;

    return &source;
}

static void AudioQueueOutputCallbackImpl(void* userData, AudioQueueRef queue, AudioQueueBufferRef buffer)
{
    MacOSAudioSource* source = (MacOSAudioSource*)userData;
    if (!source || !source->is_playing)
        return;

    u32 samples_to_copy = buffer->mAudioDataBytesCapacity / sizeof(i16);
    u32 samples_remaining = (source->audio_data_size / sizeof(i16)) - source->playback_position;

    if (samples_remaining == 0) {
        if (source->is_looping) {
            source->playback_position = 0;
            samples_remaining = source->audio_data_size / sizeof(i16);
        } else {
            source->is_playing = false;
            return;
        }
    }

    u32 samples_copied = Min(samples_to_copy, samples_remaining);
    memcpy(buffer->mAudioData, &source->audio_data[source->playback_position], samples_copied * sizeof(i16));
    buffer->mAudioDataByteSize = samples_copied * sizeof(i16);
    source->playback_position += samples_copied;

    AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
}

void StopSound(const SoundHandle& handle)
{
    MacOSAudioSource* source = GetAudioSource(handle);
    if (!source || !source->queue)
        return;

    AudioQueueStop(source->queue, true);
    source->is_playing = false;
    source->playback_position = 0;
}

void SetSoundVolume(const SoundHandle& handle, float volume)
{
    MacOSAudioSource* source = GetAudioSource(handle);
    if (!source || !source->queue)
        return;

    source->volume = Clamp(volume, 0.0f, 1.0f);
    AudioQueueSetParameter(source->queue, kAudioQueueParam_Volume, source->volume * g_mac_audio.master_volume);
}

float GetSoundVolume(const SoundHandle& handle)
{
    MacOSAudioSource* source = GetAudioSource(handle);
    if (!source)
        return 1.0f;

    return source->volume;
}

bool IsSoundPlaying(const SoundHandle& handle)
{
    MacOSAudioSource* source = GetAudioSource(handle);
    if (!source)
        return false;

    return source->is_playing;
}

SoundHandle PlaySound(Sound* sound, float volume, float pitch, bool loop)
{
    MacOSSound* msound = (MacOSSound*)sound;

    // Find an available source
    for (int i = 0; i < MAX_SOURCES; i++)
    {
        MacOSAudioSource& source = g_mac_audio.sources[i];
        if (source.is_playing)
            continue;

        // Stop and clean up existing queue if any
        if (source.queue) {
            AudioQueueStop(source.queue, true);
            if (source.buffer) {
                AudioQueueFreeBuffer(source.queue, source.buffer);
                source.buffer = nullptr;
            }
            AudioQueueDispose(source.queue, true);
            source.queue = nullptr;
        }

        // Set up audio format
        AudioStreamBasicDescription format = {};
        format.mSampleRate = SAMPLE_RATE;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
        format.mBitsPerChannel = BITS_PER_SAMPLE;
        format.mChannelsPerFrame = CHANNELS;
        format.mBytesPerFrame = (BITS_PER_SAMPLE / 8) * CHANNELS;
        format.mFramesPerPacket = 1;
        format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;

        // Create audio queue
        OSStatus status = AudioQueueNewOutput(
            &format,
            AudioQueueOutputCallbackImpl,
            &source,
            nullptr,
            nullptr,
            0,
            &source.queue
        );

        if (status != noErr || !source.queue)
            continue;

        // Allocate buffer
        status = AudioQueueAllocateBuffer(source.queue, msound->buffer_size, &source.buffer);
        if (status != noErr || !source.buffer) {
            AudioQueueDispose(source.queue, true);
            source.queue = nullptr;
            continue;
        }

        // Set up source
        source.audio_data = (const i16*)(msound + 1);
        source.audio_data_size = msound->buffer_size;
        source.playback_position = 0;
        source.is_playing = true;
        source.is_looping = loop;
        source.volume = Clamp(volume, 0.0f, 1.0f);
        source.pitch = Clamp(pitch, 0.5f, 2.0f);

        // Set volume and pitch
        AudioQueueSetParameter(source.queue, kAudioQueueParam_Volume, source.volume * g_mac_audio.master_volume);
        AudioQueueSetParameter(source.queue, kAudioQueueParam_PlayRate, source.pitch);

        // Prime the pump with initial buffer
        AudioQueueOutputCallbackImpl(&source, source.queue, source.buffer);

        // Start playback
        AudioQueueStart(source.queue, nullptr);

        return MakeSoundHandle(i, ++source.generation);
    }

    return MakeSoundHandle(0, 0xFFFFFFFF);
}

void SetMasterVolume(float volume)
{
    g_mac_audio.master_volume = Clamp(volume, 0.0f, 1.0f);

    // Update all active sources
    for (int i = 0; i < MAX_SOURCES; i++)
    {
        MacOSAudioSource& source = g_mac_audio.sources[i];
        if (source.queue) {
            AudioQueueSetParameter(source.queue, kAudioQueueParam_Volume, source.volume * g_mac_audio.master_volume);
        }
    }
}

float GetMasterVolume()
{
    return g_mac_audio.master_volume;
}

Sound* CreateSound(
    void* data,
    u32 data_size,
    u32 sample_rate,
    u32 channels,
    u32 bits_per_sample)
{
    (void)sample_rate;
    (void)channels;
    (void)bits_per_sample;

    assert(data);
    assert(data_size > 0);
    assert(sample_rate == SAMPLE_RATE);
    assert(channels == CHANNELS);
    assert(bits_per_sample == BITS_PER_SAMPLE);

    MacOSSound* sound = (MacOSSound*)Alloc(ALLOCATOR_DEFAULT, sizeof(MacOSSound) + data_size);
    sound->buffer_size = data_size;
    memcpy(sound + 1, data, data_size);
    return (Sound*)sound;
}

void DestroySound(Sound* sound)
{
    Free(sound);
}

void InitializeAudio()
{
    g_mac_audio = {};
    g_mac_audio.master_volume = 1.0f;

    // Initialize all sources
    for (int i = 0; i < MAX_SOURCES; i++)
    {
        MacOSAudioSource& source = g_mac_audio.sources[i];
        source.queue = nullptr;
        source.buffer = nullptr;
        source.is_playing = false;
        source.volume = 1.0f;
        source.pitch = 1.0f;
        source.generation = 0;
    }
}

void ShutdownAudio()
{
    // Stop and dispose all sources
    for (int i = 0; i < MAX_SOURCES; i++)
    {
        MacOSAudioSource& source = g_mac_audio.sources[i];
        if (source.queue) {
            AudioQueueStop(source.queue, true);
            if (source.buffer) {
                AudioQueueFreeBuffer(source.queue, source.buffer);
            }
            AudioQueueDispose(source.queue, true);
        }
    }

    g_mac_audio = {};
}
