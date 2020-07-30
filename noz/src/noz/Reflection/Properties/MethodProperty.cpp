///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "MethodProperty.h"

using namespace noz;

bool MethodProperty::IsEqual (Object* t1, Object* t2) const {
  if(t1==nullptr||t2==nullptr) return false;
  return Get(t1) == Get(t2);
}

bool MethodProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  String name;
  if(s.PeekValueNull()) {
    Set(target,(Method*)nullptr);
    return true;
  } else if(s.ReadValueString(name)) {
    noz_int32 dot = name.LastIndexOf('.');
    if(dot != -1) {
      Name type_name = name.Substring(0,dot);
      Name method_name = name.Substring(dot+1);
      Type* type = Type::FindType(type_name);
      if(type) {
        Set(target,(Method*)type->GetMethod(method_name));
      }
    } else {
      Set(target,(Method*)nullptr);
    }
    return true;
  }
  return false;
}

bool MethodProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);

  Method* method = Get(target);
  if(method) {
    s.WriteValueString(String::Format("%s.%s", method->GetParentType()->GetQualifiedName().ToCString(), method->GetName().ToCString()));
  } else {
    s.WriteValueNull();
  }
  return true;
}
