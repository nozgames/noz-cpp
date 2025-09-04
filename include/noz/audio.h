//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// Forward declarations
struct Sound;

// Sound loading and management
Sound* GetSound(Allocator* allocator, const Name* name);
Asset* LoadSound(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name);

u32 GetSoundSampleRate(Sound* sound);
u32 GetSoundChannels(Sound* sound);
u32 GetSoundBitsPerSample(Sound* sound);
u32 GetSoundDataSize(Sound* sound);
void* GetSoundData(Sound* sound);

// Sound importing
bool ImportWavFile(const char* input_path, const char* output_path);

// Audio source management
u32 CreateAudioSource(Sound* sound);
u32 CreateAudioSource(Allocator* allocator, const Name* sound_name);
void DestroyAudioSource(u32 handle);

// Sound playback control
bool PlaySound(u32 handle, bool loop = false);
bool PlaySound(const Name* sound_name, bool loop = false);
void StopSound(u32 handle);
void PauseSound(u32 handle);
void ResumeSound(u32 handle);

// Sound properties
void SetSoundVolume(u32 handle, float volume);
float GetSoundVolume(u32 handle);
bool IsSoundPlaying(u32 handle);

// Global audio control
void SetMasterVolume(float volume);
float GetMasterVolume();

// Convenience functions
u32 PlaySoundOneShot(const Name* sound_name);
u32 PlaySoundLooped(const Name* sound_name);

// Simple play function - automatically manages sources
void Play(Sound* sound, float volume = 1.0f, float pitch = 1.0f);
void Play(const Name* sound_name, float volume = 1.0f, float pitch = 1.0f);

// Platform audio interface
namespace platform
{
    bool InitializeAudio();
    void ShutdownAudio();
    
    u32 CreateAudioSource(void* audio_data, u32 data_size, u32 sample_rate, u32 channels, u32 bits_per_sample);
    void DestroyAudioSource(u32 source_id);
    
    bool PlayAudioSource(u32 source_id, bool loop);
    void StopAudioSource(u32 source_id);
    void PauseAudioSource(u32 source_id);
    void ResumeAudioSource(u32 source_id);
    
    void SetAudioSourceVolume(u32 source_id, float volume);
    void SetAudioSourcePitch(u32 source_id, float pitch);
    float GetAudioSourceVolume(u32 source_id);
    bool IsAudioSourcePlaying(u32 source_id);
    
    void SetMasterVolume(float volume);
    float GetMasterVolume();
}
