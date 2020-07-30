///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/BinarySerializer.h>
#include <noz/Serialization/BinaryDeserializer.h>

#include "Style.h"

using namespace noz;

Style::Style(void) {
}

Style::~Style(void) {
}

bool Style::Template::DeserializeNodes (Deserializer& s) {
  // Read array size.
  noz_uint32 size;
  if(!s.ReadStartSizedArray(size)) return false;  
  
  target_->SetChildCapacity(size);

  // Deserialize list of proxy objects directly into target list.
  for(noz_uint32 i=0; i<size; i++) {
    Object* o = nullptr;
    if(!s.ReadValueObject(o,typeof(Node))) return false;    
    if(o) {
      ((Node*)o)->SetPrivate(true);
      target_->AddChild((Node*)o);
    }
  }

  // read end array
  if(!s.ReadEndArray()) return false;  

  return true;
}

bool Style::Template::SerializeParts (Serializer& s) {
  s.WriteStartObject();
  for(noz_uint32 i=0;i<parts_.size();i++) {
    ControlPart& p = parts_[i];
    if(p.object_ == nullptr) continue;
    if(p.property_.IsEmpty()) continue;

    s.WriteMember(p.property_);
    s.WriteValueObject(p.object_);
  }
  s.WriteEndObject();
  return true;
}

bool Style::Template::DeserializeParts (Deserializer& s) {
  if(!s.ReadStartObject()) return false;

  // Deserialize list of proxy objects directly into target list.
  while(!s.PeekEndObject()) {
    // Read the property name within the control
    Name prop_name;
    if(!s.ReadMember(prop_name)) return false;

    // Read the object within the property
    Object* o = nullptr;
    if(!s.ReadValueObject(o,typeof(Object))) return false;

    // Lookup the property
    Property* prop = target_->GetType()->GetProperty(prop_name);
    if(nullptr==prop) {
      s.ReportWarning(String::Format("part target property '%s' not found on type '%s'", prop_name.ToCString(), target_->GetType()->GetQualifiedName().ToCString()).ToCString());
      continue;
    }

    // Ensure the property is an object ptr
    if(!prop->IsTypeOf(typeof(ObjectPtrProperty))) {
      s.ReportWarning(String::Format("part target property '%s' of type '%s' must be an object pointer", prop_name.ToCString(), target_->GetType()->GetQualifiedName().ToCString()).ToCString());
      continue;
    }      

    // Ensure the object matches the value type
    ObjectPtrProperty* oprop = (ObjectPtrProperty*)prop;
    if(o && !o->IsTypeOf(oprop->GetObjectType())) {
      s.ReportWarning(String::Format("invalid part type '%s' for part target property '%s' of type '%s'", 
        o->GetType()->GetQualifiedName().ToCString(),
        prop_name.ToCString(), 
        target_->GetType()->GetQualifiedName().ToCString()
        ).ToCString());
      continue;
    }

    // Set the property value..
    oprop->Set (target_, o);
  }

  // read end array
  if(!s.ReadEndObject()) return false;  

  return true;
}
