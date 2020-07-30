///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Vector2BlendTarget.h"
#include "FloatKeyFrame.h"

using namespace noz;
  
Vector2BlendTarget::Vector2BlendTarget (Object* target, Property* prop) : WeightedBlendTarget(target,prop) {
  noz_assert(target);
  noz_assert(prop);
  noz_assert(prop->GetType()==typeof(Vector2Property));

  default_value_ = ((Vector2Property*)prop)->Get(target);
}

void Vector2BlendTarget::Add (noz_uint32 track_id, KeyFrame* from, KeyFrame* to, noz_float lerp, noz_float weight) {
  noz_assert(from);
  noz_assert(from->IsType(typeof(FloatKeyFrame)));
  noz_assert(to);
  noz_assert(to->IsType(typeof(FloatKeyFrame)));
  noz_assert(track_id>=0&&track_id<=1);
   
  FloatKeyFrame* ffrom = (FloatKeyFrame*)from;
  FloatKeyFrame* fto = (FloatKeyFrame*)to;

  accumulated_value_[track_id] += Math::EvaluateCurve(lerp, ffrom->GetValue(), ffrom->GetTangentOut(), fto->GetValue(), fto->GetTangentIn()) * weight;
}

void Vector2BlendTarget::Add (noz_uint32 track_id, KeyFrame* kf, noz_float weight) {
  noz_assert(kf);
  noz_assert(kf->IsType(typeof(FloatKeyFrame)));
  noz_assert(track_id>=0&&track_id<=1);

  accumulated_value_[track_id] += ((FloatKeyFrame*)kf)->GetValue() * weight;
}

void Vector2BlendTarget::AddDefault (noz_uint32 track_id, noz_float weight) {
  accumulated_value_[track_id] += (default_value_[track_id] * weight);
}

void Vector2BlendTarget::Advance (noz_float elapsed) {
  accumulated_value_.clear();

  WeightedBlendTarget::Advance(elapsed);

  // Apply the actual value..
  ((Vector2Property*)target_property_)->Set(target_,accumulated_value_);
}
