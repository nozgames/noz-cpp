///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "JsonArray.h"

using namespace noz;

JsonArray::JsonArray(void) : JsonValue(JsonType::Array) {
}

JsonArray::~JsonArray (void) {
  Clear();
}

void JsonArray::Clear(void) {
  for(auto it=values_.begin(); it!=values_.end(); it++) {
    delete (*it);
  }
  values_.clear();
}

void JsonArray::Add (JsonValue* value) {
  values_.push_back(value);
}

JsonValue* JsonArray::Clone (void) const {
  JsonArray* clone = new JsonArray;
  for(auto it=values_.begin(); it!=values_.end(); it++) {
    clone->Add((*it)->Clone());
  }
  return clone;
}

bool JsonArray::CloneInto (JsonValue* _dst, bool overwrite) const {
  noz_assert(_dst);

  // Array objects can only clone into array objects
  if(!_dst->IsArray()) return false;

  JsonArray* dst = (JsonArray*)_dst;
  
  for(auto it=values_.begin(); it!=values_.end(); it++) {
    dst->Add((*it)->Clone());
  }

  return true;
}
