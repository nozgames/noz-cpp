///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "JsonObject.h"
#include "JsonString.h"

using namespace noz;

JsonObject::JsonObject(void) : JsonValue (JsonType::Object) {
}

JsonObject::~JsonObject(void) {
  Clear();
}

void JsonObject::Clear(void) {
  for(auto it=values_.begin(); it!=values_.end(); it++) {
    delete it->second;
  }
  values_.clear();
}

bool JsonObject::GetBoolean (const Name& key, bool default_value) const {
  JsonString* v = Get<JsonString>(key);
  if(nullptr == v) return default_value;
  return Boolean::Parse(v->Get());
}

noz_int32 JsonObject::GetInt32 (const Name& key, noz_int32 default_value) const {
  JsonString* v = Get<JsonString>(key);
  if(nullptr == v) return default_value;
  return Int32::Parse(v->Get());
}

noz_float JsonObject::GetFloat (const Name& key, noz_float default_value) const {
  JsonString* v = Get<JsonString>(key);
  if(nullptr == v) return default_value;
  return Float::Parse(v->Get());
}

String JsonObject::GetString (const Name& key) const {
  JsonString* v = Get<JsonString>(key);
  if(nullptr == v) return String::Empty;
  return v->Get();
}

JsonValue* JsonObject::Get (const Name& key) const {
  auto it = values_.find(key);
  if(it != values_.end()) {
    return it->second;
  }
  return nullptr;
}

void JsonObject::Remove(const Name& key) {
  auto it = values_.find(key);
  if(it!=values_.end()) {
    delete it->second;
    values_.erase(it);
  }
}

void JsonObject::Set(const Name& key, JsonValue* value) {
  Remove(key);
  values_[key] = value;
}

void JsonObject::Add(const Name& key, JsonValue* value) {
  Remove(key);
  values_[key] = value;
}

void JsonObject::Add(const Name& key, const char* value) {
  JsonString* str = new JsonString;
  str->Set(value);
  Add(key,str);
}

void JsonObject::Add(const Name& key, const String& value) {
  JsonString* str = new JsonString;
  str->Set(value);
  Add(key,str);
}

void JsonObject::SetString (const Name& key, const String& v) {
  JsonString* str = Get<JsonString>(key);
  if(str) {
    str->Set(v);
  } else {
    JsonString* str = new JsonString;
    str->Set(v);
    Add(key,str);  
  }
}

void JsonObject::SetString (const Name& key, const char* v) {
  SetString(key,String(v));
}

void JsonObject::SetInt32 (const Name& key, noz_int32 v) {
  SetString(key,Int32(v).ToString());
}

void JsonObject::SetBoolean (const Name& key, bool v) {
  SetString(key,Boolean(v).ToString());
}

JsonValue* JsonObject::Clone (void) const {
  JsonObject* clone = new JsonObject;
  for(auto it=values_.begin(); it!=values_.end(); it++) {
    clone->Add(it->first,it->second->Clone());
  }
  return clone;
}

bool JsonObject::CloneInto (JsonValue* _dst, bool override) const {
  noz_assert(_dst);

  // Object values can only be cloned into another object value..
  if(!_dst->IsObject()) return false;

  JsonObject* dst = (JsonObject*)_dst;

  bool result = true;

  for(auto it=values_.begin(); it!=values_.end(); it++) {
    JsonValue* dv = (*dst)[it->first];
    JsonValue* sv = it->second;

    // If the destination object does not contain this key...        
    if(nullptr==dv) {
      // Clone the value and add it to destination object
      dst->Add(it->first,sv->Clone());

    // If the destination already exists clone into it
    } else {
      result &= sv->CloneInto(dv,override);
    }
  }

  return result;
}
