///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "SpriteKeyFrame.h"
#include "SpriteBlendTarget.h"

using namespace noz;
  
SpriteKeyFrame::SpriteKeyFrame(void) {
}

SpriteKeyFrame::SpriteKeyFrame(noz_float time, Sprite* sprite) {
  SetTime(time);
  value_ = sprite;
}


