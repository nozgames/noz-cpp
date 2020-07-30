///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "Int32Property.h"

using namespace noz;


bool Int32Property::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  noz_int32 v;
  if(!s.ReadValueInt32(v)) return false;
  Set(target,v);
  return true;
}

bool Int32Property::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueInt32(Get(target));
  return true;
}
