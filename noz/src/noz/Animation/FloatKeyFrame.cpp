///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "FloatKeyFrame.h"
#include "FloatBlendTarget.h"

using namespace noz;
  
FloatKeyFrame::FloatKeyFrame(void) {
  value_ = 0;
}

FloatKeyFrame::FloatKeyFrame(noz_float time, noz_float value) {
  value_ = value;
  SetTime(time);
}

