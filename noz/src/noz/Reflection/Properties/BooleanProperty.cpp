///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "BooleanProperty.h"

using namespace noz;

BlendTarget* BooleanProperty::CreateBlendTarget (Object* target) const {
  return nullptr;;
}

bool BooleanProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);
  bool v;
  if(!s.ReadValueBoolean(v)) return false;
  Set(target,v);
  return true;
}

bool BooleanProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueBoolean(Get(target));
  return true;
}
