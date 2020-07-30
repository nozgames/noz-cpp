///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Render/SpriteNode.h>
#include "ButtonBase.h"

using namespace noz;

ButtonBase::ButtonBase (void) {
  click_mode_ = ClickMode::Release;
  SetFocusable();
  SetLogicalChildrenOnly();
}

void ButtonBase::OnMouseDown(SystemEvent* e) {    
  noz_assert(e);
  noz_assert(e->GetEventType() == SystemEventType::MouseDown);

  // Handle non interactive state.
  if(!IsInteractive()) return;

  // We only care about left ButtonBase presses
  if(e->GetButton() != MouseButton::Left) return;

  // Mark the event as handled so it does not propegate further
  e->SetHandled();

  // Set the button as focused
  if(IsFocusable()) SetFocus();

  // Set the capture to this node to ensure all future events are routed to it.
  SetCapture();

  // Update the state
  UpdateAnimationState();

  if(click_mode_ == ClickMode::Press) {
    OnClick();
  }
}

void ButtonBase::OnMouseUp(SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType() == SystemEventType::MouseUp);

  // Update state
  if(!HasCapture()) return;

  // If the position is within the ButtonBase...
  if(click_mode_==ClickMode::Release && IsMouseOver()) {
    OnClick();
  }

  // Release capture
  ReleaseCapture();

  UpdateAnimationState();
}

void ButtonBase::OnStyleChanged(void) {
  ContentControl::OnStyleChanged();

  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) GetLogicalChild(i)->Orphan(false);

  content_container_ = nullptr;
}

void ButtonBase::UpdateAnimationState(void) {
  if(HasCapture()) {
    if(IsMouseOver()) {
      SetAnimationState(UI::StatePressed);
    } else {
      SetAnimationState(UI::StateMouseOver);
    }
    return;
  }

  Control::UpdateAnimationState();
}


bool ButtonBase::OnApplyStyle(void) {
  if(!ContentControl::OnApplyStyle()) return false;

  if(content_container_) {
    for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) content_container_->AddChild(GetLogicalChild(i));
  }

  return true;
}

void ButtonBase::OnChildAdded(Node * child) {
  ContentControl::OnChildAdded(child);

  if(content_container_) content_container_->AddChild(child);
}

void ButtonBase::OnClick (void) {
  Click(this);
}
