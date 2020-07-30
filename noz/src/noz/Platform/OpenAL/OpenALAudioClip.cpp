///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "OpenAL.h"

using namespace noz;
using namespace noz::Platform;

AudioClipHandle* noz::Platform::AudioClipHandle::CreateInstance(AudioClip* clip, const noz_byte* data, noz_uint32 data_size) {
  OpenALManager::Initialize();

  OpenALAudioClip* handle = new OpenALAudioClip;
  if(!handle->Load(clip,data,data_size)) {
    delete handle;
    return nullptr;
  }

  return handle;
}

OpenALAudioClip::OpenALAudioClip(void) {
  buffer_ = 0;
  format_ = 0;
}

OpenALAudioClip::~OpenALAudioClip(void) {
  if(buffer_!=0) alDeleteBuffers(1, &buffer_);
  OpenALManager::Uninitialize();
}

void OpenALAudioClip::Play (noz_float volume, noz_float pitch) {  
  OpenALManager::Play(this, volume, pitch);
}

bool OpenALAudioClip::Load (AudioClip* clip, const noz_byte* data, noz_uint32 data_size) {
  // Determine format.
  if(clip->GetBitsPerSample()==8) {
    format_ = clip->IsMono() ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
  } else if(clip->GetBitsPerSample()==16) {
    format_ = clip->IsMono() ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
  } else {
    return false;
  }

  // Generate a buffer for the sound data  
  alGenBuffers (1, &buffer_);
   
  // Allocate buffer data
	noz_int32 buffer_size = (noz_int32)data_size;
	buffer_size += (1024 - (buffer_size % 1024));
	buffer_size += 1024;
	buffer_size = Math::Max(buffer_size,2048);
  
  noz_byte* buffer = new noz_byte[buffer_size];
  memset (buffer,0,buffer_size);
    
  // Read all data into the buffer
  memcpy (buffer, data, data_size);
          
  // Upload the buffer to OpenaL
  alBufferData ( 
    buffer_, 
    format_,
    buffer,
    buffer_size,
    clip->GetSamplesPerSecond()
  );
    
  // Free the temporary buffer
  delete[] buffer;
  
  // Success!
  return true;
}
