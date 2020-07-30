///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationController.h"

using namespace noz;

AnimationController::AnimationController(void) {
}

AnimationController::~AnimationController(void) {
  for(noz_uint32 i=0,c=layers_.size(); i<c; i++) delete layers_[i];
}

void AnimationController::AddLayer (AnimationLayer* layer) {
  layers_.push_back(layer);
  layer->index_ = layers_.size()-1;
}

