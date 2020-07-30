///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <math.h>

using namespace noz;
using namespace noz;

const noz_double Double::NaN = log(0);

Double::Double (noz_double value) {
  value_ = value;
}

Double::Double (void) {
  value_ = 0;
}

String Double::ToString(void) const {
  char buffer[1024];
#if defined(NOZ_WINDOWS)  
  sprintf_s(buffer,1024,"%f",value_);
#else
  sprintf(buffer,"%f",value_);
#endif
  return String(buffer);
}
