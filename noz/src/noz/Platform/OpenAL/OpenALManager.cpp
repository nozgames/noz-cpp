///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "OpenAL.h"

using namespace noz;
using namespace noz::Platform;

OpenALManager* OpenALManager::this_ = nullptr;

void OpenALManager::Initialize (void) {
  if(nullptr!=this_) {
    this_->initialize_count_++;
    return;
  }
  this_ = new OpenALManager;
  this_->thread_.Start(ThreadStart(this_, &OpenALManager::ThreadProc));
}

void OpenALManager::Uninitialize (void) {
  if(nullptr == this_) return;
  this_->initialize_count_--;
  if(this_->initialize_count_>0) return;

  this_->thread_exit_ = true;
  this_->thread_.Terminate();
  this_->thread_.WaitForExit(5000);

  this_->thread_lock_.WaitOne();

  if(this_->context_) alcDestroyContext (this_->context_);
  if(this_->device_) alcCloseDevice (this_->device_);

  this_->thread_lock_.Release();
  
  delete this_;
  this_ = nullptr;
}

OpenALManager::OpenALManager(void) {
  round_robin_ = 0;
  thread_exit_ = false;
  initialize_count_ = 1;

  // Initialize openAL device
  device_ = alcOpenDevice (nullptr);
  if (nullptr==device_) return;

#if 0
#ifdef NOZ_IOS
  alcMacOSXMixerOutputRateProc (44100);
#endif
#endif

  // Initialize openAL context
  context_ = alcCreateContext (device_,nullptr);
  if (nullptr==context_) {
    alcCloseDevice (device_);
    device_ = nullptr;
    return;
  }
  
  alcMakeContextCurrent (context_);
  alListener3f(AL_POSITION, 0.0f, 0.0f, 1.0f);
  alListenerf(AL_GAIN, 1.0f);

  channels_[0] = new OpenALAudioChannel;
  channels_[1] = new OpenALAudioChannel;
  channels_[2] = new OpenALAudioChannel;
  channels_[3] = new OpenALAudioChannel;
}

OpenALManager::~OpenALManager(void) {
  delete channels_[0];
  delete channels_[1];
  delete channels_[2];
  delete channels_[3];
}

void OpenALManager::Play (OpenALAudioClip* clip, noz_float volume, noz_float pitch) {
  this_->thread_lock_.WaitOne();
  noz_int32 c;
  for(c=0;c<4;c++) {
    if(!this_->channels_[c]->clip_) {
      this_->Play(this_->channels_[c],clip,noz_int32(100.0f*volume),pitch,false);
      break;
    }
  }
  if(c>=4) {
    this_->Play(this_->channels_[this_->round_robin_],clip,noz_int32(100.0f*volume),pitch,false);
    this_->round_robin_ = (this_->round_robin_+1)%4;
  }
  this_->thread_lock_.Release();
}

bool OpenALManager::Play (OpenALAudioChannel* channel, OpenALAudioClip* clip, noz_int32 volume, noz_float pitch, bool loop) {  
  noz_assert(channel);
  noz_assert(clip);

  // Cancel current sound on channel
  alSourceStop (channel->id_);

  // Set channel volume
  alSourcef (channel->id_, AL_GAIN, (noz_float)volume / (noz_float)100);

  // Set the buffer and play it
  alSourcei (channel->id_, AL_BUFFER, clip->GetBuffer());
  alSourcef (channel->id_, AL_PITCH, pitch);
  alSourcei (channel->id_, AL_LOOPING, loop?AL_TRUE:AL_FALSE );
  alSourcePlay (channel->id_ );

  channel->clip_ = clip;
  channel->loop_ = loop;
  
  return true;
}

void OpenALManager::Stop (OpenALAudioChannel* channel) {
  noz_assert(channel);

  if(nullptr == channel->clip_) return;

  // Stop AL channel
  alSourceStop (channel->id_);

  // Unqueue any queued buffers
  ALint queued = 0;
  alGetSourcei (channel->id_, AL_BUFFERS_QUEUED, &queued);
  if (queued != 0) {
    ALuint buffer[4];    
    alSourceUnqueueBuffers (channel->id_, queued, buffer);
  }  
    
  alGetSourcei (channel->id_, AL_BUFFERS_QUEUED, &queued);

  // Clear reference to clip that is playing
  channel->clip_ = nullptr;
}

void OpenALManager::ThreadProc (void) {
  while (!thread_exit_) {
    thread_lock_.WaitOne();
    
    if (device_) {
      for(noz_uint32 i=0;i<4;i++) {
        OpenALAudioChannel* channel = channels_[i];

        // Ensure the channel is active..
        if(nullptr == channel->clip_) continue;

        // Check for sounds that are finished playing
        ALint state;
        alGetSourcei (channel->id_, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
          Stop (channel);
          continue;
        }      
      }
    }
    
    thread_lock_.Release();
    
    // Run at about 30Hz
    Thread::Sleep(30);
  }  
}

