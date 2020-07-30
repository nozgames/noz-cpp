///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ToggleButton.h"

using namespace noz;

ToggleButton::ToggleButton (void) {
  checked_ = false;
}

void ToggleButton::SetChecked (bool checked) {
  if(checked == checked_) return;

  checked_ = checked;

  if(checked_) {
    Checked();
  } else {
    Unchecked();
  }

  UpdateAnimationState();
}

void ToggleButton::UpdateAnimationState(void) {
  ButtonBase::UpdateAnimationState();

  SetAnimationState(checked_ ? UI::StateChecked : UI::StateUnChecked);
}

void ToggleButton::OnClick (void) {
  SetChecked(!checked_);
  ButtonBase::OnClick();
}
