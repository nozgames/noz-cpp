///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryReader.h>
#include <noz/IO/BinaryWriter.h>

using namespace noz;

Byte::Byte (noz_byte value) {
  value_ = value;  
}

Byte::Byte (void) {
  value_ = 0;
}

String Byte::ToString(void) const {
  char buffer[64];
#if defined(NOZ_WINDOWS)
  _itoa_s(value_,buffer,10);
#else
  sprintf(buffer,"%d",value_);
#endif
  return String(buffer);
}

Byte Byte::Parse (const char* s) {
  return (noz_byte)Int32::Parse(s);
}

