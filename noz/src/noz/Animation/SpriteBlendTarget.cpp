///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "SpriteBlendTarget.h"
#include "SpriteKeyFrame.h"

using namespace noz;
  

SpriteBlendTarget::SpriteBlendTarget(Object* target, Property* prop) : FixedBlendTarget(target,prop) {
  noz_assert(target);
  noz_assert(prop);
  noz_assert(prop->GetType()->IsCastableTo(typeof(ObjectPtrProperty)));

  default_value_ = (Sprite*) ((ObjectPtrProperty*)prop)->Get(target);
}

void SpriteBlendTarget::Set(KeyFrame* kf) {
  noz_assert(kf);
  noz_assert(kf->IsType(typeof(SpriteKeyFrame)));

  ((ObjectPtrProperty*)target_property_)->Set(target_, ((SpriteKeyFrame*)kf)->GetValue());
}

void SpriteBlendTarget::SetDefault(void) {
  ((ObjectPtrProperty*)target_property_)->Set(target_, default_value_);
}

void SpriteBlendTarget::Advance (noz_float elapsed) {
  FixedBlendTarget::Advance(elapsed);
}


