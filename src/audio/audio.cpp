//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "noz/noz.h"
#include "../platform.h"

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

void InitAudio() {
    platform::InitializeAudio();
}

void ShutdownAudio() {
    platform::ShutdownAudio();
}
