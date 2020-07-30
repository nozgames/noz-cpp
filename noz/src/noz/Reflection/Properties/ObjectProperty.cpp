///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "ObjectProperty.h"

using namespace noz;

bool ObjectProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  Object* o = Get(target);
  return s.ReadValueObject(o,GetObjectType());
}

bool ObjectProperty::Serialize (Object* target, Serializer& s) {
  s.WriteValueObject(Get(target));
  return true;
}
