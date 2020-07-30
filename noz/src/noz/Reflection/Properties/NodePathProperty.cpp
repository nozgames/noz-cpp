///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "NodePathProperty.h"

using namespace noz;


bool NodePathProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  String value;
  if(!s.ReadValueString(value)) return false;
  Set(target,value);
  return true;
}

bool NodePathProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  const NodePath& np = Get(target);
  StringBuilder sb;
  if(np.GetLength()>0) {
    sb.Append(np[0]);
    for(noz_uint32 i=1;i <np.GetLength(); i++) {
      sb.Append('/');
      sb.Append(np[i]);
    }
    s.WriteValueString(sb.ToString());
    return true;
  }

  s.WriteValueString("");
  return true;
}

