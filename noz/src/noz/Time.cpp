///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;

noz_float Time::delta_time_ = 0.0f;

noz_float Time::time_ = 0.0f;

noz_float Time::fixed_time_ = 0.0f;

bool Time::fixed_frame_ = false;

noz_float Time::fixed_delta_time_ = 0.02f;

noz_float Time::average_delta_time_ = 0.0f;


void Time::BeginFrame(void) {
  noz_float t = GetApplicationTime();
  delta_time_ = t - time_;
  noz_assert(delta_time_>=0.0f);
  time_ = t;
  fixed_frame_ = false;  

  // smooth the averge time.
  average_delta_time_ = delta_time_ * 0.1f + average_delta_time_ * 0.9f;
}

void Time::BeginFixedFrame(void) {
  fixed_time_ += fixed_delta_time_;
  fixed_frame_ = true;
}

void Time::EndFixedFrame(void) {
  fixed_frame_ = false;
}



