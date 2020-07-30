
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryReader.h>

#include "BinaryDeserializer.h"

using namespace noz;

BinaryDeserializer::BinaryDeserializer(void) {
  reader_ = nullptr;
  error_ = false;
  peek_ = BinarySerializer::DataType::Unknown;
}

BinaryDeserializer::~BinaryDeserializer(void) {
  // For each objects in the serialized objects array delete any
  // that are only referenced by the serialized object itself.
  for(auto it=objects_.begin(); it!=objects_.end(); it++) {
    SerializedObject& so = *it;
    if(so.object_ && so.object_->GetRefs() == 1) {
      delete so.object_;
    }
  }
}

Object* BinaryDeserializer::Deserialize (Stream* stream, Object* root, const char* filename) {
  noz_uint32 header_pos = stream->GetPosition();

  // Read in the header..
  BinarySerializer::Header header;
  if(sizeof(header)!=stream->Read((char*)&header, 0, sizeof(header))) {
    // TODO: report error
    return nullptr;
  }

  // Ensure the four CC code matches.
  if(header.fourcc_ != BinarySerializer::FourCC) {
    // TODO: report error;
    return nullptr;
  }

  BinaryReader reader(stream);
  reader_ = &reader;
  filename_ = filename ? filename : (
    stream->IsTypeOf(typeof(FileStream)) ? ((FileStream*)stream)->GetFilename() : "<unknown>");

  // Read the name table
  names_.clear();
  names_.push_back(Name::Empty);

  if(header.name_table_count_) {
    names_.reserve(header.name_table_count_+1);
    stream->Seek(header_pos + header.name_table_offset_,SeekOrigin::Begin);
    BinaryReader reader(stream);
    for(noz_uint32 i=0; i<header.name_table_count_; i++) {
      names_.push_back(reader.ReadName());
    } 
  }

  // Read the object table.
  if(header.object_table_count_) {
    objects_.resize(header.object_table_count_);
    queue_.reserve(header.object_table_count_);
    stream->Seek(header_pos + header.object_table_offset_,SeekOrigin::Begin);
    BinaryReader reader(stream);
    for(noz_uint32 i=0;i<header.object_table_count_; i++) {
      SerializedObject& so = objects_[i];
      so.type_ = Type::FindType(ReadName());
      so.offset_ = reader.ReadUInt32();      
      if(so.offset_ == 0) {
        noz_uint64 horder = reader.ReadUInt64();
        noz_uint64 lorder = reader.ReadUInt64();
        so.asset_guid_ = Guid(horder,lorder);
      }
    }
  }

  // Find the root object and add it to the queue
  SerializedObject* root_so = GetSerializedObject(header.root_,root);
  if(nullptr != root_so) {
    root = root_so->object_;

    // Deserialize all objects
    for(noz_uint32 o=0; o<queue_.size() && !HasError(); o++) {
      SerializedObject* so = queue_[o];
      noz_assert(so);
      noz_assert(so->object_);

      Type* otype = so->object_->GetType();
      noz_assert(otype);

      // Skip objects with no offset (Assets)
      if(so->offset_ == 0) continue;

      so->object_->OnDeserializing();

      // Seek to object start.
      stream->Seek(header_pos + so->offset_, SeekOrigin::Begin);
      peek_ = BinarySerializer::DataType::Unknown;
    
      // Read properties
      while(!HasError()) {
        // Read property name 
        const Name& property_name = ReadName();

        // Empty property name signifies an end of properties
        if(property_name==Name::Empty) break;

        // Read property version
        noz_uint32 property_version = reader.ReadUInt32();

        // Read Property data size
        noz_uint32 property_data_size = reader.ReadUInt32();

        // Save position prior to reading property
        noz_uint32 property_pos = stream->GetPosition();

        // Find the property in the object
        Property* p = otype->GetProperty(property_name);

        // Property no longer exists?
        if(nullptr==p) {
          stream->Seek(property_pos + property_data_size, SeekOrigin::Begin);
          peek_ = BinarySerializer::DataType::Unknown;
          continue;
        }

        // Deserialize the property
        p->Deserialize(so->object_, *this);
      
        // Seek past the property data..
        stream->Seek(property_pos + property_data_size, SeekOrigin::Begin);
        peek_ = BinarySerializer::DataType::Unknown;
      }
    }

    for(noz_uint32 o=0; o<queue_.size(); o++) {
      SerializedObject* so = queue_[o];
      noz_assert(so);

      // It is possible the object got deleted..
      if(nullptr == so->object_) continue;

      if(so->object_->IsDeserializing()) so->object_->OnDeserialized();
    }
  }

  reader_ = nullptr;

  return root;
}

