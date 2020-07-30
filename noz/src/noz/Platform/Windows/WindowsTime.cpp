///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

#include <Windows.h>

using namespace noz;

noz_float noz::Time::GetApplicationTime(void) {
  static noz_uint64 start = 0;
  static noz_double frequency = 0;
  
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);

  if(start==0) {
    start = li.QuadPart;

    LARGE_INTEGER li2;
    QueryPerformanceFrequency(&li2);
    frequency = 1.0 / noz_double(li2.QuadPart);
  }

  return noz_float(noz_double(li.QuadPart-start) * frequency);
}