///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Splitter.h"
#include "Thumb.h"

using namespace noz;


Splitter::Splitter (void) {
  split_ = 0;
  split_align_ = Alignment::Center;
  drag_split_ = 0.0f;
  orientation_ = Orientation::Vertical;
  thumb_size_ = Float::NaN;
}

void Splitter::SetSplit (noz_float split) {
  if(split == split_) return;
  split_=split;
  InvalidateTransform();
}

void Splitter::SetSplitAlignment (Alignment align) {
  if(align==split_align_) return;
  split_align_ = align;
  InvalidateTransform();
}  

void Splitter::SetOrientation (Orientation orientation) {
  if(orientation_==orientation) return;
  orientation_ = orientation;

  if(thumb_) thumb_->SetCursor(orientation_==Orientation::Vertical?Cursors::SizeNS:Cursors::SizeWE);

  InvalidateTransform();
}

void Splitter::SetThumbSize(noz_float thumb_size) {
  if(thumb_size_==thumb_size) return;
  thumb_size_ = thumb_size;
  InvalidateTransform();
}

void Splitter::OnThumbDragStarted (UINode*) {
  drag_split_ = split_;
}

void Splitter::OnChildAdded(Node * child) {
  UINode::OnChildAdded(child);

  if(child->IsTypeOf(typeof(Thumb))) {
    if(nullptr == thumb_ || child->GetIndex() < thumb_->GetIndex()) {
      if(thumb_) {
        thumb_->DragStarted -= DragStartedEventHandler::Delegate(this, &Splitter::OnThumbDragStarted);
        thumb_->DragDelta -= DragDeltaEventHandler::Delegate(this, &Splitter::OnThumbDragDelta);      
      }
      thumb_ = (Thumb*)child;
      thumb_->SetCursor(orientation_==Orientation::Vertical?Cursors::SizeNS:Cursors::SizeWE);
      thumb_->DragStarted += DragStartedEventHandler::Delegate(this, &Splitter::OnThumbDragStarted);
      thumb_->DragDelta += DragDeltaEventHandler::Delegate(this, &Splitter::OnThumbDragDelta);      
    }
  }
}

void Splitter::OnChildRemoved (Node* child) {
  UINode::OnChildRemoved(child);
  if(child == thumb_) {
    thumb_->DragStarted -= DragStartedEventHandler::Delegate(this, &Splitter::OnThumbDragStarted);
    thumb_->DragDelta -= DragDeltaEventHandler::Delegate(this, &Splitter::OnThumbDragDelta);      
  }
}

void Splitter::OnThumbDragDelta (DragDeltaEventArgs* args) {
  // The U axis is the axis on which content flows normally before wrapping
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // Increase drag delta.
  if(split_align_==Alignment::Max) {
    drag_split_ -= args->GetDelta()[u_axis];
  } else {
    drag_split_ += args->GetDelta()[u_axis];
  }

  // Set new split
  SetSplit(drag_split_);
}

Vector2 Splitter::MeasureChildren (const Vector2& available_size) {
  Vector2 size;

  // The U axis is the axis on which content flows normally before wrapping
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis on which the panel grows when a wrap occurrs
  noz_int32 v_axis = !u_axis;

  noz_float u_available = available_size[u_axis];
  noz_float panel1_available = 0.0f;
  noz_float panel2_available = 0.0f;

  Vector2 msize = thumb_ ? thumb_->Measure(available_size) : Vector2::Zero;
  if(thumb_size_==Float::NaN) {
    size[u_axis] += msize[u_axis];
    size[v_axis] = Math::Max(size[v_axis],msize[v_axis]);
    u_available -= msize[u_axis];
  } else {
    size[u_axis] += thumb_size_;
    u_available -= thumb_size_;
  }
  
  switch(split_align_) {
    case Alignment::Center:
      panel1_available = (u_available * 0.5f) + split_;
      panel2_available = u_available - panel1_available;
      break;

    case Alignment::Min:
      panel1_available = split_;
      panel2_available = u_available - panel1_available;
      break;

    case Alignment::Max:
      panel2_available = split_;
      panel1_available = u_available - panel2_available;
      break;
  }

  noz_uint32 thumb_index = thumb_ ? thumb_->GetIndex() : GetChildCount();

  // Content1
  Vector2 a;
  a[u_axis] = panel1_available;
  a[v_axis] = available_size[v_axis];
  for(noz_uint32 i=0; i<thumb_index; i++) {
    msize = GetChild(i)->Measure(a);
    size[u_axis] += msize[u_axis];
    size[v_axis] = Math::Max(size[v_axis],msize[v_axis]);
  }

  // Content2
  a[u_axis] = panel2_available;
  a[v_axis] = available_size[v_axis];
  for(noz_uint32 i=thumb_index+1,c=GetChildCount(); i<c; i++) {
    msize = GetChild(i)->Measure(a);
    size[u_axis] += msize[u_axis];
    size[v_axis] = Math::Max(size[v_axis],msize[v_axis]);
  }

  return size;
}
   
void Splitter::ArrangeChildren (const Rect& nrect) {
  // The U axis is the axis on which content flows normally before wrapping
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis on which the panel grows when a wrap occurrs
  noz_int32 v_axis = !u_axis;

  // Calculate thumb size
  noz_float thumb_size = thumb_size_;
  if(thumb_size==Float::NaN) {
    thumb_size = thumb_ ? thumb_->GetMeasuredSize()[u_axis] : 0.0f;
  }

  noz_float panel1_size;
  noz_float panel2_size;
  noz_float non_thumb_size = nrect[u_axis+2] - thumb_size;

  switch(split_align_) {
    case Alignment::Min: 
      split_ = Math::Clamp(split_,0.0f,non_thumb_size);
      panel1_size = split_;
      panel2_size = non_thumb_size - panel1_size;
      break;

    case Alignment::Max:
      split_ = Math::Clamp(split_,0.0f,non_thumb_size);
      panel2_size = split_;
      panel1_size = non_thumb_size - panel2_size;
      break;

    case Alignment::Center: {
      noz_float non_thumb_center = non_thumb_size*0.5f;
      split_ = Math::Clamp(split_,-non_thumb_center,non_thumb_center);
      panel1_size = non_thumb_center + split_;
      panel2_size = non_thumb_size - panel1_size;      
      break;
    }
  }

  noz_uint32 thumb_index = thumb_ ? thumb_->GetIndex() : GetChildCount();

  // Arrange Content1
  Rect r = nrect;
  r[u_axis+2] = panel1_size;
  for(noz_uint32 i=0; i<thumb_index; i++) GetChild(i)->Arrange(r);

  // Arrange thumb
  r[u_axis] += r[u_axis+2];
  r[u_axis+2] = thumb_size;
  if(thumb_) thumb_->Arrange(r);
  
  // Arrange panel2
  r[u_axis] += r[u_axis+2];
  r[u_axis+2] = panel2_size;
  for(noz_uint32 i=thumb_index+1,c=GetChildCount(); i<c; i++) GetChild(i)->Arrange(r);
}