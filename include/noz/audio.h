//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Sound : Asset {};

struct SoundHandle
{
    u64 value;
};

SoundHandle Play(Sound* sound, float volume = 1.0f, float pitch = 1.0f, bool loop=false);

void SetVolume(const SoundHandle& handle, float volume);
void SetPitch(const SoundHandle& handle, float pitch);
float GetVolume(const SoundHandle& handle);
float GetPitch(const SoundHandle& handle);
void Stop(const SoundHandle& handle);
bool IsPlaying(const SoundHandle& handle);

void SetMasterVolume(float volume);
float GetMasterVolume();

