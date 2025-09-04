//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "noz/noz.h"

struct AudioSourceHandle
{
    u32 platform_id;
    Sound* sound;
    bool is_valid;
    
    AudioSourceHandle() : platform_id(UINT32_MAX), sound(nullptr), is_valid(false) {}
};

static constexpr u32 MAX_AUDIO_SOURCES = 256;
static AudioSourceHandle g_audio_sources[MAX_AUDIO_SOURCES];
static u32 g_next_handle = 0;

u32 AllocateAudioSourceHandle()
{
    for (u32 i = 0; i < MAX_AUDIO_SOURCES; ++i)
    {
        u32 index = (g_next_handle + i) % MAX_AUDIO_SOURCES;
        if (!g_audio_sources[index].is_valid)
        {
            g_next_handle = (index + 1) % MAX_AUDIO_SOURCES;
            g_audio_sources[index].is_valid = true;
            return index;
        }
    }
    
    LogError("No available audio source handles");
    return UINT32_MAX;
}

void FreeAudioSourceHandle(u32 handle)
{
    if (handle < MAX_AUDIO_SOURCES && g_audio_sources[handle].is_valid)
    {
        g_audio_sources[handle].is_valid = false;
        g_audio_sources[handle].platform_id = UINT32_MAX;
        g_audio_sources[handle].sound = nullptr;
    }
}


u32 CreateAudioSource(Sound* sound)
{
    if (!sound)
    {
        LogError("Cannot create audio source from null sound");
        return UINT32_MAX;
    }
    
    // Allocate handle
    u32 handle = AllocateAudioSourceHandle();
    if (handle == UINT32_MAX)
    {
        return UINT32_MAX;
    }
    
    AudioSourceHandle& source_handle = g_audio_sources[handle];
    
    // Create platform audio source
    u32 platform_id = platform::CreateAudioSource(
        GetSoundData(sound),
        GetSoundDataSize(sound),
        GetSoundSampleRate(sound),
        GetSoundChannels(sound),
        GetSoundBitsPerSample(sound)
    );
    
    if (platform_id == UINT32_MAX)
    {
        FreeAudioSourceHandle(handle);
        LogError("Failed to create platform audio source");
        return UINT32_MAX;
    }
    
    source_handle.platform_id = platform_id;
    source_handle.sound = sound;
    
    return handle;
}

u32 CreateAudioSource(Allocator* allocator, const Name* sound_name)
{
    Sound* sound = GetSound(allocator, sound_name);
    if (!sound)
    {
        LogError("Failed to load sound: %s", GetValue(sound_name));
        return UINT32_MAX;
    }
    
    return CreateAudioSource(sound);
}

void DestroyAudioSource(u32 handle)
{
    if (handle >= MAX_AUDIO_SOURCES || !g_audio_sources[handle].is_valid)
    {
        LogError("Invalid audio source handle: %u", handle);
        return;
    }
    
    AudioSourceHandle& source_handle = g_audio_sources[handle];
    
    if (source_handle.platform_id != UINT32_MAX)
    {
        platform::DestroyAudioSource(source_handle.platform_id);
    }
    
    FreeAudioSourceHandle(handle);
}

bool PlaySound(u32 handle, bool loop)
{
    if (handle >= MAX_AUDIO_SOURCES || !g_audio_sources[handle].is_valid)
    {
        LogError("Invalid audio source handle: %u", handle);
        return false;
    }
    
    AudioSourceHandle& source_handle = g_audio_sources[handle];
    return platform::PlayAudioSource(source_handle.platform_id, loop);
}

bool PlaySound(const Name* sound_name, bool loop)
{
    u32 handle = CreateAudioSource(g_default_allocator, sound_name);
    if (handle == UINT32_MAX)
    {
        return false;
    }
    
    if (!PlaySound(handle, loop))
    {
        DestroyAudioSource(handle);
        return false;
    }
    
    // For one-shot sounds, we could auto-destroy when finished
    // For now, caller is responsible for cleanup
    return true;
}

void StopSound(u32 handle)
{
    if (handle >= MAX_AUDIO_SOURCES || !g_audio_sources[handle].is_valid)
    {
        LogError("Invalid audio source handle: %u", handle);
        return;
    }
    
    AudioSourceHandle& source_handle = g_audio_sources[handle];
    platform::StopAudioSource(source_handle.platform_id);
}

