///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimatorLayer.h"

using namespace noz;

AnimatorLayer::AnimatorLayer(void) {
  index_ = 0;
  weight_ = 1.0f;
  current_state_ = nullptr;
}

AnimatorLayer::~AnimatorLayer(void) {
  // Delete all states
  for(auto it=states_.begin(); it!=states_.end(); it++) {
    delete (*it);
  }
  states_.clear();  
}

AnimatorState* AnimatorLayer::GetState(const Name& name) const {
  for(auto it=states_.begin(); it!=states_.end(); it++) {
    AnimatorState* state = *it;
    noz_assert(state);

    if(state->name_ == name) return state;
  }

  return nullptr;
}
