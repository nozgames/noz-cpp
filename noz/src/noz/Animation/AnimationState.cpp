///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationState.h"

using namespace noz;

AnimationState::AnimationState(void) {
  blend_time_ = 0.0f;
  speed_ = 1.0f;
}

void AnimationState::SetName (const char* text) {
  name_ = text;
}

void AnimationState::SetAnimation (Animation* animation) {
  animation_ = animation;
}

