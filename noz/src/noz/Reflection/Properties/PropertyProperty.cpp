///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "PropertyProperty.h"

using namespace noz;

bool PropertyProperty::IsEqual (Object* t1, Object* t2) const {
  if(t1==nullptr||t2==nullptr) return false;
  return Get(t1) == Get(t2);
}

bool PropertyProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  String name;
  if(s.PeekValueNull()) {
    Set(target,(Property*)nullptr);
    return true;
  } else if(s.ReadValueString(name)) {
    noz_int32 dot = name.LastIndexOf('.');
    if(dot != -1) {
      Name type_name = name.Substring(0,dot);
      Name prop_name = name.Substring(dot+1);
      Type* type = Type::FindType(type_name);
      if(type) {
        Set(target,(Property*)type->GetProperty(prop_name));
      }
    } else {
      Set(target,(Property*)nullptr);
    }
    return true;
  }
  return false;
}

bool PropertyProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);

  Property* prop = Get(target);
  if(prop) {
    s.WriteValueString(String::Format("%s.%s", prop->GetParentType()->GetQualifiedName().ToCString(), prop->GetName().ToCString()));
  } else {
    s.WriteValueNull();
  }
  return true;
}
