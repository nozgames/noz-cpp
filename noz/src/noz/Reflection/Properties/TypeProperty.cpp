///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "TypeProperty.h"

using namespace noz;

bool TypeProperty::IsEqual (Object* t1, Object* t2) const {
  if(t1==nullptr||t2==nullptr) return false;
  return Get(t1) == Get(t2);
}

bool TypeProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  if(s.PeekValueNull()) {
    s.ReadValueNull();
    Set(target,nullptr);
    return true;
  }

  String type_name;
  if(!s.ReadValueString(type_name)) return false;

  NOZ_TODO("warning if type missing?");

  Set(target,Type::FindType(type_name));

  return true;
}

bool TypeProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  if(nullptr==Get(target)) {
    s.WriteValueNull();
  } else {
    s.WriteValueString(Get(target)->GetQualifiedName());
  }
  return true;
}