///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "DateTime.h"

using namespace noz;

const DateTime DateTime::MinValue (DateTime::MinTicks);
const DateTime DateTime::MaxValue (DateTime::MaxTicks);


DateTime::DateTime (noz_uint64 ticks) {
  data_ = ticks;
}

DateTime::DateTime (void) {
  data_ = 0;
}

noz_int32 DateTime::CompareTo (const DateTime& dt) const {
  noz_uint64 ticks1 = GetTicks();
  noz_uint64 ticks2 = dt.GetTicks();
  if(ticks1 > ticks2) return 1;
  if(ticks1 < ticks2) return -1;
  return 0;
}
