///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryReader.h>
#include <noz/IO/BinaryWriter.h>

#include "BinarySerializer.h"

using namespace noz;

BinarySerializer::BinarySerializer(void) {
  writer_ = nullptr;
}

bool BinarySerializer::Serialize (Object* o, Stream* stream) { 
  header_position_ = stream->GetPosition();
  
  // Empty name is index 0 in name table.
  names_.clear();
  names_.push_back(Name::Empty);
  name_map_[Name::Empty] = 0;

  // Root object
  SerializedObject& root = object_map_[o->GetId()];
  root.id_ = 1;
  root.object_ = o;
  root.offset_ = 0;
  root.type_ = o->GetType();
  objects_.push_back(&root);

  // Write empty header..
  Header header;
  header.fourcc_ = FourCC;
  header.name_table_count_ = 0;
  header.name_table_offset_ = 0;
  header.object_table_offset_ = 0;
  header.root_ = 1;
  header.version_ = 1;
  stream->Write((char*)&header,0,sizeof(Header));

  BinaryWriter writer(stream);
  writer_ = &writer;

  // Write Objects..
  for(size_t q=0; q<objects_.size(); q++) {
    if(!Serialize(stream,objects_[q])) {
    } else {
    }
  }  

  // Write Object table
  header.object_table_offset_ = stream->GetPosition() - header_position_;
  header.object_table_count_ = objects_.size();
  for(size_t o=0;o<objects_.size();o++) {
    SerializedObject* so = objects_[o];
    
    // Write object type name.
    WriteName(so->object_->GetType()->GetQualifiedName());

    // Write object ooffset
    writer.WriteUInt32(so->offset_);

    // If the object has no offset within this file it must be an external asset..
    if(so->offset_ == 0) {
      writer.WriteUInt64(so->asset_guid_.GetHighOrder());
      writer.WriteUInt64(so->asset_guid_.GetLowOrder());
    }
  }

  // Write name table.
  header.name_table_offset_ = stream->GetPosition() - header_position_;
  header.name_table_count_ = names_.size() - 1;
  for(size_t n=1;n<names_.size();n++) {
    writer.WriteName(names_[n]);
  }

  // Seek back to the header position and write the completed header 
  noz_uint32 end_pos = stream->GetPosition();
  stream->Seek(header_position_,SeekOrigin::Begin);
  stream->Write((char*)&header, 0, sizeof(Header));

  // Seek back to the end of the stream
  stream->Seek(end_pos,SeekOrigin::Begin);

  writer_ = nullptr;

  return true;
}

noz_uint32 BinarySerializer::WriteObject (Object* o) {
  noz_assert(writer_);

  if(nullptr==o) {
    writer_->WriteUInt32(0);
    return 0;
  }

  auto it = object_map_.find(o->GetId());
  if(it != object_map_.end()) {
    writer_->WriteUInt32(it->second.id_);
    return it->second.id_;
  }
    
  SerializedObject& so = object_map_[o->GetId()];
  so.id_ = object_map_.size();
  so.object_ = o;
  so.offset_ = 0;
  so.type_ = o->GetType();
  objects_.push_back(&so);

  // If the object is a manged asset we want to only serialize its guid.
  if(so.type_->IsAsset() && ((Asset*)o)->IsManaged()) {    
    so.asset_guid_ = ((Asset*)o)->GetGuid();
  } else {
    writer_->WriteUInt32(so.id_);
  }

  return so.id_;
}

bool BinarySerializer::Serialize (Stream* stream, SerializedObject* so) {
  if(!so->asset_guid_.IsEmpty()) return true;

  Object* target = so->object_;
  noz_assert(target);

  // Save off the start position of the object..
  so->offset_ = stream->GetPosition() - header_position_;

  BinaryWriter temp_writer(&temp_stream_);
 
  // Write all non default properties
  for(Type* t=so->type_; t; t=t->GetBase()) {
    for(auto p=t->GetProperties().begin(); p!=t->GetProperties().end(); p++) {
      if(!(*p)->IsSerializable()) continue;
      if(!(*p)->IsDefault(target)) {       
        // Write the property to the temp stram
        temp_stream_.SetLength(0);
        BinaryWriter* old_writer = writer_;
        writer_ = &temp_writer;
        bool result = (*p)->Serialize(target, *this);
        writer_ = old_writer;
        if(!result || temp_stream_.GetLength() == 0) {
          noz_assert(false);
          continue;
        }

        // Write Property Name
        WriteName((*p)->GetName());

        // Write Property version
        writer_->WriteUInt32(0);

        // Write Property size
        writer_->WriteUInt32(temp_stream_.GetLength());

        // Write Property data
        writer_->Write((char*)temp_stream_.GetBuffer(), 0, temp_stream_.GetLength());
      }
    }
  }

  // Null name ot signify end of the properties
  writer_->WriteUInt32(0);

  return true;
}

