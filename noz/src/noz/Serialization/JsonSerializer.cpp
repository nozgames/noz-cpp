///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/StreamWriter.h>
#include <noz/Text/Json/JsonWriter.h>

#include "JsonSerializer.h"

using namespace noz;

static const Name NameId ("_Id");
static const Name NameType ("_Type");
static const Name NameRoot ("Root");
static const Name NameObjects ("Objects");


bool JsonSerializer::Serialize (Object* o, Stream* stream) { 
  StreamWriter swriter(stream);
  JsonWriter writer(&swriter);

  writer_ = &writer;

  // Write top level object
  writer.WriteStartObject();
  writer.WriteMember(NameRoot);
  WriteValueObject(o,true);
  writer.WriteMember(NameObjects);

  // Write object array
  writer.WriteStartArray();

  // Serilize all objects in the queue.
  for(size_t q=0; q<queue_.size(); q++) {
    if(!Serialize(writer,queue_[q])) {
    } else {
    }
  }  

  writer.WriteEndArray();
  writer.WriteEndObject();

  swriter.Write('\n');

  writer_ = nullptr;

  return true;
}

bool JsonSerializer::Serialize (JsonWriter& writer, SerializedObject* so) {
  noz_assert(so);
  noz_assert(so->id_ != 0);
  noz_assert(so->type_);

  Object* target = so->object_;
  noz_assert(target);

  // Write an object with id and type members
  writer.WriteStartObject();
  writer.WriteMember(NameId);
  writer.WriteValueUInt32(so->id_);
  writer.WriteMember(NameType);
  writer.WriteValueString(so->type_->GetQualifiedName());

  // Write all non default properties
  for(Type* t=so->type_; t; t=t->GetBase()) {
    for(auto p=t->GetProperties().begin(); p!=t->GetProperties().end(); p++) {
      if(!(*p)->IsSerializable()) continue;
      if(!(*p)->IsDefault(target)) {
        writer.WriteMember((*p)->GetName());
        (*p)->Serialize(target, *this);
      }
    }
  }

  // End the object..
  writer.WriteEndObject();

  return true;
}

bool JsonSerializer::WriteStartObject (void) {
  noz_assert(writer_);
  writer_->WriteStartObject();
  return true;
}

bool JsonSerializer::WriteEndObject (void) {
  noz_assert(writer_);
  writer_->WriteEndObject();
  return true;
}

bool JsonSerializer::WriteStartSizedArray (noz_uint32 size) {
  noz_assert(writer_);
  writer_->WriteStartArray();
  return true;
}

bool JsonSerializer::WriteStartArray (void) {
  noz_assert(writer_);
  writer_->WriteStartArray();
  return true;
}

bool JsonSerializer::WriteEndArray (void) {
  noz_assert(writer_);
  writer_->WriteEndArray();
  return true;
}

bool JsonSerializer::WriteMember (const char* name) {
  noz_assert(writer_);
  writer_->WriteMember(name);
  return true;
}

bool JsonSerializer::WriteValueColor (Color v) {
  noz_assert(writer_);
  writer_->WriteValueString(v.ToString());
  return true;
}
  
bool JsonSerializer::WriteValueName (const Name& v) {
  noz_assert(writer_);
  writer_->WriteValueString(v);
  return true;
}

bool JsonSerializer::WriteValueString (const String& v) {
  noz_assert(writer_);
  writer_->WriteValueString(v);
  return true;
}

bool JsonSerializer::WriteValueInt32 (noz_int32 v) {
  noz_assert(writer_);
  writer_->WriteValueInt32(v);
  return true;
}

bool JsonSerializer::WriteValueUInt32 (noz_uint32 v) {
  noz_assert(writer_);
  writer_->WriteValueUInt32(v);
  return true;
}

bool JsonSerializer::WriteValueFloat (noz_float v) {
  noz_assert(writer_);
  writer_->WriteValueFloat(v);
  return true;
}

bool JsonSerializer::WriteValueNull (void) {
  noz_assert(writer_);
  writer_->WriteValueNull();
  return true;
}

bool JsonSerializer::WriteValueBoolean (bool v) {
  noz_assert(writer_);
  writer_->WriteValueBoolean(v);
  return true;
}

bool JsonSerializer::WriteValueByte (noz_byte v) {
  noz_assert(writer_);
  writer_->WriteValueByte(v);
  return true;
}

bool JsonSerializer::WriteValueBytes (noz_byte* b, noz_uint32 size) {
  // writing byte arrays not supported in JSON
  noz_assert(false);
  return false;
}

bool JsonSerializer::WriteValueObject (Object* o, bool inline_assets) {
  // Write zero to indicated no object
  if(nullptr==o) {
    return WriteValueUInt32(0);
  }

  // Managed asset?
  if(!inline_assets && o->IsTypeOf(typeof(Asset)) && ((Asset*)o)->IsManaged()) {
    WriteValueString(String::Format("{%s}",((Asset*)o)->GetGuid().ToString().ToCString()));
    return true;
  }

  // Look up the object by its identifier to see if we have already
  // assigned it to a SerializedObject
  auto it = objects_.find(o->GetId());
  if(it != objects_.end()) {
    return WriteValueUInt32(it->second.id_);
  }

  // Create a new serialized object..
  SerializedObject& so = objects_[o->GetId()];
  so.id_ = objects_.size();
  so.object_ = o;
  so.type_ = o->GetType();

  queue_.push_back(&so);

  // Write the serialized object identifier 
  return WriteValueUInt32(so.id_);
}

bool JsonSerializer::WriteValueGuid (const Guid& guid) {
  return WriteValueString(String::Format("{%s}", guid.ToString().ToCString()));
}
