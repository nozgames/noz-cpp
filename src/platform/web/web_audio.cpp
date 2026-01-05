//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
//  Web/Emscripten audio implementation using Web Audio API
//

#include "../../platform.h"
#include "../../internal.h"

#include <emscripten.h>
#include <emscripten/em_js.h>

// Web-specific sound structure
struct WebSound {
    u32 sound_id;
};

// Audio state
struct WebAudio {
    float master_volume;
    float sound_volume;
    float music_volume;
    bool music_playing;
    u32 next_handle_id;
};

static WebAudio g_web_audio = {};

// Sound handle encoding
static u32 GetSourceIndex(const PlatformSoundHandle& handle) {
    return (u32)handle.value & 0xFFFFFFFF;
}

static u32 GetSourceGeneration(const PlatformSoundHandle& handle) {
    return (u32)(handle.value >> 32);
}

static PlatformSoundHandle MakeSoundHandle(u32 index, u32 generation) {
    PlatformSoundHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

// JavaScript functions using EM_JS
EM_JS(void, js_init_audio, (), {
    if (typeof window.nozAudio === 'undefined') {
        window.nozAudio = {};
        window.nozAudio.context = null;
        window.nozAudio.masterGain = null;
        window.nozAudio.soundGain = null;
        window.nozAudio.musicGain = null;
        window.nozAudio.sounds = {};
        window.nozAudio.musicSource = null;
        window.nozAudio.pendingSounds = [];  // Sounds created before context exists
    }

    // Helper to ensure audio context exists and is running
    window.nozAudio.ensureContext = function() {
        if (!window.nozAudio.context) {
            console.log('[AUDIO] ensureContext: creating new AudioContext');
            window.nozAudio.context = new (window.AudioContext || window.webkitAudioContext)();
            console.log('[AUDIO] ensureContext: context created, state:', window.nozAudio.context.state);

            window.nozAudio.masterGain = window.nozAudio.context.createGain();
            window.nozAudio.soundGain = window.nozAudio.context.createGain();
            window.nozAudio.musicGain = window.nozAudio.context.createGain();

            window.nozAudio.soundGain.connect(window.nozAudio.masterGain);
            window.nozAudio.musicGain.connect(window.nozAudio.masterGain);
            window.nozAudio.masterGain.connect(window.nozAudio.context.destination);

            // Process any sounds that were created before context existed
            console.log('[AUDIO] ensureContext: processing', window.nozAudio.pendingSounds.length, 'pending sounds');
            window.nozAudio.pendingSounds.forEach(function(pending) {
                window.nozAudio.createSoundBuffer(pending.soundId, pending.pcmData, pending.sampleRate, pending.channels, pending.bitsPerSample);
            });
            window.nozAudio.pendingSounds = [];
        }

        if (window.nozAudio.context.state === 'suspended') {
            console.log('[AUDIO] ensureContext: resuming suspended context');
            window.nozAudio.context.resume().then(function() {
                console.log('[AUDIO] ensureContext: context resumed, new state:', window.nozAudio.context.state);
            });
        }

        return window.nozAudio.context;
    };

    // Helper to create sound buffer (called after context exists)
    window.nozAudio.createSoundBuffer = function(soundId, pcmData, sampleRate, channels, bitsPerSample) {
        var bytesPerSample = bitsPerSample / 8;
        var numSamples = Math.floor(pcmData.length / (bytesPerSample * channels));
        var audioBuffer = window.nozAudio.context.createBuffer(channels, numSamples, sampleRate);

        for (var ch = 0; ch < channels; ch++) {
            var channelData = audioBuffer.getChannelData(ch);
            for (var i = 0; i < numSamples; i++) {
                var offset = (i * channels + ch) * bytesPerSample;
                var sample = 0;

                if (bitsPerSample === 8) {
                    sample = (pcmData[offset] - 128) / 128.0;
                } else if (bitsPerSample === 16) {
                    var value = pcmData[offset] | (pcmData[offset + 1] << 8);
                    if (value >= 32768) value -= 65536;
                    sample = value / 32768.0;
                }

                channelData[i] = sample;
            }
        }

        window.nozAudio.sounds[soundId] = {};
        window.nozAudio.sounds[soundId].buffer = audioBuffer;
        window.nozAudio.sounds[soundId].instances = {};
    };

    // Create context on first user interaction and start any pending music
    var initOnGesture = function() {
        window.nozAudio.ensureContext();
        // If there's pending music, start it now that we have a user gesture
        if (window.nozAudio.startMusic) {
            window.nozAudio.startMusic();
        }
    };
    document.addEventListener('click', initOnGesture);
    document.addEventListener('keydown', initOnGesture);
    document.addEventListener('touchstart', initOnGesture);
});

EM_JS(void, js_shutdown_audio, (), {
    if (window.nozAudio && window.nozAudio.context) {
        window.nozAudio.context.close();
        window.nozAudio = undefined;
    }
});

EM_JS(void, js_resume_audio, (), {
    if (window.nozAudio) {
        window.nozAudio.ensureContext();
    }
});

EM_JS(void, js_create_sound, (int soundId, const void* dataPtr, int dataSize, int sampleRate, int channels, int bitsPerSample), {
    if (!window.nozAudio) return;

    // Copy PCM data from WASM memory
    var pcmData = new Uint8Array(dataSize);
    for (var i = 0; i < dataSize; i++) {
        pcmData[i] = HEAPU8[dataPtr + i];
    }

    // If context doesn't exist yet, queue for later
    if (!window.nozAudio.context) {
        window.nozAudio.pendingSounds.push({
            soundId: soundId,
            pcmData: pcmData,
            sampleRate: sampleRate,
            channels: channels,
            bitsPerSample: bitsPerSample
        });
        return;
    }

    // Context exists, create the buffer now
    window.nozAudio.createSoundBuffer(soundId, pcmData, sampleRate, channels, bitsPerSample);
});

EM_JS(void, js_free_sound, (int soundId), {
    if (window.nozAudio && window.nozAudio.sounds[soundId]) {
        var sound = window.nozAudio.sounds[soundId];
        for (var key in sound.instances) {
            try { sound.instances[key].source.stop(); } catch (e) {}
        }
        delete window.nozAudio.sounds[soundId];
    }
});

EM_JS(void, js_play_sound, (int soundId, int instanceId, float volume, float pitch, int loop), {
    if (!window.nozAudio) return;

    // Ensure context exists and is running
    window.nozAudio.ensureContext();

    if (!window.nozAudio.sounds[soundId]) return;

    var sound = window.nozAudio.sounds[soundId];
    var source = window.nozAudio.context.createBufferSource();
    var gainNode = window.nozAudio.context.createGain();

    source.buffer = sound.buffer;
    source.loop = loop ? true : false;
    source.playbackRate.value = pitch;
    gainNode.gain.value = volume;

    source.connect(gainNode);
    gainNode.connect(window.nozAudio.soundGain);
    source.start(0);

    sound.instances[instanceId] = {};
    sound.instances[instanceId].source = source;
    sound.instances[instanceId].gain = gainNode;
    sound.instances[instanceId].volume = volume;
    sound.instances[instanceId].pitch = pitch;

    source.onended = function() {
        delete sound.instances[instanceId];
    };
});

EM_JS(void, js_stop_sound, (int soundId, int instanceId), {
    if (!window.nozAudio || !window.nozAudio.sounds[soundId]) return;
    var sound = window.nozAudio.sounds[soundId];
    if (sound.instances[instanceId]) {
        try { sound.instances[instanceId].source.stop(); } catch (e) {}
        delete sound.instances[instanceId];
    }
});

EM_JS(void, js_set_sound_volume, (int soundId, int instanceId, float volume), {
    if (!window.nozAudio || !window.nozAudio.sounds[soundId]) return;
    var sound = window.nozAudio.sounds[soundId];
    if (sound.instances[instanceId]) {
        sound.instances[instanceId].gain.gain.value = volume;
        sound.instances[instanceId].volume = volume;
    }
});

EM_JS(void, js_set_sound_pitch, (int soundId, int instanceId, float pitch), {
    if (!window.nozAudio || !window.nozAudio.sounds[soundId]) return;
    var sound = window.nozAudio.sounds[soundId];
    if (sound.instances[instanceId]) {
        sound.instances[instanceId].source.playbackRate.value = pitch;
        sound.instances[instanceId].pitch = pitch;
    }
});

EM_JS(int, js_is_sound_playing, (int soundId, int instanceId), {
    if (!window.nozAudio || !window.nozAudio.sounds[soundId]) return 0;
    var sound = window.nozAudio.sounds[soundId];
    return sound.instances[instanceId] ? 1 : 0;
});

EM_JS(float, js_get_sound_volume, (int soundId, int instanceId), {
    if (!window.nozAudio || !window.nozAudio.sounds[soundId]) return 0;
    var sound = window.nozAudio.sounds[soundId];
    if (sound.instances[instanceId]) {
        return sound.instances[instanceId].volume;
    }
    return 0;
});

EM_JS(float, js_get_sound_pitch, (int soundId, int instanceId), {
    if (!window.nozAudio || !window.nozAudio.sounds[soundId]) return 1;
    var sound = window.nozAudio.sounds[soundId];
    if (sound.instances[instanceId]) {
        return sound.instances[instanceId].pitch;
    }
    return 1;
});

EM_JS(void, js_play_music, (int soundId), {
    if (!window.nozAudio) {
        console.warn('[AUDIO] js_play_music: nozAudio not initialized');
        return;
    }

    // Store the pending music to play
    window.nozAudio.pendingMusicId = soundId;

    // Helper function to actually start the music
    window.nozAudio.startMusic = function() {
        var musicId = window.nozAudio.pendingMusicId;
        if (musicId === null || musicId === undefined) return;

        if (!window.nozAudio.sounds[musicId]) {
            console.warn('[AUDIO] startMusic: sound not found:', musicId);
            return;
        }

        // Stop any existing music
        if (window.nozAudio.musicSource) {
            try { window.nozAudio.musicSource.stop(); } catch (e) {}
        }

        var sound = window.nozAudio.sounds[musicId];
        var source = window.nozAudio.context.createBufferSource();

        source.buffer = sound.buffer;
        source.loop = true;
        source.connect(window.nozAudio.musicGain);
        source.start(0);

        window.nozAudio.musicSource = source;
        window.nozAudio.pendingMusicId = null;
        console.log('[AUDIO] startMusic: playing soundId:', musicId);
    };

    // Ensure context exists
    window.nozAudio.ensureContext();

    // If context is running, start immediately
    if (window.nozAudio.context.state === 'running') {
        window.nozAudio.startMusic();
    } else {
        // Context is suspended, wait for it to resume
        console.log('[AUDIO] js_play_music: context suspended, deferring music playback');
        window.nozAudio.context.resume().then(function() {
            console.log('[AUDIO] js_play_music: context resumed, starting music');
            window.nozAudio.startMusic();
        });
    }
});

EM_JS(void, js_stop_music, (), {
    if (!window.nozAudio || !window.nozAudio.musicSource) return;
    try { window.nozAudio.musicSource.stop(); } catch (e) {}
    window.nozAudio.musicSource = null;
});

EM_JS(void, js_set_master_volume, (float volume), {
    if (window.nozAudio && window.nozAudio.masterGain) {
        window.nozAudio.masterGain.gain.value = volume;
    }
});

EM_JS(void, js_set_sound_gain, (float volume), {
    if (window.nozAudio && window.nozAudio.soundGain) {
        window.nozAudio.soundGain.gain.value = volume;
    }
});

EM_JS(void, js_set_music_gain, (float volume), {
    if (window.nozAudio && window.nozAudio.musicGain) {
        window.nozAudio.musicGain.gain.value = volume;
    }
});

// Platform API implementations

void PlatformInitAudio() {
    g_web_audio = {};
    g_web_audio.master_volume = 1.0f;
    g_web_audio.sound_volume = 1.0f;
    g_web_audio.music_volume = 1.0f;
    g_web_audio.next_handle_id = 1;

    js_init_audio();
}

void PlatformShutdownAudio() {
    js_shutdown_audio();
}

PlatformSound* PlatformCreateSound(void* data, u32 data_size, u32 sample_rate, u32 channels, u32 bits_per_sample) {
    if (!data || data_size == 0) {
        return nullptr;
    }

    WebSound* sound = new WebSound();
    u32 sound_id = g_web_audio.next_handle_id++;

    js_create_sound(sound_id, data, data_size, sample_rate, channels, bits_per_sample);

    sound->sound_id = sound_id;
    return (PlatformSound*)sound;
}

void PlatformFree(PlatformSound* sound) {
    if (!sound) return;

    WebSound* wsound = (WebSound*)sound;
    js_free_sound(wsound->sound_id);
    delete wsound;
}

PlatformSoundHandle PlatformPlaySound(PlatformSound* sound, float volume, float pitch, bool loop) {
    if (!sound) {
        return PlatformSoundHandle{0};
    }

    WebSound* wsound = (WebSound*)sound;
    u32 sound_id = wsound->sound_id;
    u32 instance_id = g_web_audio.next_handle_id++;

    js_play_sound(sound_id, instance_id, volume, pitch, loop ? 1 : 0);

    return MakeSoundHandle(instance_id, sound_id);
}

void PlatformStopSound(const PlatformSoundHandle& handle) {
    if (handle.value == 0) return;

    u32 instance_id = GetSourceIndex(handle);
    u32 sound_id = GetSourceGeneration(handle);

    js_stop_sound(sound_id, instance_id);
}

void PlatformSetSoundVolume(const PlatformSoundHandle& handle, float volume) {
    if (handle.value == 0) return;

    u32 instance_id = GetSourceIndex(handle);
    u32 sound_id = GetSourceGeneration(handle);

    js_set_sound_volume(sound_id, instance_id, volume);
}

void PlatformSetSoundPitch(const PlatformSoundHandle& handle, float pitch) {
    if (handle.value == 0) return;

    u32 instance_id = GetSourceIndex(handle);
    u32 sound_id = GetSourceGeneration(handle);

    js_set_sound_pitch(sound_id, instance_id, pitch);
}

bool PlatformIsSoundPlaying(const PlatformSoundHandle& handle) {
    if (handle.value == 0) return false;

    u32 instance_id = GetSourceIndex(handle);
    u32 sound_id = GetSourceGeneration(handle);

    return js_is_sound_playing(sound_id, instance_id) != 0;
}

float PlatformGetSoundVolume(const PlatformSoundHandle& handle) {
    if (handle.value == 0) return 0.0f;

    u32 instance_id = GetSourceIndex(handle);
    u32 sound_id = GetSourceGeneration(handle);

    return js_get_sound_volume(sound_id, instance_id);
}

float PlatformGetSoundPitch(const PlatformSoundHandle& handle) {
    if (handle.value == 0) return 1.0f;

    u32 instance_id = GetSourceIndex(handle);
    u32 sound_id = GetSourceGeneration(handle);

    return js_get_sound_pitch(sound_id, instance_id);
}

void PlatformPlayMusic(PlatformSound* sound) {
    if (!sound) return;

    PlatformStopMusic();
    g_web_audio.music_playing = true;

    WebSound* wsound = (WebSound*)sound;
    js_play_music(wsound->sound_id);
}

bool PlatformIsMusicPlaying() {
    return g_web_audio.music_playing;
}

void PlatformStopMusic() {
    g_web_audio.music_playing = false;
    js_stop_music();
}

void PlatformSetMasterVolume(float volume) {
    g_web_audio.master_volume = Clamp(volume, 0.0f, 1.0f);
    js_set_master_volume(g_web_audio.master_volume);
}

void PlatformSetSoundVolume(float volume) {
    g_web_audio.sound_volume = Clamp(volume, 0.0f, 1.0f);
    js_set_sound_gain(g_web_audio.sound_volume);
}

void PlatformSetMusicVolume(float volume) {
    g_web_audio.music_volume = Clamp(volume, 0.0f, 1.0f);
    js_set_music_gain(g_web_audio.music_volume);
}

float PlatformGetMasterVolume() {
    return g_web_audio.master_volume;
}

float PlatformGetSoundVolume() {
    return g_web_audio.sound_volume;
}

float PlatformGetMusicVolume() {
    return g_web_audio.music_volume;
}
