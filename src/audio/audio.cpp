//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "noz/noz.h"
#include "../platform.h"

extern void PlayMusicInternal(Sound* sound);

void SetMasterVolume(float volume) {
    platform::SetMasterVolume(volume);
}

float GetMasterVolume() {
    return platform::GetMasterVolume();
}

void SetSoundVolume(float volume) {
    platform::SetSoundVolume(volume);
}

float GetSoundVolume() {
    return platform::GetSoundVolume();
}

void SetMusicVolume(float volume) {
    platform::SetMusicVolume(volume);
}

float GetMusicVolume() {
    return platform::GetMusicVolume();
}

void PlayMusic(Sound* sound) {
    if (IsMusicPlaying())
        StopMusic();

    PlayMusicInternal(sound);
}

void StopMusic() {
    if (!IsMusicPlaying())
        return;

    platform::StopMusic();
}

bool IsMusicPlaying() {
    return platform::IsMusicPlaying();
}

void Stop(const SoundHandle& handle) {
    platform::StopSound({handle.value});
}

void InitAudio() {
    platform::InitializeAudio();
}

void ShutdownAudio() {
    StopMusic();
    platform::ShutdownAudio();
}
