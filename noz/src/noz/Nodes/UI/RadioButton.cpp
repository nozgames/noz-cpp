///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RadioButton.h"

using namespace noz;

RadioButton::RadioButton(void) {
}

void RadioButton::OnClick (void) {
  SetChecked(true);
  ButtonBase::OnClick();
}
