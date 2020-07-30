///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_AudioClipHandle_h__
#define __noz_Platform_AudioClipHandle_h__

namespace noz { class AudioClip; }

namespace noz {
namespace Platform { 

  class AudioClipHandle : public Handle {
    public: static AudioClipHandle* CreateInstance(AudioClip* clip, const noz_byte* data, noz_uint32 data_size);

    public: virtual void Play (noz_float volume=1.0f, noz_float pitch=1.0f) = 0;

    public: virtual noz_float GetDuration (void) const = 0;
  };

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_AudioClipHandle_h__

