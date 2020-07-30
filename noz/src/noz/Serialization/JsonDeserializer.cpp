
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/StreamReader.h>

#include "JsonDeserializer.h"

using namespace noz;

static const Name NameId ("_Id");
static const Name NameType ("_Type");
static const Name NameRoot ("Root");
static const Name NameObjects ("Objects");


Object* JsonDeserializer::Deserialize (Stream* stream, Object* root) {
  String filename = 
    stream->IsTypeOf(typeof(FileStream)) ? ((FileStream*)stream)->GetFilename() : "<unknown>";

  StreamReader sreader(stream);
  JsonReader reader(&sreader, filename.ToCString());
  reader_ = &reader;

  noz_uint32 root_id = DeserializeObjectMap(reader);

  if(reader.HasError()) {
    reader_ = nullptr;
    return nullptr;
  }

  if(root_id==0) {
    return nullptr;
  }

  auto it=objects_.find(root_id);
  if(it==objects_.end()) {
    return nullptr;
  }

  // Fill in root.
  if(root) {
    if(!root->IsTypeOf(it->second.type_)) return nullptr;
    it->second.object_ = root;
  }

  // Empty queue
  queue_.push_back(&it->second);
  for(size_t q=0; q<queue_.size(); q++) {
    if(!Deserialize(reader,queue_[q])) break;
  }  

  for(size_t q=0; q<queue_.size(); q++) {
    SerializedObject* so = queue_[q];
    noz_assert(so);
    noz_assert(so->object_);

    if(so->object_->IsDeserializing()) so->object_->OnDeserialized();
  }  

  if(reader.HasError()) {
    return nullptr;
  }

  return it->second.object_;
}

noz_uint32 JsonDeserializer::DeserializeObjectMap(JsonReader& reader) {  
  noz_uint32 root_id = 0;

  if(!reader.ReadStartObject()) return 0;

  while(!reader.IsEOS() && !reader.IsEndObject()) {
    // Read member name.
    Name member;
    if(!reader.ReadMember(member)) break;

    // Root identifier..
    if(member == NameRoot) {      
      if(!reader.ReadValueUInt32(root_id)) break;
      continue;
    }

    // Object array.
    if(member == NameObjects) {
      if(!reader.ReadStartArray()) break;

      while(!reader.IsEOS() && !reader.IsEndArray()) {
        noz_uint32 id;
        Type* type;

        // Save a marker prior to reading the object.
        noz_uint32 marker = reader.AddMarker();

        if(!reader.ReadStartObject()) break;
        while(!reader.IsEOS() && !reader.IsEndObject()) {
          Name member;
          if(!reader.ReadMember(member)) break;
          if(member == NameId) {
            if(!reader.ReadValueUInt32(id)) break;
            continue;
          }
          if(member == NameType) {
            String type_name;
            if(!reader.ReadValueString(type_name)) break;
            type = Type::FindType(type_name);
            if(type==nullptr) {
              ReportWarning(String::Format("unknown type '%s'", type_name.ToCString()).ToCString());;
            }
            continue;
          }
          reader_->SkipValue();
        }

        // Add a serialized object for the 
        SerializedObject& so = objects_[id];
        so.type_ = type;
        so.id_ = id;
        so.marker_ = marker;

        if(!reader.ReadEndObject()) break;
      }

      if(!reader.ReadEndArray()) break;
      continue;
    }

    if(!reader.SkipValue()) break;
  }

  if(!reader.ReadEndObject()) return 0;

  return root_id;
}

bool JsonDeserializer::Deserialize(JsonReader& reader, SerializedObject* so) {
  noz_assert(so);
  noz_assert(so->type_);

  // Allocate the object
  if(so->object_ == nullptr) {
    so->object_ = so->type_->CreateInstance();
    if(nullptr == so->object_) {
      // TODO: report error
      return false;
    }
  }

  Object* o = so->object_;
  Type* otype = o->GetType();

  // Move the reader back to marker for object
  reader.SeekMarker(so->marker_);

  if(!reader.ReadStartObject()) return false;

  o->OnDeserializing();

  bool error = false;
  while(!error && !reader.IsEOS() && !reader.IsEndObject()) {
    Name member;
    if(!reader.ReadMember(member)) {error = true; continue;}

    // Skip id and type
    if(member==NameType || member==NameId) {
      if(!reader.SkipValue()) error = true; 
      continue;
    }

    // Find the property within the type.
    Property* prop = otype->GetProperty(member);
    if(nullptr == prop) {
      if(!reader.SkipValue()) error=true;
      // TODO: report warning
      continue;
    }

    // Deserialize its value
    if(!prop->Deserialize(so->object_, *this)) { error = true; continue; }
  }

  if(error) return false;

  return reader.ReadEndObject();
}