BinaryDeserializer::SerializedObject* BinaryDeserializer::GetSerializedObject(noz_uint32 id, Object* o) {
  if(id==0) return nullptr;
  if(id>objects_.size()) return nullptr;

  SerializedObject& so = objects_[id-1];
  if(so.object_) return &so;

  if(o) {
    // Queue the object for deserialization if this was the first time it was seen
    if(so.object_ != o) {
      queue_.push_back(&so);
      so.object_ = o;
    }
    return &so;
  }

  if(!so.asset_guid_.IsEmpty()) {
    so.object_ = AssetManager::LoadAsset(so.type_, so.asset_guid_);
    return &so;
  }

  if(nullptr == so.type_) return nullptr;

  so.object_ = so.type_->CreateInstance();
  if(nullptr == so.object_) {
    return &so;
  }

  queue_.push_back(&so);
  return &so; 
}

Object* BinaryDeserializer::ReadObject (Object* o) {
  noz_assert(reader_);

  noz_uint32 id = reader_->ReadUInt32();
  SerializedObject* so = GetSerializedObject(id,o);
  if(nullptr == so) return nullptr;
  return so->object_;
}

const Name& BinaryDeserializer::ReadName (void) {
  noz_uint32 id = reader_->ReadUInt32();
  if(id>names_.size()) return Name::Empty;
  return names_[id];
}

BinarySerializer::DataType BinaryDeserializer::ReadDataType(void) {
  if(peek_ != BinarySerializer::DataType::Unknown) {
    BinarySerializer::DataType dt = peek_;
    peek_ = BinarySerializer::DataType::Unknown;
    return dt;
  }
  return (BinarySerializer::DataType)reader_->ReadByte();
}

BinarySerializer::DataType BinaryDeserializer::PeekDataType(void) {
  if(peek_ != BinarySerializer::DataType::Unknown) return peek_;
  peek_ = ReadDataType();
  return peek_;
}

bool BinaryDeserializer::PeekEndObject (void) {
  return PeekDataType() == BinarySerializer::DataType::EndObject;
}
    
bool BinaryDeserializer::PeekEndArray (void) {
  return PeekDataType() == BinarySerializer::DataType::EndArray;
} 

bool BinaryDeserializer::PeekValueNull(void) {
  return PeekDataType() == BinarySerializer::DataType::ValueNull;
}

bool BinaryDeserializer::PeekValueFloat (void) {
  return PeekDataType() == BinarySerializer::DataType::ValueFloat;
}

bool BinaryDeserializer::ReadStartObject (void) {
  return ReadDataType() == BinarySerializer::DataType::StartObject;
}

bool BinaryDeserializer::ReadEndObject (void) {
  return ReadDataType() == BinarySerializer::DataType::EndObject;
}

bool BinaryDeserializer::ReadStartArray (void) {
  return ReadDataType() == BinarySerializer::DataType::StartArray;
}

bool BinaryDeserializer::ReadStartSizedArray (noz_uint32& size) {  
  if(ReadDataType() != BinarySerializer::DataType::StartSizedArray) return false;
  size = reader_->ReadUInt32();  
  return true;
}

bool BinaryDeserializer::ReadEndArray (void) {
  return ReadDataType() == BinarySerializer::DataType::EndArray;
}

