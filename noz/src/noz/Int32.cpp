///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <stdlib.h>

using namespace noz;
using namespace noz;

Int32::Int32 (noz_int32 value) {
  value_ = value;  
}

Int32::Int32 (void) {
  value_ = 0;
}

String Int32::ToString(void) const {
  char buffer[64];
#if defined(NOZ_WINDOWS)
  _itoa_s(value_,buffer,10);
#else
  sprintf(buffer,"%d",value_);
#endif
  return String(buffer);
}

noz_int32 Int32::Parse(String s) {
  return Parse(s.ToCString());
}

noz_int32 Int32::Parse(const char* s) {
  if(s[0]=='0' && (s[1]=='x' || s[1]=='X')) {
    return (noz_int32) strtol(s+2, (char **)NULL, 16);
  }
  return (noz_int32) strtol(s, (char **)NULL, 10);
}


#if 0
noz_uint64 Int32::ToUInt64(IFormatProvider* provider) {
  noz_assert(provider==nullptr);
  return value_;
}

noz_int64 Int32::ToInt64(IFormatProvider* provider) {
  noz_assert(provider==nullptr);
  return value_;
}
#endif
