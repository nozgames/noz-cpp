///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Thumb.h"

using namespace noz;


Thumb::Thumb (void) {
}

void Thumb::OnMouseDown (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType() == SystemEventType::MouseDown);

  Control::OnMouseDown(e);

  // We only care about left button
  if(e->GetButton() != MouseButton::Left) return;

  // Handle different button states
  DragStarted(this);

  // Cache the drag start position
  drag_start_ = e->GetPosition();

  // Mark the event as handled so it does not propegate further
  e->SetHandled();

  // Set the capture to this node to ensure all future mouse events
  // are routed to it.
  SetCapture();

  // Update the state
  UpdateAnimationState();  
}

void Thumb::OnMouseUp (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType() == SystemEventType::MouseUp);

  Control::OnMouseUp(e);

  if(HasCapture()) {
    ReleaseCapture();
    UpdateAnimationState();    
    DragCompleted(this);
  }
}
    
void Thumb::OnMouseOver(SystemEvent* e) {
  Control::OnMouseOver(e);

  if(HasCapture()) {
    DragDeltaEventArgs args(this,e->GetPosition()-drag_start_);
    DragDelta(&args);
    drag_start_ = e->GetPosition();
  }
}

void Thumb::UpdateAnimationState(void) {
  if(HasCapture()) {
    SetAnimationState(UI::StatePressed);
    return;
  }

  Control::UpdateAnimationState();
}