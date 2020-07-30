///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/BinarySerializer.h>
#include <noz/Serialization/JsonSerializer.h>
#include "UInt32.h"
#include <stdlib.h>

using namespace noz;

UInt32::UInt32 (noz_uint32 value) {
  value_ = value;  
}

UInt32::UInt32 (void) {
  value_ = 0;
}

String UInt32::ToString(void) const {
  char buffer[64];
#if defined(NOZ_WINDOWS)
  sprintf_s(buffer,64,"%u",value_);
#else
  sprintf(buffer,"%u",value_);
#endif
  return String(buffer);
}

noz_uint32 UInt32::Parse(String s) {
  return Parse(s.ToCString());
}

noz_uint32 UInt32::Parse(const char* s) {
  if(s[0]=='0' && (s[1]=='x' || s[1]=='X')) {
    return (noz_uint32) strtol(s+2, (char **)NULL, 16);
  }
  return (noz_uint32) strtol(s, (char **)NULL, 10);
}