bool BinaryDeserializer::ReadMember (Name& v) {
  if(ReadDataType() != BinarySerializer::DataType::Member) return false;
  v = ReadName();
  return true;
}

bool BinaryDeserializer::ReadValueString (String& v) {
  if(ReadDataType() != BinarySerializer::DataType::ValueString) return false;
  v = reader_->ReadString();
  return true;
}

bool BinaryDeserializer::ReadValueName (Name& v) {
  if(ReadDataType() != BinarySerializer::DataType::ValueName) return false;
  v = ReadName();
  return true;
}

bool BinaryDeserializer::ReadValueInt32 (noz_int32& v) {
  if(ReadDataType() != BinarySerializer::DataType::ValueInt32) return false;
  v = reader_->ReadInt32();
  return true;
}

bool BinaryDeserializer::ReadValueUInt32 (noz_uint32& v) {
  if(ReadDataType() != BinarySerializer::DataType::ValueUInt32) return false;
  v = reader_->ReadUInt32();
  return true;
}

bool BinaryDeserializer::ReadValueFloat (noz_float& v) {
  if(ReadDataType() != BinarySerializer::DataType::ValueFloat) return false;
  v = reader_->ReadFloat();
  return true;
}

bool BinaryDeserializer::ReadValueNull (void) {
  return ReadDataType() == BinarySerializer::DataType::ValueNull;
}

bool BinaryDeserializer::ReadValueBoolean (bool& v) {
  if(ReadDataType() != BinarySerializer::DataType::ValueBoolean) return false;
  v = reader_->ReadBoolean();
  return true;
}

bool BinaryDeserializer::ReadValueColor (Color& v) {
  if(ReadDataType() != BinarySerializer::DataType::ValueColor) return false;
  v.raw = reader_->ReadUInt32();
  return true;
}

bool BinaryDeserializer::ReadValueByte (noz_byte& v) {
  if(ReadDataType() != BinarySerializer::DataType::ValueByte) return false;
  v = reader_->ReadByte();
  return true;
}

bool BinaryDeserializer::ReadValueBytesSize (noz_uint32& size) {
  if(ReadDataType() != BinarySerializer::DataType::ValueBytesSize) return false;
  bytes_size_ = size = reader_->ReadUInt32();
  return true;
}

bool BinaryDeserializer::ReadValueBytes (noz_byte* b) {
  if(ReadDataType() != BinarySerializer::DataType::ValueBytes) return false;
  reader_->Read((char*)b, 0, bytes_size_);
  return true;
}

bool BinaryDeserializer::ReadValueObject (Object*& o,Type* t) {
  noz_assert(t);

  if(PeekDataType() == BinarySerializer::DataType::ValueGuid) {
    Guid asset_guid;
    if(!ReadValueGuid(asset_guid)) return false;
    o = AssetManager::LoadAsset(t,asset_guid);
    return true;
  }

  if(ReadDataType() != BinarySerializer::DataType::ValueObject) return false;

  // Read the object from the stream.
  o = ReadObject(o);

  // If the object is not the correct type then just return nullptr.  The object
  // itself will be cleaned up when the deserializer is freed.
  if(o != nullptr && !o->IsTypeOf(t)) {
    o = nullptr;
  }

  return true;
}

bool BinaryDeserializer::ReadValueGuid (Guid& guid) {
  if(ReadDataType() != BinarySerializer::DataType::ValueGuid) return false;
  noz_uint64 horder = reader_->ReadUInt64();
  noz_uint64 lorder = reader_->ReadUInt64();
  guid = Guid(horder,lorder);
  return true;
}

void BinaryDeserializer::ReportError (const char* msg) {
  if(error_) return;

  Console::WriteLine("%s: error: %s", filename_.ToCString(), msg);
}

void BinaryDeserializer::ReportWarning (const char* msg) {
  if(error_) return;
  
  Console::WriteLine("%s: warning: %s", filename_.ToCString(), msg);  
}

