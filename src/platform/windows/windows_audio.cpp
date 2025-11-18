//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <xaudio2.h>
#include <x3daudio.h>

constexpr int MAX_SOURCES = 32;

const WAVEFORMATEX g_wav_format = {
    WAVE_FORMAT_PCM,     // wFormatTag
    1,                   // nChannels (mono)
    44100,              // nSamplesPerSec
    88200,              // nAvgBytesPerSec
    2,                  // nBlockAlign
    16,                 // wBitsPerSample
    0                   // cbSize
};

struct WindowsAudioSource
{
    IXAudio2SourceVoice* voice;
    float volume;
    float pitch;
    u32 generation;
};

struct WindowsAudio
{
    IXAudio2* xaudio2;
    IXAudio2MasteringVoice* mastering_voice;
    X3DAUDIO_HANDLE instance;
    WindowsAudioSource sources[MAX_SOURCES];
    u32 next_source_id = 0;
};

struct WindowsSound
{
    u32 buffer_size;
};

static WindowsAudio g_win_audio = {};

static u32 GetSourceIndex(const platform::SoundHandle& handle)
{
    return (u32)handle.value & 0xFFFFFFFF;
}

static u32 GetSourceGeneration(const platform::SoundHandle& handle)
{
    return (u32)(handle.value >> 32);
}

static platform::SoundHandle MakeSoundHandle(u32 index, u32 generation)
{
    platform::SoundHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

static WindowsAudioSource* GetAudioSource(const platform::SoundHandle& handle)
{
    u32 source_index = GetSourceIndex(handle);
    u32 source_generation = GetSourceGeneration(handle);
    assert(source_index < MAX_SOURCES);

    WindowsAudioSource& source = g_win_audio.sources[source_index];
    if (source.generation != source_generation)
        return nullptr;

    assert(source.voice);

    return &source;
}

void platform::StopSound(const SoundHandle& handle)
{
    WindowsAudioSource* source = GetAudioSource(handle);
    if (!source)
        return;

    source->voice->Stop();
    source->voice->FlushSourceBuffers();
}

void platform::SetSoundVolume(const SoundHandle& handle, float volume)
{
    WindowsAudioSource* source = GetAudioSource(handle);
    if (!source)
        return;

    source->volume = Clamp(volume, 0.0f, 1.0f);
    source->voice->SetVolume(volume);
}

float platform::GetSoundVolume(const SoundHandle& handle)
{
    WindowsAudioSource* source = GetAudioSource(handle);
    if (!source)
        return 1.0f;

    return source->volume;
}

bool platform::IsSoundPlaying(const SoundHandle& handle)
{
    u32 source_index = GetSourceIndex(handle);
    u32 source_generation = GetSourceGeneration(handle);
    assert(source_index < MAX_SOURCES);

    WindowsAudioSource& source = g_win_audio.sources[source_index];
    if (source.generation != source_generation)
        return false;

    assert(source.voice);

    XAUDIO2_VOICE_STATE state;
    source.voice->GetState(&state);
    return state.BuffersQueued > 0;
}

platform::SoundHandle platform::PlaySound(Sound* sound, float volume, float pitch, bool loop)
{
    WindowsSound* wsound = (WindowsSound*)sound;

    for (int i=0; i<MAX_SOURCES; i++)
    {
        WindowsAudioSource& source = g_win_audio.sources[i];
        XAUDIO2_VOICE_STATE state;
        source.voice->GetState(&state);
        if (state.BuffersQueued > 0)
            continue;

        // Found one!
        source.voice->Stop();
        source.voice->FlushSourceBuffers();

        XAUDIO2_BUFFER buffer = {};  // Create a new buffer on the stack
        buffer.AudioBytes = wsound->buffer_size;
        buffer.pAudioData = (const BYTE*)(wsound+1);
        buffer.Flags = XAUDIO2_END_OF_STREAM;
        buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

        source.voice->SubmitSourceBuffer(&buffer);
        source.voice->SetVolume(Clamp(volume, 0.0f, 1.0f));
        source.voice->SetFrequencyRatio(Clamp(pitch, 0.5f, 2.0f));
        source.voice->Start(0);

        return MakeSoundHandle(i, ++source.generation);
    }

    return MakeSoundHandle(0, 0xFFFFFFFF);
}

void platform::SetMasterVolume(float volume)
{
    assert(g_win_audio.mastering_voice);
    g_win_audio.mastering_voice->SetVolume(Clamp(volume, 0.0f, 1.0f));
}

float platform::GetMasterVolume()
{
    assert(g_win_audio.mastering_voice);

    float volume;
    g_win_audio.mastering_voice->GetVolume(&volume);
    return volume;
}

platform::Sound* platform::CreateSound(
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
    assert(sample_rate == g_wav_format.nSamplesPerSec);
    assert(channels == g_wav_format.nChannels);
    assert(bits_per_sample == g_wav_format.wBitsPerSample);

    WindowsSound* sound = (WindowsSound*)Alloc(ALLOCATOR_DEFAULT, sizeof(WindowsSound) + data_size);
    sound->buffer_size = data_size;
    memcpy(sound + 1, data, data_size);
    return (Sound*)sound;
}

void platform::DestroySound(Sound* sound)
{
    Free(sound);
}

/*
 *
*        // Create audio source
source.voice = source_voice;
source.volume = 1.0f;
source.buffer.AudioBytes = 0;
source.buffer.pAudioData = (const BYTE*)nullptr;;
source.buffer.Flags = XAUDIO2_END_OF_STREAM;
source.buffer.LoopCount = 0;
*/


void platform::InitializeAudio()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        LogError("Failed to initialize COM for audio");
        return;
    }

    // XAudio2
    hr = XAudio2Create(&g_win_audio.xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        LogError("Failed to create XAudio2 engine");
        CoUninitialize();
        return;
    }

    // mastering voice
    hr = g_win_audio.xaudio2->CreateMasteringVoice(&g_win_audio.mastering_voice);
    if (FAILED(hr))
    {
        LogError("Failed to create XAudio2 mastering voice");
        g_win_audio.xaudio2->Release();
        g_win_audio.xaudio2 = nullptr;
        CoUninitialize();
        return;
    }

    // Sources
    for (int i=0; i<MAX_SOURCES; i++)
    {
        WindowsAudioSource& source = g_win_audio.sources[i];
        g_win_audio.xaudio2->CreateSourceVoice(&source.voice, &g_wav_format);
    }

    // Initialize 3D audio
    // DWORD channel_mask;
    // g_audio.mastering_voice->GetChannelMask(&channel_mask);
    // X3DAudioInitialize(channel_mask, X3DAUDIO_SPEED_OF_SOUND, g_audio.instance);
}

void platform::ShutdownAudio()
{
    for (int i=0; i<MAX_SOURCES; i++)
    {
        WindowsAudioSource& source = g_win_audio.sources[i];
        if (!source.voice)
            continue;

        source.voice->Stop();
        source.voice->DestroyVoice();
    }

    if (g_win_audio.mastering_voice)
        g_win_audio.mastering_voice->DestroyVoice();

    if (g_win_audio.xaudio2)
        g_win_audio.xaudio2->Release();

    CoUninitialize();

    g_win_audio = {};
}
