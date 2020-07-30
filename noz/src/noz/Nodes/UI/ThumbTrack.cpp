///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ThumbTrack.h"
#include "Thumb.h"
#include "RepeatButton.h"

using namespace noz;


ThumbTrack::ThumbTrack (void) {
  orientation_ = Orientation::Vertical;
  minimum_ = 0.0f;
  maximum_ = 100.0f;
  value_ = 0.0f;
  viewport_size_ = 10.0f;
  large_change_ = 1.0f;
  small_change_ = 1.0f;
}

ThumbTrack::~ThumbTrack(void) {
}

void ThumbTrack::SetThumb (Thumb* thumb) {
  if(thumb_ == thumb) return;

  if(thumb_) {
    thumb_->Orphan();
    thumb_->Destroy();
  }

  thumb_ = thumb;
  
  if(thumb_ ) {  
    AddPrivateChild(thumb_);
    thumb_->DragDelta += DragDeltaEventHandler::Delegate(this,&ThumbTrack::OnThumbDragDelta);
    thumb_->DragCompleted += DragCompletedEventHandler::Delegate(this,&ThumbTrack::OnThumbDragCompleted);
  }
}

void ThumbTrack::SetDecreaseButton (RepeatButton* button) {
  if(decrease_button_==button) return;
  if(decrease_button_) {
    decrease_button_->Click -= ClickEventHandler::Delegate(this,&ThumbTrack::OnDecreaseButton);
  }
  decrease_button_ = button;
  if(decrease_button_) {
    decrease_button_->Click += ClickEventHandler::Delegate(this,&ThumbTrack::OnDecreaseButton);
  }
}

void ThumbTrack::SetIncreaseButton (RepeatButton* button) {
  if(increase_button_==button) return;
  if(increase_button_) {
    increase_button_->Click -= ClickEventHandler::Delegate(this,&ThumbTrack::OnIncreaseButton);
  }
  increase_button_ = button;
  if(increase_button_) {
    increase_button_->Click += ClickEventHandler::Delegate(this,&ThumbTrack::OnIncreaseButton);
  }
}

void ThumbTrack::SetOrientation (Orientation orientation) {
  if(orientation_==orientation) return;
  orientation_ = orientation;
  InvalidateTransform();
}

void ThumbTrack::SetSmallChange (noz_float change) {
  small_change_ = change;
}

void ThumbTrack::SetLargeChange (noz_float change) {
  large_change_ = change;
}

void ThumbTrack::SetValue (noz_float value) {
  // Clamp the new value to the minimum and maxium
  value = Math::Clamp(value, minimum_, maximum_);

  // Ensure the value changed.
  if(value == value_) return;

  value_ = value;

  // Fire value changed event if awake
  if(IsAwake()) ValueChanged(this);

  InvalidateTransform();
}

void ThumbTrack::SetMaximum(noz_float value) {
  if(maximum_ == value) return;

  maximum_ = value;
  SetValue(value_);

  InvalidateTransform();
}

void ThumbTrack::SetMinimum(noz_float value) {
  if(minimum_ == value) return;

  minimum_ = value;
  SetValue(value_);

  InvalidateTransform();
}

void ThumbTrack::SetRange(noz_float minimum, noz_float maximum) {
  if(minimum == minimum_ && maximum==maximum_) return;

  minimum_ = minimum;
  maximum_ = maximum;
  SetValue(value_);

  InvalidateTransform();
}  

void ThumbTrack::SetViewportSize (noz_float view_port) {
  if(view_port == viewport_size_) return;

  viewport_size_ = view_port;

  InvalidateTransform();
}  


Vector2 ThumbTrack::MeasureChildren (const Vector2& a) {
  Vector2 size;

  // The U axis is the axis that the thumb moves on
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis which will have a fixed size
  noz_int32 v_axis = !u_axis;

  noz_uint32 thumb_index = thumb_ ? thumb_->GetIndex() : GetChildCount();

  noz_float size_before = 0.0f;
  for(noz_uint32 i=0;i<thumb_index;i++) {
    Vector2 csize = GetChild(i)->Measure(a);
    size[v_axis] = Math::Max(size[v_axis],csize[v_axis]);
    size_before = Math::Max(size_before,csize[u_axis]);
  }  

  noz_float size_after = 0.0f;
  for(noz_uint32 i=thumb_index+1;i<GetChildCount();i++) {
    Vector2 csize = GetChild(i)->Measure(a);
    size[v_axis] = Math::Max(size[v_axis],csize[v_axis]);
    size_after = Math::Max(size_before,csize[u_axis]);
  }  

  size[u_axis] = size_before + size_after;
  if(thumb_) {
    Vector2 msize = thumb_->Measure(a);
    size[v_axis] = Math::Max(size[v_axis],msize[v_axis]);
    size[u_axis] += msize[u_axis];
  }

  return size;
}

