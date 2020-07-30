///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "EnumKeyFrame.h"
#include "EnumBlendTarget.h"

using namespace noz;
  
EnumKeyFrame::EnumKeyFrame(void) {
}

EnumKeyFrame::EnumKeyFrame (noz_float time, const Name& value) {
  SetTime(time);
  value_ = value;
}
