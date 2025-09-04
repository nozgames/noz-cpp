//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <windows.h>
#include <xaudio2.h>
#include <x3daudio.h>
#include <vector>

#include "noz/noz.h"

static IXAudio2* g_xaudio2 = nullptr;
static IXAudio2MasteringVoice* g_mastering_voice = nullptr;
static X3DAUDIO_HANDLE g_x3d_instance = {};

struct AudioSource
{
    IXAudio2SourceVoice* voice;
    XAUDIO2_BUFFER buffer;
    bool is_playing;
    bool is_looping;
    float volume;
    
    AudioSource() : voice(nullptr), is_playing(false), is_looping(false), volume(1.0f)
    {
        memset(&buffer, 0, sizeof(buffer));
    }
};

static std::vector<AudioSource> g_audio_sources;
static u32 g_next_source_id = 0;

namespace platform
{
    bool InitializeAudio()
    {
        HRESULT hr;
        
        // Initialize COM
        hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr))
        {
            LogError("Failed to initialize COM for audio");
            return false;
        }
        
        // Create XAudio2 engine
        hr = XAudio2Create(&g_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr))
        {
            LogError("Failed to create XAudio2 engine");
            CoUninitialize();
            return false;
        }
        
        // Create mastering voice
        hr = g_xaudio2->CreateMasteringVoice(&g_mastering_voice);
        if (FAILED(hr))
        {
            LogError("Failed to create XAudio2 mastering voice");
            g_xaudio2->Release();
            g_xaudio2 = nullptr;
            CoUninitialize();
            return false;
        }
        
        // Initialize 3D audio
        DWORD channel_mask;
        g_mastering_voice->GetChannelMask(&channel_mask);
        X3DAudioInitialize(channel_mask, X3DAUDIO_SPEED_OF_SOUND, g_x3d_instance);
        
        LogInfo("XAudio2 initialized successfully");
        return true;
    }
    
    void ShutdownAudio()
    {
        // Stop and destroy all source voices
        for (AudioSource& source : g_audio_sources)
        {
            if (source.voice)
            {
                source.voice->Stop();
                source.voice->DestroyVoice();
                source.voice = nullptr;
            }
        }
        g_audio_sources.clear();
        
        // Destroy mastering voice
        if (g_mastering_voice)
        {
            g_mastering_voice->DestroyVoice();
            g_mastering_voice = nullptr;
        }
        
        // Release XAudio2
        if (g_xaudio2)
        {
            g_xaudio2->Release();
            g_xaudio2 = nullptr;
        }
        
        // Uninitialize COM
        CoUninitialize();
        
        LogInfo("XAudio2 shutdown complete");
    }
    
    u32 CreateAudioSource(void* audio_data, u32 data_size, u32 sample_rate, u32 channels, u32 bits_per_sample)
    {
        if (!g_xaudio2 || !audio_data || data_size == 0)
        {
            LogError("Invalid parameters for CreateAudioSource");
            return UINT32_MAX;
        }
        
        // Setup wave format
        WAVEFORMATEX wave_format = {};
        wave_format.wFormatTag = WAVE_FORMAT_PCM;
        wave_format.nChannels = (WORD)channels;
        wave_format.nSamplesPerSec = sample_rate;
        wave_format.wBitsPerSample = (WORD)bits_per_sample;
        wave_format.nBlockAlign = (WORD)(channels * bits_per_sample / 8);
        wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
        wave_format.cbSize = 0;
        
        // Create source voice
        IXAudio2SourceVoice* source_voice = nullptr;
        HRESULT hr = g_xaudio2->CreateSourceVoice(&source_voice, &wave_format);
        if (FAILED(hr))
        {
            LogError("Failed to create XAudio2 source voice");
            return UINT32_MAX;
        }
        
        // Create audio source
        g_audio_sources.emplace_back();
        AudioSource& source = g_audio_sources.back();
        source.voice = source_voice;
        source.volume = 1.0f;
        
        // Setup buffer
        source.buffer.AudioBytes = data_size;
        source.buffer.pAudioData = (const BYTE*)audio_data;
        source.buffer.Flags = XAUDIO2_END_OF_STREAM;
        source.buffer.LoopCount = 0;
        
        u32 source_id = g_next_source_id++;
        LogInfo("Created audio source %u (rate: %u Hz, channels: %u, bits: %u)", 
                source_id, sample_rate, channels, bits_per_sample);
        
        return source_id;
    }
    
    void DestroyAudioSource(u32 source_id)
    {
        if (source_id >= g_audio_sources.size())
        {
            LogError("Invalid audio source ID: %u", source_id);
            return;
        }
        
        AudioSource& source = g_audio_sources[source_id];
        if (source.voice)
        {
            source.voice->Stop();
            source.voice->DestroyVoice();
            source.voice = nullptr;
        }
        
        LogInfo("Destroyed audio source %u", source_id);
    }
    
    bool PlayAudioSource(u32 source_id, bool loop)
    {
        if (source_id >= g_audio_sources.size())
        {
            LogError("Invalid audio source ID: %u", source_id);
            return false;
        }
        
        AudioSource& source = g_audio_sources[source_id];
        if (!source.voice)
        {
            LogError("Audio source %u has no voice", source_id);
            return false;
        }
        
        // Update loop settings
        source.is_looping = loop;
        source.buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
        
        // Stop current playback if any
        source.voice->Stop();
        source.voice->FlushSourceBuffers();
        
        // Submit buffer and start playback
        HRESULT hr = source.voice->SubmitSourceBuffer(&source.buffer);
        if (FAILED(hr))
        {
            LogError("Failed to submit audio buffer for source %u", source_id);
            return false;
        }
        
        hr = source.voice->Start();
        if (FAILED(hr))
        {
            LogError("Failed to start audio source %u", source_id);
            return false;
        }
        
        source.is_playing = true;
        return true;
    }
    
    void StopAudioSource(u32 source_id)
    {
        if (source_id >= g_audio_sources.size())
        {
            LogError("Invalid audio source ID: %u", source_id);
            return;
        }
        
        AudioSource& source = g_audio_sources[source_id];
        if (source.voice)
        {
            source.voice->Stop();
            source.voice->FlushSourceBuffers();
            source.is_playing = false;
        }
    }
    
    void PauseAudioSource(u32 source_id)
    {
        if (source_id >= g_audio_sources.size())
        {
            LogError("Invalid audio source ID: %u", source_id);
            return;
        }
        
        AudioSource& source = g_audio_sources[source_id];
        if (source.voice && source.is_playing)
        {
            source.voice->Stop();
        }
    }
    
    void ResumeAudioSource(u32 source_id)
    {
        if (source_id >= g_audio_sources.size())
        {
            LogError("Invalid audio source ID: %u", source_id);
            return;
        }
        
        AudioSource& source = g_audio_sources[source_id];
        if (source.voice && source.is_playing)
        {
            source.voice->Start();
        }
    }
    
    void SetAudioSourceVolume(u32 source_id, float volume)
    {
        if (source_id >= g_audio_sources.size())
        {
            LogError("Invalid audio source ID: %u", source_id);
            return;
        }
        
        volume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
        
        AudioSource& source = g_audio_sources[source_id];
        source.volume = volume;
        
        if (source.voice)
        {
            source.voice->SetVolume(volume);
        }
    }
    
    void SetAudioSourcePitch(u32 source_id, float pitch)
    {
        if (source_id >= g_audio_sources.size())
        {
            LogError("Invalid audio source ID: %u", source_id);
            return;
        }
        
        // Clamp pitch to XAudio2 limits (0.5 to 2.0 for frequency ratio)
        pitch = pitch < 0.5f ? 0.5f : (pitch > 2.0f ? 2.0f : pitch);
        
        AudioSource& source = g_audio_sources[source_id];
        if (source.voice)
        {
            source.voice->SetFrequencyRatio(pitch);
        }
    }
    
    float GetAudioSourceVolume(u32 source_id)
    {
        if (source_id >= g_audio_sources.size())
        {
            LogError("Invalid audio source ID: %u", source_id);
            return 0.0f;
        }
        
        return g_audio_sources[source_id].volume;
    }
    
    bool IsAudioSourcePlaying(u32 source_id)
    {
        if (source_id >= g_audio_sources.size())
        {
            return false;
        }
        
        AudioSource& source = g_audio_sources[source_id];
        if (!source.voice || !source.is_playing)
        {
            return false;
        }
        
        // Check if voice is actually playing
        XAUDIO2_VOICE_STATE state;
        source.voice->GetState(&state);
        
        // If no buffers are queued, the source has finished playing
        if (state.BuffersQueued == 0)
        {
            source.is_playing = false;
            return false;
        }
        
        return true;
    }
    
    void SetMasterVolume(float volume)
    {
        volume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
        
        if (g_mastering_voice)
        {
            g_mastering_voice->SetVolume(volume);
        }
    }
    
    float GetMasterVolume()
    {
        if (g_mastering_voice)
        {
            float volume;
            g_mastering_voice->GetVolume(&volume);
            return volume;
        }
        
        return 0.0f;
    }
}