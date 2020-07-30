///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RepeatButton.h"

using namespace noz;


RepeatButton::RepeatButton (void) {
  click_mode_ = ClickMode::Press;
  interval_ = 0.033f;
  delay_ = 0.5f;  
}

void RepeatButton::OnClick(void) {
  ButtonBase::OnClick();
}

void RepeatButton::OnMouseDown (SystemEvent* e) {
  ButtonBase::OnMouseDown (e);

  if(e->GetButton() != MouseButton::Left) return;

  NOZ_FIXME()
  //EnableEvent(ComponentEvent::Update);
  
  next_click_ = delay_;
}

void RepeatButton::OnMouseUp (SystemEvent* e) {
  ButtonBase::OnMouseUp(e);

  NOZ_FIXME()
  //EnableEvent(ComponentEvent::Update,false);
}

void RepeatButton::Update (void) {  
  ButtonBase::Update();

  if(HasCapture()) {
    for(next_click_ -= Time::GetDeltaTime(); next_click_ < 0; next_click_ += interval_) {
      OnClick();
    }
  }
}
