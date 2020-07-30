///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenALAudioChannel_h__
#define __noz_Platform_OpenALAudioChannel_h__

#include "OpenALAudioClip.h"

namespace noz {
namespace Platform {

  class OpenALManager;

  class OpenALAudioChannel { 
    friend class OpenALManager;

    private: ALuint id_;
    private: bool loop_;
    private: OpenALAudioClip* clip_;

    public: OpenALAudioChannel(void);

    public: ~OpenALAudioChannel(void);
  };


} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenALAudioChannel_h__