Object* JsonDeserializer::DeserializeObject (noz_uint32 id, Object* o) {
  if(id==0) return nullptr;

  auto it = objects_.find(id);
  if(it == objects_.end()) {
    return nullptr;
  }

  if(it->second.object_) {
    // If an object was given ensure it matches.    
    if(o && it->second.object_ != o) {
      NOZ_TODO("report error for invalid object")
      return nullptr;
    }

    return it->second.object_;
  }

  if(o) {
    it->second.object_ = o;
  } else if(nullptr==it->second.type_) {
    return nullptr;
  } else {
    it->second.object_ = it->second.type_->CreateInstance();
    if(nullptr == it->second.object_) {
      NOZ_TODO("report warning for CreateInstance failure")
      return nullptr;
    }
  }

  // Queue the object for deserialization
  queue_.push_back(&it->second);

  return it->second.object_;
}

bool JsonDeserializer::PeekEndObject (void) {
  return reader_->IsEndObject();
}
    
bool JsonDeserializer::PeekEndArray (void) {
  return reader_->IsEndArray();
}

bool JsonDeserializer::PeekValueNull(void) {
  return reader_->IsValueNull();
}

bool JsonDeserializer::PeekValueFloat (void) {
  return reader_->IsValueNumber();
}

bool JsonDeserializer::ReadStartObject (void) {
  return reader_->ReadStartObject();
}

bool JsonDeserializer::ReadEndObject (void) {
  return reader_->ReadEndObject();
}

bool JsonDeserializer::ReadStartArray (void) {
  return reader_->ReadStartArray();
}

bool JsonDeserializer::ReadStartSizedArray (noz_uint32& size) {
  if(!reader_->ReadStartArray()) return false;
  noz_uint32 marker = reader_->AddMarker();
  noz_uint32 c;
  for(c=0;!reader_->IsEndArray();c++) {
    if(!reader_->SkipValue()) return false;
  }

  reader_->SeekMarker(marker);
  size = c;
  return true;
}

bool JsonDeserializer::ReadEndArray (void) {
  return reader_->ReadEndArray();;
}

bool JsonDeserializer::ReadMember (Name& v) {
  return reader_->ReadMember(v);
}

bool JsonDeserializer::ReadValueString (String& v) {
  return reader_->ReadValueString(v);
}

bool JsonDeserializer::ReadValueName (Name& v) {
  String s;
  if(!reader_->ReadValueString(s)) return false;
  v = s;
  return true;
}

bool JsonDeserializer::ReadValueInt32 (noz_int32& v) {
  return reader_->ReadValueInt32(v);
}

bool JsonDeserializer::ReadValueUInt32 (noz_uint32& v) {
  return reader_->ReadValueUInt32(v);
}

bool JsonDeserializer::ReadValueFloat (noz_float& v) {
  return reader_->ReadValueFloat(v);
}

bool JsonDeserializer::ReadValueNull (void) {
  return reader_->ReadValueNull();
}

bool JsonDeserializer::ReadValueBoolean (bool& v) {
  return reader_->ReadValueBoolean(v);
}

bool JsonDeserializer::ReadValueColor (Color& v) {
  String s;
  if(!reader_->ReadValueString(s)) return false;
  v = Color::Parse(s);
  return true;
}

bool JsonDeserializer::ReadValueByte (noz_byte& v) {
  return reader_->ReadValueByte(v);
}

bool JsonDeserializer::ReadValueBytesSize (noz_uint32& size) {
  noz_assert(false);
  return false;
}

bool JsonDeserializer::ReadValueBytes (noz_byte* b) {
  noz_assert(false);
  return false;
}

bool JsonDeserializer::ReadValueObject (Object*& o, Type* type) {
  noz_uint32 id;
  if(reader_->IsValueString()) {
    String name;
    if(!reader_->ReadValueString(name)) return false;
    name = name.Trim();

    // GUID?
    if(name[0] == '{') {
      o = AssetManager::LoadAsset(type,Guid::Parse(name));
    } else {
      o = AssetManager::LoadAsset(type,name);
    }
    return true;
  }
  if(!reader_->ReadValueUInt32(id)) return false;
  o = DeserializeObject(id, o);
  return true;
}

void JsonDeserializer::ReportError (const char* msg) {
  reader_->ReportError(msg);  
}

void JsonDeserializer::ReportWarning (const char* msg) {
  reader_->ReportWarning(msg);  
}

bool JsonDeserializer::ReadValueGuid (Guid& guid) {
  String guid_str;
  if(!reader_->ReadValueString(guid_str)) {
    guid = Guid::Empty;
    return false;
  }
  guid = Guid::Parse(guid_str);
  return true;
}
