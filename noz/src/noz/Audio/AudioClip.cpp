///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AudioClip.h"

using namespace noz;
using namespace noz::Platform;

AudioClip::AudioClip(void) {
  handle_ = nullptr;
  mono_ = false;
  samples_per_second_ = 0;
  bits_per_sample_ = 0;
}

AudioClip::AudioClip (const noz_byte* data, noz_uint32 data_size, noz_uint32 samples_per_sec, noz_uint32 bits_per_sample, bool mono) {
  samples_per_second_ = samples_per_sec;
  bits_per_sample_ = bits_per_sample;
  mono_ = mono;
  buffer_.resize(data_size);
  memcpy(&buffer_[0],data,data_size);
  handle_ = AudioClipHandle::CreateInstance(this, data, data_size);
}

AudioClip::~AudioClip(void) {
  delete handle_;
}


void AudioClip::Play(noz_float volume, noz_float pitch) {
  if(handle_==nullptr) return;
  handle_->Play(volume,pitch);
}

void AudioClip::OnDeserialized (void) {
  if(buffer_.empty()) return;
  delete handle_;
  handle_ = AudioClipHandle::CreateInstance(this, &buffer_[0], buffer_.size());
}
