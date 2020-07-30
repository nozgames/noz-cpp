///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Guid.h"

using namespace noz;

Guid Guid::Empty;

Guid::Guid(void) {
  value_h_ = value_l_ = 0;
}

Guid::Guid(noz_uint64 h, noz_uint64 l) {
  value_h_ = h;
  value_l_ = l;
}

Guid Guid::Parse(const char* p) {
  Guid result;
  if(*p == '{') p++;

  for(noz_int32 shift=60; *p && *p != '}' && shift>=0; p++) {
    if(*p == '-') continue;
    result.value_h_ |= (((noz_uint64)Math::Hex2Dec(*p))<<shift);
    shift-=4;
  }
  for(noz_int32 shift=60; *p && *p != '}' && shift>=0; p++) {
    if(*p == '-') continue;
    result.value_l_ |= (((noz_uint64)Math::Hex2Dec(*p))<<shift);
    shift-=4;
  }
  return result;
}

String Guid::ToString(void) const {
  return String::Format("%08X-%04X-%04X-%04X-%012llX",
    (noz_uint32)(value_h_ >> 32),
    (noz_uint32)((value_h_ >> 16) & 0xFFFF), 
    (noz_uint32)(value_h_ & 0xFFFF), 
    (noz_uint32)((value_l_ >> 48) & 0xFFFF), 
    (value_l_ & 0xFFFFFFFFFFFFL)
  );
}
