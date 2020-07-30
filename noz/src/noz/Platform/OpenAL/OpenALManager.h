///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenALManager_h__
#define __noz_Platform_OpenALManager_h__

#include <noz/System/Thread.h>
#include <noz/System/Mutex.h>

namespace noz {
namespace Platform {

  class OpenALAudioChannel;
  class OpenALAudioClip;

  class OpenALManager { 
    private: static OpenALManager* this_;

    /// Thread running audio
    private: Thread thread_;
  
    /// Mutex used to lock the audio thread
    private: Mutex thread_lock_;

    /// Set to true when the audio thread should exit.
    private: bool thread_exit_;

    private: noz_int32 initialize_count_;

    /// Available sound audio channels.
    private: OpenALAudioChannel* channels_[4];

    /// Round robin channel to play on
    private: noz_int32 round_robin_;

    /// OpenAL device
    private: ALCdevice* device_;

    /// OpenAL context
    private: ALCcontext* context_;

    public: static void Initialize (void);

    public: static void Uninitialize (void);

    private: OpenALManager (void);

    private: ~OpenALManager (void);

    public: static void Play (OpenALAudioClip* clip, noz_float volume=1.0f, noz_float pitch=1.0f);

    private: void ThreadProc (void);

    /// Play the given audio clip on the given channel
    private: bool Play (OpenALAudioChannel* channel, OpenALAudioClip* clip, noz_int32 volume, noz_float pitch, bool loop);

    /// Stop audio on the given channel
    private: void Stop (OpenALAudioChannel* channel);
  };


} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenALManager_h__