void ThumbTrack::ArrangeChildren (const Rect& r) {
  // The U axis is the axis that the trumb moves on
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis which will have a fixed size
  noz_int32 v_axis = !u_axis;

  noz_uint32 thumb_index = thumb_ ? thumb_->GetIndex() : GetChildCount();

  noz_float s[3];
  if(viewport_size_ > 0.0f) {
    noz_float vrange = (maximum_ - minimum_);
    noz_float trange = vrange + viewport_size_;
    noz_float ratio0 = ((value_-minimum_) / vrange) * (vrange / trange);
    noz_float ratio1 = viewport_size_/ trange;

    s[1] = Math::Max(r[v_axis+2], (noz_float)(noz_int32)(r[u_axis+2] * ratio1));
    s[0] = (noz_float)(noz_int32)(r[u_axis+2] * ratio0);
    s[2] = r[u_axis+2] - s[0] - s[1];
  } else {
    s[1] = thumb_ ? thumb_->GetMeasuredSize()[u_axis] : 0.0f;

    noz_float vrange = (maximum_ - minimum_);
    noz_float ratio0 = ((value_-minimum_) / vrange);
    noz_float remain = r[u_axis+2] - s[1];
    s[0] = (noz_float)(noz_int32)(ratio0 * remain);
    s[2] = remain - s[0];
  }

  Rect crect(r.x,r.y,0,0);
  crect[v_axis+2] = r[v_axis+2];

  // Decrease button  
  crect[u_axis+2] = s[0];
  for(noz_uint32 i=0;i<thumb_index; i++) GetChild(i)->Arrange(crect);
  crect[u_axis] += s[0];

  // Thumb button  
  crect[u_axis+2] = s[1];
  if(thumb_) thumb_->Arrange(crect);
  crect[u_axis] += s[1];

  // Increase button  
  crect[u_axis+2] = s[2];
  for(noz_uint32 i=thumb_index+1,c=GetChildCount(); i<c; i++) GetChild(i)->Arrange(crect);
}

void ThumbTrack::OnThumbDragCompleted(UINode*) {
  ValueChanged(this);
}

void ThumbTrack::OnThumbDragDelta (DragDeltaEventArgs* args) {
  noz_float vrange = (maximum_ - minimum_);
  noz_float trange = vrange + viewport_size_;
  noz_float ratio = vrange / trange;

  noz_float vchange = 0.0f;
  if(orientation_==Orientation::Horizontal) {
    vchange = vrange * (args->GetDelta().x / (GetRectangle().w * ratio));
  } else {
    vchange = vrange * (args->GetDelta().y / (GetRectangle().h * ratio));
  }

  if(vchange!=0.0f) SetValue(value_ + vchange);
}


void ThumbTrack::OnIncreaseButton (UINode* sender) {
  SetValue(value_ + viewport_size_);
}

void ThumbTrack::OnDecreaseButton (UINode* sender) {
  SetValue(value_ - viewport_size_);
}

void ThumbTrack::OnChildAdded(Node* child) {
  UINode::OnChildAdded(child);

  if(child->IsTypeOf(typeof(Thumb))) {
    if(nullptr == thumb_ || child->GetIndex() < thumb_->GetIndex()) {
      if(thumb_) {
        thumb_->DragDelta -= DragDeltaEventHandler::Delegate(this,&ThumbTrack::OnThumbDragDelta);
        thumb_->DragCompleted -= DragCompletedEventHandler::Delegate(this,&ThumbTrack::OnThumbDragCompleted);
      }
      thumb_ = (Thumb*)child;
      thumb_->DragDelta += DragDeltaEventHandler::Delegate(this,&ThumbTrack::OnThumbDragDelta);
      thumb_->DragCompleted += DragCompletedEventHandler::Delegate(this,&ThumbTrack::OnThumbDragCompleted);
    }
  }
}

void ThumbTrack::OnChildRemoved (Node* child) {
  UINode::OnChildRemoved(child);
  if(child == thumb_) {
    thumb_->DragDelta -= DragDeltaEventHandler::Delegate(this,&ThumbTrack::OnThumbDragDelta);
    thumb_ = nullptr;
  } else if(decrease_button_==child) {
    decrease_button_->Click -= ClickEventHandler::Delegate(this,&ThumbTrack::OnDecreaseButton);
  } else if(increase_button_==child) {
    increase_button_->Click -= ClickEventHandler::Delegate(this,&ThumbTrack::OnIncreaseButton);
  }
}