void PauseSound(u32 handle)
{
    if (handle >= MAX_AUDIO_SOURCES || !g_audio_sources[handle].is_valid)
    {
        LogError("Invalid audio source handle: %u", handle);
        return;
    }
    
    AudioSourceHandle& source_handle = g_audio_sources[handle];
    platform::PauseAudioSource(source_handle.platform_id);
}

void ResumeSound(u32 handle)
{
    if (handle >= MAX_AUDIO_SOURCES || !g_audio_sources[handle].is_valid)
    {
        LogError("Invalid audio source handle: %u", handle);
        return;
    }
    
    AudioSourceHandle& source_handle = g_audio_sources[handle];
    platform::ResumeAudioSource(source_handle.platform_id);
}

void SetSoundVolume(u32 handle, float volume)
{
    if (handle >= MAX_AUDIO_SOURCES || !g_audio_sources[handle].is_valid)
    {
        LogError("Invalid audio source handle: %u", handle);
        return;
    }
    
    AudioSourceHandle& source_handle = g_audio_sources[handle];
    platform::SetAudioSourceVolume(source_handle.platform_id, volume);
}

float GetSoundVolume(u32 handle)
{
    if (handle >= MAX_AUDIO_SOURCES || !g_audio_sources[handle].is_valid)
    {
        LogError("Invalid audio source handle: %u", handle);
        return 0.0f;
    }
    
    AudioSourceHandle& source_handle = g_audio_sources[handle];
    return platform::GetAudioSourceVolume(source_handle.platform_id);
}

bool IsSoundPlaying(u32 handle)
{
    if (handle >= MAX_AUDIO_SOURCES || !g_audio_sources[handle].is_valid)
    {
        return false;
    }
    
    AudioSourceHandle& source_handle = g_audio_sources[handle];
    return platform::IsAudioSourcePlaying(source_handle.platform_id);
}

void SetMasterVolume(float volume)
{
    platform::SetMasterVolume(volume);
}

float GetMasterVolume()
{
    return platform::GetMasterVolume();
}

u32 PlaySoundOneShot(const Name* sound_name)
{
    return CreateAudioSource(g_default_allocator, sound_name);
}

u32 PlaySoundLooped(const Name* sound_name)
{
    u32 handle = CreateAudioSource(g_default_allocator, sound_name);
    if (handle != UINT32_MAX)
    {
        PlaySound(handle, true);
    }
    return handle;
}

// Simple play functions that automatically manage sources
void Play(Sound* sound, float volume, float pitch)
{
    if (!sound)
    {
        LogError("Cannot play null sound");
        return;
    }
    
    u32 handle = CreateAudioSource(sound);
    if (handle != UINT32_MAX)
    {
        SetSoundVolume(handle, volume);
        platform::SetAudioSourcePitch(g_audio_sources[handle].platform_id, pitch);
        PlaySound(handle, false);
        
        // Note: For true "fire and forget" behavior, we could track these handles
        // and auto-destroy them when they finish playing. For now, caller is 
        // responsible for cleanup if needed.
    }
}

void Play(const Name* sound_name, float volume, float pitch)
{
    Sound* sound = GetSound(g_default_allocator, sound_name);
    if (sound)
    {
        Play(sound, volume, pitch);
    }
    else
    {
        LogError("Failed to load sound: %s", GetValue(sound_name));
    }
}

void InitAudio()
{
    if (!platform::InitializeAudio())
        Exit("Failed to initialize platform audio");

    for (u32 i = 0; i < MAX_AUDIO_SOURCES; ++i)
        g_audio_sources[i] = AudioSourceHandle();
}

void ShutdownAudio()
{
    // Stop and destroy all active audio sources
    for (u32 i = 0; i < MAX_AUDIO_SOURCES; ++i)
    {
        if (g_audio_sources[i].is_valid)
        {
            if (g_audio_sources[i].platform_id != UINT32_MAX)
            {
                platform::DestroyAudioSource(g_audio_sources[i].platform_id);
            }
            FreeAudioSourceHandle(i);
        }
    }

    // Shutdown platform audio
    platform::ShutdownAudio();

    LogInfo("Audio system shutdown complete");
}