noz_uint32 BinarySerializer::WriteName (const Name& name) {
  auto it = name_map_.find(name);
  if(it != name_map_.end()) {
    writer_->WriteUInt32(it->second);
    return it->second;
  }

  names_.push_back(name);
  name_map_[name] = names_.size()-1;
  writer_->WriteUInt32(names_.size()-1);
  return names_.size()-1;
}

void BinarySerializer::WriteDataType(DataType vt) {
  noz_assert(writer_);
  writer_->WriteByte((noz_byte)vt);
}

bool BinarySerializer::WriteStartObject (void) {
  WriteDataType(DataType::StartObject);
  return true;
}

bool BinarySerializer::WriteEndObject (void) {
  WriteDataType(DataType::EndObject);
  return true;
}

bool BinarySerializer::WriteStartSizedArray (noz_uint32 size) {
  WriteDataType(DataType::StartSizedArray);
  writer_->WriteUInt32(size);
  return true;
}

bool BinarySerializer::WriteStartArray (void) {
  WriteDataType(DataType::StartArray);
  return true;
}

bool BinarySerializer::WriteEndArray (void) {
  WriteDataType(DataType::EndArray);
  return true;
}

bool BinarySerializer::WriteMember (const char* name) {
  WriteDataType(DataType::Member);
  WriteName(name);
  return true;
}

bool BinarySerializer::WriteValueString (const String& v) {
  WriteDataType(DataType::ValueString);
  writer_->WriteString(v);
  return true;
}

bool BinarySerializer::WriteValueName (const Name& v) {
  WriteDataType(DataType::ValueName);
  WriteName(v);
  return true;
}

bool BinarySerializer::WriteValueInt32 (noz_int32 v) {
  WriteDataType(DataType::ValueInt32);
  writer_->WriteInt32(v);
  return true;
}

bool BinarySerializer::WriteValueUInt32 (noz_uint32 v) {
  WriteDataType(DataType::ValueUInt32);
  writer_->WriteUInt32(v);
  return true;
}

bool BinarySerializer::WriteValueFloat (noz_float v) {
  WriteDataType(DataType::ValueFloat);
  writer_->WriteFloat(v);
  return true;
}

bool BinarySerializer::WriteValueNull (void) {
  WriteDataType(DataType::ValueNull);
  return true;
}

bool BinarySerializer::WriteValueBoolean (bool v) {
  WriteDataType(DataType::ValueBoolean);
  writer_->WriteBoolean(v);
  return true;
}

bool BinarySerializer::WriteValueColor (Color v) {
  WriteDataType(DataType::ValueColor);
  writer_->WriteUInt32(v.raw);
  return true;
}

bool BinarySerializer::WriteValueByte (noz_byte v) {
  WriteDataType(DataType::ValueByte);
  writer_->WriteByte(v);
  return true;
}

bool BinarySerializer::WriteValueBytes (noz_byte* b, noz_uint32 size) {
  WriteDataType(DataType::ValueBytesSize);
  writer_->WriteUInt32(size);

  if(size>0) {
    WriteDataType(DataType::ValueBytes);
    writer_->Write((char*)b,0,size);
  }
  return true;
}

bool BinarySerializer::WriteValueObject (Object* o, bool inline_assets) {
  if(!inline_assets && o && o->IsTypeOf(typeof(Asset)) && ((Asset*)o)->IsManaged()) {
    WriteValueGuid(((Asset*)o)->GetGuid());
    return true;
  }

  WriteDataType(DataType::ValueObject);
  WriteObject(o);
  return true;
}

bool BinarySerializer::WriteValueGuid (const Guid& guid) {
  WriteDataType(DataType::ValueGuid);
  writer_->WriteUInt64(guid.GetHighOrder());
  writer_->WriteUInt64(guid.GetLowOrder());
  return true;
}
