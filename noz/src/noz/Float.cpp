///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <math.h>

using namespace noz;

const noz_float Float::NaN = logf(0);
const noz_float Float::PI = 3.14159265358979323846f;
const noz_float Float::Infinity = INFINITY;
const noz_float Float::PI_180 = Float::PI / 180.0f;
const noz_float Float::PI_2 = Float::PI * 0.5f;
const noz_float Float::M_180_PI = 180.0f / Float::PI;

Float::Float (noz_float value) {
  value_ = value;
}

Float::Float (void) {
  value_ = 0;
}

bool Float::Equals (Object* v) const {
  if(v==nullptr) return false;
  if(v==this) return true;

  switch(v->GetTypeCode()) {
    case TypeCode::Float: return *((Float*)v) == value_;
    case TypeCode::UInt32: return *((UInt32*)v) == (noz_uint32)value_;
    case TypeCode::Int32: return *((Int32*)v) == (noz_int32)value_;
    case TypeCode::Double: return *((Double*)v) == (noz_double)value_;
    case TypeCode::String: return Float::Parse(((StringObject*)v)->GetValue()) == value_;
    default:
      break;
  }

  return false;
}

noz_float Float::Parse(String s) {
  if(!s.CompareTo("NaN")) {
    return Float::NaN;
  }

  if(!s.CompareTo("Infinity") || !s.CompareTo("INF")) {
    return Float::Infinity;
  }

  return Parse(s.ToCString());
}

noz_float Float::Parse (const char* s) {
  noz_int32 len=(noz_int32)strlen(s);
  if(len==0) {
    return 0.0;
  }
  if(len < 127 && s[len-1]=='%') {
    char temp[128];
#ifdef NOZ_WINDOWS
    strcpy_s (temp, 128, s);
#else
    strcpy (temp, s);
#endif
    temp[len-1]=0;
    return (noz_float)atof(temp) / 100.0f;
  }
  
  return (noz_float)atof(s);
}

String Float::ToString(void) const {
  if(value_ == Float::Infinity) return "Infinity";
  if(value_ == Float::NaN) return "NaN";

  String result = String::Format("%8.3f", value_).Trim();

  // Trim off trailing zeros and even the decimal place if an integer.
  noz_int32 dot = result.LastIndexOf('.');
  if(dot != -1) {
    noz_int32 trim = result.GetLength()-1;
    for(;trim>dot && result[trim]=='0';trim--);
    if(trim>=dot) {
      if(trim == dot) trim--;
      result = result.Substring(0,trim+1);
    }
  }

  return result;
}
