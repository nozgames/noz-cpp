///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "JsonString.h"

using namespace noz;

JsonString::JsonString(void) : JsonValue(JsonType::String) {
}

JsonString::JsonString(const String& value) : JsonValue(JsonType::String) {
  value_ = value;
}

JsonValue* JsonString::Clone (void) const {
  JsonString* clone = new JsonString;
  clone->value_ = value_;
  return clone;
}

bool JsonString::CloneInto (JsonValue* _dst, bool overwrite) const {
  noz_assert(_dst);

  // If not overwriting there is nothing to do.
  if(!overwrite) return false;

  // String objects can only clone into other string objects
  if(!_dst->IsString()) return false;

  JsonString* dst = (JsonString*)_dst;
  dst->value_ = value_;

  return true;
}
