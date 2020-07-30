///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include "ProgressBar.h"

using namespace noz;


ProgressBar::ProgressBar(void) {
  minimum_ = 0.0f;
  maximum_ = 1.0f;
  value_ = 0.0f;
  step_ = 0.1f;
}

bool ProgressBar::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  UpdateProgressTransform();
  return true;
}

void ProgressBar::SetMinimum (noz_float v) {
  if(minimum_ == v) return;
  minimum_ = v;
  UpdateProgressTransform();
}

void ProgressBar::SetMaximum (noz_float v) {
  if(maximum_== v) return;
  maximum_ = v;
  UpdateProgressTransform();
}

void ProgressBar::SetStep (noz_float v) {
  if(step_==v) return;
  step_ = v;
}

void ProgressBar::SetValue (noz_float v) {
  if(value_==v) return;
  value_ = v;
  UpdateProgressTransform();
}

void ProgressBar::UpdateProgressTransform (void) {
  if(nullptr==progress_transform_) return;
  noz_float v = Math::Clamp(value_, minimum_, maximum_);
  noz_float r = maximum_ - minimum_;
  progress_transform_->SetWidth(LayoutLength(LayoutUnitType::Percentage, v / r * 100.0f));
}

void ProgressBar::PerformStep (void) {
  noz_float v = Math::Min(value_+step_, maximum_);
  SetValue(v);
}
