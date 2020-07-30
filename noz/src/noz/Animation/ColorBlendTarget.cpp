///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ColorBlendTarget.h"
#include "FloatKeyFrame.h"

using namespace noz;
  
ColorBlendTarget::ColorBlendTarget (Object* target, Property* prop) : WeightedBlendTarget(target,prop) {
  noz_assert(target);
  noz_assert(prop);
  noz_assert(prop->GetType()==typeof(ColorProperty));

  default_value_ = ((ColorProperty*)prop)->Get(target).ToVector4();
}

void ColorBlendTarget::Add (noz_uint32 track_id, KeyFrame* from, KeyFrame* to, noz_float lerp, noz_float weight) {
  noz_assert(from);
  noz_assert(from->IsType(typeof(FloatKeyFrame)));
  noz_assert(to);
  noz_assert(to->IsType(typeof(FloatKeyFrame)));
   
  FloatKeyFrame* ffrom = (FloatKeyFrame*)from;
  FloatKeyFrame* fto = (FloatKeyFrame*)to;

  accumulated_value_[track_id] += (weight * Math::EvaluateCurve(lerp, ffrom->GetValue(), ffrom->GetTangentOut(), fto->GetValue(), fto->GetTangentIn()));
}

void ColorBlendTarget::Add (noz_uint32 track_id, KeyFrame* kf, noz_float weight) {
  noz_assert(kf);
  noz_assert(kf->IsType(typeof(FloatKeyFrame)));

  accumulated_value_[track_id] += ((FloatKeyFrame*)kf)->GetValue() * weight;
}

void ColorBlendTarget::AddDefault (noz_uint32 track_id, noz_float weight) {
  accumulated_value_[track_id] += (default_value_[track_id] * weight);
}

void ColorBlendTarget::Advance (noz_float elapsed) {
  accumulated_value_.Clear();

  WeightedBlendTarget::Advance(elapsed);

  // Apply the actual value..
  ((ColorProperty*)target_property_)->Set(target_,Color(accumulated_value_));
}
