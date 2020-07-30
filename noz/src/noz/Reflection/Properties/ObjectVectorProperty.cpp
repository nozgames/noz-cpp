///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Deserializer.h>
#include <noz/Serialization/Serializer.h>
#include "ObjectPtrProperty.h"

using namespace noz;

bool ObjectVectorProperty::Deserialize (Object* target, Deserializer& s) {
  noz_uint32 size;
  if(!s.ReadStartSizedArray(size)) return false;

  // Set the vector size
  SetSize(target,size);

  for(noz_uint32 i=0; i<size; i++) {
    Object* o = AddElement(target);
    if(!s.ReadValueObject(o,GetElementType())) return false;    
  }

  return s.ReadEndArray();
}

bool ObjectVectorProperty::Serialize (Object* target, Serializer& s) {
  noz_uint32 size = GetSize(target);
  s.WriteStartSizedArray(size);
  for(noz_uint32 i=0;i<size;i++) {
    s.WriteValueObject(GetElement(target,i));
  }
  s.WriteEndArray();
  return true;
}
