///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "NameProperty.h"

using namespace noz;

bool NameProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  Name value;
  if(!s.ReadValueName(value)) return false;
  Set(target,value);
  return true;
}

bool NameProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueName(Get(target));
  return true;
}
