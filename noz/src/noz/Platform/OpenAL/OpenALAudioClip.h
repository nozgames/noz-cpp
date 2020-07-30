///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenALAudioClip_h__
#define __noz_Platform_OpenALAudioClip_h__

#include <noz/Audio/AudioClip.h>
#include <noz/Platform/AudioClipHandle.h>

namespace noz {
namespace Platform {

  class OpenALAudioClip : public AudioClipHandle {
    /// OpenAL buffer
    private: ALuint buffer_;
    
    /// OpenAL buffer format 
    private: ALenum format_;
    
    public: OpenALAudioClip(void);

    public: ~OpenALAudioClip(void);

    public: virtual void Play (noz_float volume=1.0f, noz_float pitch=1.0f) override;

    public: bool Load (AudioClip* clip, const noz_byte* data, noz_uint32 data_size);

    /// Return the buffer
    public: ALuint GetBuffer (void) const {return buffer_;}

    /// Override to return the duration of the AudioClip
    public: virtual noz_float GetDuration (void) const override {return 0.0f;}
    
    /// Return the OpenaL format for the given bit and channel counts
    private: static ALenum GetFormat (noz_int32 bits, noz_int32 channels);
  };

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenALAudioClip_h__
