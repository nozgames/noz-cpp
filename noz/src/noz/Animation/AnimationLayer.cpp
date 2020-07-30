///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationLayer.h"

using namespace noz;

AnimationLayer::AnimationLayer(void) {
  index_ = 0xFFFFFFFF;
  weight_ = 1.0f;
}

AnimationLayer::~AnimationLayer(void) {
  for(noz_uint32 i=0,c=states_.size();i<c;i++) delete states_[i];
}

AnimationState* AnimationLayer::GetState (const Name& name) const {
  for(noz_uint32 i=0,c=states_.size();i<c;i++) {
    if(states_[i]->GetName()==name) return states_[i];
  }
  return nullptr;
}

void AnimationLayer::AddState (AnimationState* state) {
  noz_assert(state);
  states_.push_back(state);
}

