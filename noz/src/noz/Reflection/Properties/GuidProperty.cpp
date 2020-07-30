///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "GuidProperty.h"

using namespace noz;


bool GuidProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  Guid v;
  if(!s.ReadValueGuid(v)) return false;
  Set(target,v);
  return true;
}

bool GuidProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueGuid(Get(target));
  return true;
}
