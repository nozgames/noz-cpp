///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Input.h"

using namespace noz;

Input* Input::this_ = nullptr;

void Input::Initialize(void) {
  if (this_ != nullptr) return;

  this_ = new Input;
}

void Input::Uninitialize(void) {
  if (this_ == nullptr) return;

  delete this_;
  this_ = nullptr;
}

Input::Input (void) {
  memset(keys_,0,sizeof(keys_));
  mouse_window_ = nullptr;
  mouse_touch_id_ = 0;
  mouse_button_time_[0] = 0.0f;
  mouse_button_time_[1] = 0.0f;
  mouse_button_time_[2] = 0.0f;
}

void Input::Update (void) {
  for (noz_uint32 i = 0; i<KeyCodeSize; i++) {
    if(this_->keys_[i] & ButtonUp) {
      this_->keys_[i] = 0;
    } else if (this_->keys_[i] & ButtonDown) {
      this_->keys_[i] &= (~ButtonDown);
    }
  }

  for(noz_uint32 a=0;a<InputAxisCount;a++) {
    this_->axis_[a] = 0.0f;
  }

  for(noz_uint32 b=0;b<3;b++) {
    if(this_->mouse_button_[b] & ButtonUp) {
      this_->mouse_button_[b] = 0;
      this_->mouse_drag_delta_[b].clear();
    } else if (this_->mouse_button_[b] & ButtonDown) {
      this_->mouse_button_[b] &= (~ButtonDown);
    }
  }

  // Remove all touches that ended last frame.
  for(noz_uint i=this_->touches_.size(); i>0; i--) {
    if(this_->touches_[i-1].phase_ == TouchPhase::Ended || this_->touches_[i-1].phase_ == TouchPhase::Cancelled) {
      this_->touches_.erase(this_->touches_.begin() + (i-1));
    }
  }
}

void Input::HandleEvent(SystemEvent* e) {
  noz_assert(this_);
  noz_assert(e);

  switch(e->GetEventType()) {
    case SystemEventType::MouseDown: this_->HandleMouseDown(e); break;
    case SystemEventType::MouseMove: this_->HandleMouseMove(e); break;
    case SystemEventType::MouseUp: this_->HandleMouseUp(e); break;
    case SystemEventType::MouseWheel: this_->HandleMouseWheel(e); break;

    case SystemEventType::TouchBegan: this_->HandleTouchBegan(e); break;
    case SystemEventType::TouchMoved: this_->HandleTouchMoved(e); break;
    case SystemEventType::TouchEnded: this_->HandleTouchEnded(e); break;
    case SystemEventType::TouchCancelled: this_->HandleTouchCancelled(e); break;

    case SystemEventType::DragLeave:
    case SystemEventType::Drop: {
      // This is a bit of a hack but on windows after a drag drop operation 
      // has completed the mouse up event is not sent to the window so the 
      // button gets "stuck" up.
      for(noz_uint32 i=0;i<3;i++) {
        this_->mouse_button_[i] = 0;
        this_->mouse_drag_delta_[i].clear();
      }
      break;
    }

    case SystemEventType::KeyDown: {
      noz_byte& state = this_->keys_[e->GetKeyCode()];
      state = ButtonDown | ButtonPressed;
      break;
    }

    case SystemEventType::KeyUp: {
      noz_byte& state = this_->keys_[e->GetKeyCode()];
      state = ButtonUp;
      break;
    }
    
    default: break;
  }
}

void Input::HandleMouseDown (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType()==SystemEventType::MouseDown);

  noz_int32 bidx = (noz_int32)e->GetButton();

  UpdateMousePosition(e);
  mouse_drag_origin_[bidx] = e->GetPosition();
  mouse_drag_delta_[bidx].clear();
  mouse_button_[bidx] |= (ButtonDown | ButtonPressed);
  mouse_button_time_[bidx] = Time::GetTime();
}

void Input::HandleMouseMove (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType()==SystemEventType::MouseMove);
  UpdateMousePosition(e);
}

void Input::HandleMouseUp (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType()==SystemEventType::MouseUp);
  UpdateMousePosition(e);
  mouse_button_[(noz_int32)e->GetButton()] |= ButtonUp;
}

