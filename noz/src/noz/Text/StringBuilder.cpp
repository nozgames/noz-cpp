///////////////////////////////////////////////////////////////////////////////
// noZ C-Sharp Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "StringBuilder.h"

using namespace noz;

StringBuilder::StringBuilder(void) {
}

StringBuilder::StringBuilder(const char* value) {
  Append(value);
}

StringBuilder::StringBuilder(String value) {
  Append(value);
}

void StringBuilder::Clear(void) {
  value_.clear();
}

String StringBuilder::ToString (void) const {
  if(value_.empty()) {
    return String();
  }
  return String(&value_[0], 0, (noz_int32)value_.size());
}

void StringBuilder::Append(char value) {
  value_.push_back(value);
}

void StringBuilder::Append(char value, noz_int32 count) {
  value_.reserve(value_.size()+count);
  for(;count>0;count--) value_.push_back(value);
}

void StringBuilder::Append(const char* value) {
  if(nullptr==value || !*value) return;
  noz_uint32 size = (noz_uint32)strlen(value);
  noz_uint32 offset = (noz_uint32)value_.size();
  value_.resize(value_.size()+size);
  memcpy(&value_[offset],value,size);
}

void StringBuilder::Append(const char* value, noz_int32 length) {
  if(nullptr==value || !*value) return;
  noz_uint32 size = length;
  noz_uint32 offset = (noz_uint32)value_.size();
  value_.resize(value_.size()+size);
  memcpy(&value_[offset],value,size);
}

StringBuilder& StringBuilder::Append(String value) {
  if(!value.IsEmpty()) {
    noz_uint32 offset = (noz_uint32)value_.size();
    value_.resize(value_.size()+value.GetLength());
    memcpy(&value_[offset],value.value_,value.GetLength());
  }
  return *this;
}

StringBuilder& StringBuilder::Insert(noz_int32 index, String value) {
  if(!value.IsEmpty()) {
    value_.insert(value_.begin()+index,value.GetLength(),0);
    memcpy(&value_[index],value.value_,value.GetLength());
  }
  return *this;
}

StringBuilder& StringBuilder::Insert(noz_int32 index, char value) {
  value_.insert(value_.begin()+index,value);
  return *this;
}

