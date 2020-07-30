///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "MeasureScaler.h"

using namespace noz;

MeasureScaler::MeasureScaler(void) {
  scale_ = 1.0f;
}

void MeasureScaler::SetScale (noz_float scale) {
  if(scale_ == scale) return;
  scale_ = scale;
  InvalidateTransform();
}

Vector2 MeasureScaler::MeasureChildren (const Vector2& a) {
  return UINode::MeasureChildren(a) * scale_;
}