void Input::HandleMouseWheel (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType()==SystemEventType::MouseWheel);
  UpdateMousePosition(e);
  axis_[(noz_int32)InputAxis::MouseWheel] = e->GetDelta().y;  
}

void Input::UpdateMousePosition (SystemEvent* e) {
  // Calculate mouse delta
  if(mouse_window_ == e->GetWindow()) {
    mouse_delta_ = e->GetPosition() - mouse_position_;
  } else {
    mouse_delta_.clear();
  }

  // Update mouse drag
  for(noz_uint32 i=0;i<3;i++) {
    if(mouse_button_[i] & ButtonPressed) {
      mouse_drag_delta_[i] = e->GetPosition() - mouse_drag_origin_[i];
      if(mouse_drag_delta_[i].length_sqr() >= 25.0f) {
        mouse_button_[i] |= ButtonDrag;
      }
    }
  }

  // Save the mouse position and window in Input
  mouse_position_ = e->GetPosition();
  mouse_window_ = e->GetWindow();  
}

void Input::HandleTouchBegan (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType()==SystemEventType::TouchBegan);

  if(touches_.empty()) {
    mouse_touch_id_ = e->GetId();
    SystemEvent::PushEvent(SystemEventType::MouseDown, e->GetWindow(), MouseButton::Left, e->GetClickCount(), e->GetPosition(), e->GetModifiers());
  }

  Touch touch;
  touch.position_ = e->GetPosition();
  touch.id_ = e->GetId();
  touch.tap_count_ = e->GetClickCount();
  touch.phase_ = TouchPhase::Began;
  touch.timestamp_ = Time::GetTime();
  touches_.push_back(touch);
}

void Input::HandleTouchMoved (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType()==SystemEventType::TouchMoved);

  Touch* touch = FindTouch(e->GetId());
  if(nullptr == touch) return;

  touch->delta_position_ = e->GetPosition () - touch->position_;
  touch->position_ = e->GetPosition();
  touch->phase_ = TouchPhase::Moved;
  touch->timestamp_ = Time::GetTime();
  touch->tap_count_ = e->GetClickCount();

  if(e->GetId() == mouse_touch_id_) {
    SystemEvent::PushEvent(SystemEventType::MouseMove, e->GetWindow(), e->GetPosition(), Vector2::Empty, e->GetModifiers());
  }
}

void Input::HandleTouchEnded (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType()==SystemEventType::TouchEnded);

  Touch* touch = FindTouch(e->GetId());
  if(nullptr == touch) return;

  touch->delta_position_ = e->GetPosition () - touch->position_;
  touch->position_ = e->GetPosition();
  touch->phase_ = TouchPhase::Ended;
  touch->tap_count_ = touch->tap_count_;
  touch->timestamp_ = Time::GetTime();
  touch->tap_count_ = e->GetClickCount();

  if(e->GetId() == mouse_touch_id_) {
    mouse_touch_id_ = 0;
    SystemEvent::PushEvent(SystemEventType::MouseUp, e->GetWindow(), MouseButton::Left, 1, e->GetPosition(), e->GetModifiers());
  }
}

void Input::HandleTouchCancelled (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType()==SystemEventType::TouchCancelled);

  Touch* touch = FindTouch(e->GetId());
  if(nullptr == touch) return;

  touch->delta_position_ = e->GetPosition () - touch->position_;
  touch->position_ = e->GetPosition();
  touch->phase_ = TouchPhase::Cancelled;
  touch->tap_count_ = touch->tap_count_;
  touch->timestamp_ = Time::GetTime();
  touch->tap_count_ = e->GetClickCount();

  if(e->GetId() == mouse_touch_id_) {
    mouse_touch_id_ = 0;
    SystemEvent::PushEvent(SystemEventType::MouseUp, e->GetWindow(), MouseButton::Left, 1, e->GetPosition(), e->GetModifiers());
  }
}

Touch* Input::FindTouch (noz_uint64 id) {
  for(noz_uint i=0,c=touches_.size(); i<c; i++) {
    if(touches_[i].id_ == id) return &touches_[i];
  }
  return nullptr;
}
