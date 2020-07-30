///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationCurve.h"

using namespace noz;


AnimationCurve::AnimationCurve(void) {
}


void AnimationCurve::AddKey(const Keyframe& kf) {
  // Add to the back?
  if(keyframes_.empty() || kf.time_ > keyframes_.back().time_ ) {
    keyframes_.push_back(kf);
  } else {
    keyframes_.insert(keyframes_.begin() + GetKeyframeIndex(kf.time_) + 1, kf);
  }
}


noz_uint32 AnimationCurve::GetKeyframeIndex(noz_float t) {
  for(noz_uint32 i=0; i<keyframes_.size()-1; i++) {
    if(t>=keyframes_[i].time_) {
      return i;
    }
  }
    
  return keyframes_.size()-1;
}


noz_float AnimationCurve::Evaluate(noz_float t) {
  if(keyframes_.empty()) return 0.0f;
  noz_uint32 kfi = GetKeyframeIndex(t);
  const Keyframe& kf1 = keyframes_[kfi];
  const Keyframe& kf2 = keyframes_[kfi+1];

  return Math::EvaluateCurve((t-kf1.time_)/(kf2.time_-kf1.time_),kf1.value_,kf1.out_tangent_, kf2.value_, kf2.in_tangent_);
}

