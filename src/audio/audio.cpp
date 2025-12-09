//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "noz/noz.h"
#include "../platform.h"

extern void PlayMusicInternal(Sound* sound);

void SetMasterVolume(float volume) {
    PlatformSetMasterVolume(volume);
}

float GetMasterVolume() {
    return PlatformGetMasterVolume();
}

void SetSoundVolume(float volume) {
    PlatformSetSoundVolume(volume);
}

float GetSoundVolume() {
    return PlatformGetSoundVolume();
}

void SetMusicVolume(float volume) {
    PlatformSetMusicVolume(volume);
}

float GetMusicVolume() {
    return PlatformGetMusicVolume();
}

void PlayMusic(Sound* sound) {
    if (IsMusicPlaying())
        StopMusic();

    PlayMusicInternal(sound);
}

void StopMusic() {
    if (!IsMusicPlaying())
        return;

    PlatformStopMusic();
}

bool IsMusicPlaying() {
    return PlatformIsMusicPlaying();
}

void Stop(const SoundHandle& handle) {
    PlatformStopSound({handle.value});
}

void InitAudio() {
    PlatformInitAudio();
}

void ShutdownAudio() {
    StopMusic();
    PlatformShutdownAudio();
}
