///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Spacer.h"

using namespace noz;

Spacer::Spacer(void) {
  width_ = 0;
  height_ = 0;
  count_ = 1;
}

void Spacer::SetCount(noz_int32 count) {
  if(count_ == count) return;
  count_ = count;
  InvalidateTransform();
}

void Spacer::SetWidth(noz_float width) {
  if(width_ == width) return;
  width_ = width;
  InvalidateTransform();
}

void Spacer::SetHeight(noz_float height) {
  if(height_ == height) return;
  height_ = height;
  InvalidateTransform();
}

Vector2 Spacer::MeasureChildren (const Vector2& a) {
  return Math::Max(UINode::MeasureChildren(a),Vector2(width_*count_,height_*count_));
}

