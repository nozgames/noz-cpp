///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Math.h>
#include <noz/Guid.h>
#include <noz/Platform/CursorHandle.h>

#include <mach/mach_time.h>

using namespace noz;


noz_float noz::Time::GetApplicationTime(void) {
  const int64_t kOneMillion = 1000 * 1000;
  static mach_timebase_info_data_t s_timebase_info;

  if (s_timebase_info.denom == 0) {
    (void) mach_timebase_info(&s_timebase_info);
  }

  // mach_absolute_time() returns billionth of seconds,
  // so divide by one million to get milliseconds
  noz_uint64 t = (noz_uint64)((mach_absolute_time() * s_timebase_info.numer) / (kOneMillion * s_timebase_info.denom));
  
  static noz_uint64 start = 0;
  if(start==0) {
    start = t;
  }
    
  return (t-start) / 1000.0f;
}


Guid Guid::Generate (void) {
  return Guid(0,0);
}


Platform::CursorHandle* Platform::CursorHandle::CreateInstance(Cursor* cursor) {
  return nullptr;
}
