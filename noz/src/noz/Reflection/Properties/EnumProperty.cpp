///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Animation/EnumBlendTarget.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "EnumProperty.h"

using namespace noz;

BlendTarget* EnumProperty::CreateBlendTarget (Object* target) const {
  return new EnumBlendTarget(target,(Property*)this);
}

bool EnumProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);
  Name v;
  if(!s.ReadValueName(v)) return false;
  Set (target,v);
  return true;
}

bool EnumProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueName(Get(target));
  return true;
}
