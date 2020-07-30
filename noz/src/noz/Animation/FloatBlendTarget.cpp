///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "FloatBlendTarget.h"
#include "FloatKeyFrame.h"

using namespace noz;
  
FloatBlendTarget::FloatBlendTarget (Object* target, Property* prop) : WeightedBlendTarget(target,prop) {
  noz_assert(target);
  noz_assert(prop);
  noz_assert(prop->GetType()==typeof(FloatProperty));

  default_value_ = ((FloatProperty*)prop)->Get(target);
}

void FloatBlendTarget::Add (noz_uint32 track_id, KeyFrame* from, KeyFrame* to, noz_float lerp, noz_float weight) {
  noz_assert(from);
  noz_assert(from->IsType(typeof(FloatKeyFrame)));
  noz_assert(to);
  noz_assert(to->IsType(typeof(FloatKeyFrame)));
  noz_assert(track_id==0);
   
  FloatKeyFrame* ffrom = (FloatKeyFrame*)from;
  FloatKeyFrame* fto = (FloatKeyFrame*)to;

  accumulated_value_ += Math::EvaluateCurve(lerp, ffrom->GetValue(), ffrom->GetTangentOut(), fto->GetValue(), fto->GetTangentIn());
}

void FloatBlendTarget::Add (noz_uint32 track_id, KeyFrame* kf, noz_float weight) {
  noz_assert(kf);
  noz_assert(kf->IsType(typeof(FloatKeyFrame)));
  noz_assert(track_id==0);

  accumulated_value_ += ((FloatKeyFrame*)kf)->GetValue() * weight;
}

void FloatBlendTarget::AddDefault (noz_uint32 track_id, noz_float weight) {
  noz_assert(track_id==0);
  accumulated_value_ += default_value_ * weight;
}

void FloatBlendTarget::Advance (noz_float elapsed) {
  accumulated_value_ = 0.0f;
  WeightedBlendTarget::Advance(elapsed);

  // Apply the actual value..
  ((FloatProperty*)target_property_)->Set(target_,accumulated_value_);
}
