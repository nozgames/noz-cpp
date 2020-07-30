///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "ByteProperty.h"

using namespace noz;

bool ByteProperty::IsEqual (Object* t1, Object* t2) const {
  if(t1==nullptr||t2==nullptr) return false;
  return Get(t1) == Get(t2);
}

bool ByteProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);
  noz_byte v;
  if(!s.ReadValueByte(v)) return false;
  Set(target,v);
  return true;
}

bool ByteProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueByte(Get(target));
  return true;
}


bool ByteVectorProperty::Deserialize (Object* target, Deserializer& s) {
  std::vector<noz_byte>& v = Get (target);
  noz_uint32 size = 0;
  if(!s.ReadValueBytesSize(size)) return false;  
  if(size==0) {
    v.clear();
    return true;
  }

  v.resize(size);
  if(!s.ReadValueBytes(&v[0])) return false;
  return true; 
}

bool ByteVectorProperty::Serialize (Object* target, Serializer& s) {
  std::vector<noz_byte>& v = Get (target);
  if(v.size()>0) {
    s.WriteValueBytes(&v[0],v.size());
  } else {
    s.WriteValueBytes(nullptr,0);
  }
  return true;
}
