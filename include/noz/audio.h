//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Sound : Asset {};

struct SoundHandle {
    u64 value;
};

extern SoundHandle Play(Sound** sounds, int count, float volume = 1.0f, float pitch = 1.0f, bool loop=false);
extern SoundHandle Play(Sound* sound, float volume = 1.0f, float pitch = 1.0f, bool loop=false);
extern void PlayMusic(Sound* sound);
extern void StopMusic();
extern bool IsMusicPlaying();
extern void SetVolume(const SoundHandle& handle, float volume);
extern void SetPitch(const SoundHandle& handle, float pitch);
extern float GetVolume(const SoundHandle& handle);
extern float GetPitch(const SoundHandle& handle);
extern void Stop(const SoundHandle& handle);
extern bool IsPlaying(const SoundHandle& handle);

extern void SetMasterVolume(float volume);
extern void SetSoundVolume(float volume);
extern void SetMusicVolume(float volume);

extern float GetMasterVolume();
extern float GetSoundVolume();
extern float GetMusicVolume();

extern Sound** SOUND;
extern int SOUND_COUNT;