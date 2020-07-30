///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AudioClip_h__
#define __noz_AudioClip_h__

#include <noz/Platform/AudioClipHandle.h>

namespace noz { namespace Editor { class AudioClipFile; } }

namespace noz {

  class AudioClip : public Asset {
    NOZ_OBJECT(Managed)

    friend class Editor::AudioClipFile;

    private: Platform::AudioClipHandle* handle_;

    private: NOZ_PROPERTY(Name=SamplesPerSecond) noz_uint32 samples_per_second_;

    private: NOZ_PROPERTY(Name=BitsPerSample) noz_uint32 bits_per_sample_;

    private: NOZ_PROPERTY(Name=Mono) bool mono_;

    private: NOZ_PROPERTY(Name=Buffer) std::vector<noz_byte> buffer_;

    /// Return the duration of the audio clip in seconds
    public: noz_float GetDuration (void) const {return handle_->GetDuration();}
    
    public: noz_uint32 GetBitsPerSample (void) const {return bits_per_sample_;}

    public: noz_uint32 GetSamplesPerSecond (void) const {return samples_per_second_;}

    public: bool IsMono (void) const {return mono_;}

    public: bool IsStereo (void) const {return !IsMono();}

    /// Play the audip clip on any free channel
    public: void Play(noz_float volume=1.0f, noz_float pitch=1.0f);

    public: AudioClip ( void );

    public: AudioClip (const noz_byte* data, noz_uint32 data_size, noz_uint32 samples_per_sec, noz_uint32 bits_per_sample, bool mono);

    public: ~AudioClip( void );

    protected: virtual void OnDeserialized (void) override;
  };
    
} // namespace noz


#endif // __noz_AudioClip_h__
