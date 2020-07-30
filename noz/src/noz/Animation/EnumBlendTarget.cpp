///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "EnumBlendTarget.h"
#include "EnumKeyFrame.h"

using namespace noz;
  

EnumBlendTarget::EnumBlendTarget(Object* target, Property* prop) : FixedBlendTarget(target,prop) {
  noz_assert(target);
  noz_assert(prop);
  noz_assert(prop->GetType()->IsCastableTo(typeof(EnumProperty)));

  default_value_ = ((EnumProperty*)prop)->Get (target);
}

void EnumBlendTarget::Set(KeyFrame* kf) {
  noz_assert(kf);
  noz_assert(kf->IsType(typeof(EnumKeyFrame)));

  ((EnumProperty*)target_property_)->Set (target_, ((EnumKeyFrame*)kf)->GetValue());
}

void EnumBlendTarget::SetDefault(void) {
  ((EnumProperty*)target_property_)->Set ( target_, default_value_);
}

void EnumBlendTarget::Advance (noz_float elapsed) {
  FixedBlendTarget::Advance(elapsed);
}


