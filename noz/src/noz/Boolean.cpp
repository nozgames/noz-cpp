///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;

Boolean::Boolean (bool value) {
  value_ = value;  
}

Boolean::Boolean (void) {
  value_ = 0;
}

String Boolean::ToString(void) const {
  return value_ ? "true" : "false";
}

bool Boolean::Parse(const char* s) {
#if defined(NOZ_WINDOWS)
  if(!_stricmp(s,"true")) return true;
  if(!_stricmp(s,"false")) return false;
  if(!_stricmp(s,"yes")) return true;
  if(!_stricmp(s,"no")) return false;
#else
  if(!strcasecmp(s,"true")) return true;
  if(!strcasecmp(s,"false")) return false;
  if(!strcasecmp(s,"yes")) return true;
  if(!strcasecmp(s,"no")) return false;
#endif
  return !!Int32::Parse(s);
}

