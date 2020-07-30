///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "UInt32Property.h"

using namespace noz;

bool UInt32Property::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  noz_uint32 value = 0;
  if(!s.ReadValueUInt32(value)) return false;

  Set (target,value);

  return true;
}

bool UInt32Property::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueUInt32(Get(target));
  return true;
}
