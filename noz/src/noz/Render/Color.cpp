///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;

const Color Color::Black(0,0,0,255);
const Color Color::White(255,255,255,255);
const Color Color::Red(255,0,0,255);
const Color Color::Green(0,255,0,255);
const Color Color::Blue(0,0,255,255);

Color::Color(const Vector4& v) : Color(v.x,v.y,v.z,v.w) {
}

String Color::ToString (void) const {
  return String::Format("#%02x%02x%02x%02x", r, g, b, a);
}

Color Color::Parse(const char* s) {
  // Skip optional pound
  if(s[0]=='#') s++;

  // Convert the hex string to a number and pad it with 0xF    
  noz_uint32 mask = 0;
  noz_uint32 raw = Math::Hex2Dec(s,&mask);  
  noz_uint32 shift = Math::CountLeadingZeroBits(mask);
  raw = (0xFFFFFFFF & ~(mask<<shift)) | (raw<<shift);

  // Generate the color
  return Color((noz_byte)((raw>>24)&0xFF), (noz_byte)((raw>>16)&0xFF), (noz_byte)((raw>>8)&0xFF), (noz_byte)(raw&0xFF));
}

Vector4 Color::ToVector4(void) const {
  return Vector4 (
    noz_float(r) / 255.0f,
    noz_float(g) / 255.0f,
    noz_float(b) / 255.0f,
    noz_float(a) / 255.0f
  );
}
