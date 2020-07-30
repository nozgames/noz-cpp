///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "StringReader.h"

using namespace noz;


StringReader::StringReader(const char* value) {
  value_ = String(value);
  position_ = 0;
}

StringReader::StringReader(String value) {
  value_ = value;
  position_ = 0;
}

noz_int32 StringReader::Read(char* buffer, noz_int32 index, noz_int32 count) {
  count = Math::Min(count,noz_int32(value_.GetLength()-position_));
  if(count==0) {
    return 0;
  }

  memcpy(buffer+index,value_.ToCString()+position_,count);
  position_ += count;
  return count;
}
