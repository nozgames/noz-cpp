///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/BinaryDeserializer.h>
#include "SerializedObject.h"

using namespace noz;

SerializedObject::SerializedObject(void) {
  type_ = nullptr;
}

void SerializedObject::Set (Object* o) {
  data_.clear();

  if(nullptr == o) {
    type_ = nullptr;
    return;
  }

  // Save the type.
  type_ = o->GetType();
  
  // Serialize to the object data.
  MemoryStream ms(4096);
  BinarySerializer().Serialize(o,&ms);  
  data_.resize(ms.GetLength());
  memcpy(&data_[0], ms.GetBuffer(), ms.GetLength());    
}

Object* SerializedObject::CreateInstance (void) const {
  if(nullptr==type_) return nullptr;

  Object* o = type_->CreateInstance();
  if(nullptr == o) return nullptr;

  MemoryStream ms((noz_byte*)&data_[0], data_.size());
  if(nullptr==BinaryDeserializer().Deserialize(&ms,o)) {
    delete o;
    return nullptr;
  }

  return o;
}

bool SerializedObject::DeserializeInto (Object* o) const {
  if(nullptr==type_) return nullptr;
  if(nullptr==o) return nullptr;
  if(!o->IsTypeOf(type_)) return nullptr;

  MemoryStream ms((noz_byte*)&data_[0], data_.size());
  return nullptr!=BinaryDeserializer().Deserialize(&ms,o);
}
