///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "OpenAL.h"
#include "OpenALAudioChannel.h"

using namespace noz;
using namespace noz::Platform;

OpenALAudioChannel::OpenALAudioChannel(void) {
  alGenSources (1, &id_);

  alSourcef (id_, AL_PITCH, 1.0f);
  alSourcef (id_, AL_GAIN, 1.0f);

  clip_ = nullptr;
  loop_ = false;
}


OpenALAudioChannel::~OpenALAudioChannel(void) {
  alDeleteSources (1, &id_);
}
