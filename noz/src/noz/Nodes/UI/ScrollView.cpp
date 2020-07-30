///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ScrollView.h"
#include "ScrollBar.h"

using namespace noz;

ScrollView::ScrollView (void) {
  drag_ = false;

  scrollbar_visibility_[Axis::Horizontal] = ScrollBarVisibility::Auto;
  scrollbar_visibility_[Axis::Vertical] = ScrollBarVisibility::Auto;
  scroll_enabled_[Axis::Horizontal] = true;
  scroll_enabled_[Axis::Vertical] = true;

  movement_type_ = ScrollMovementType::None;

  SetClipChildren(true);
  SetLogicalChildrenOnly();
}

ScrollView::~ScrollView(void) {  
}

void ScrollView::SetHorizontal(bool v) {
  if(scroll_enabled_[Axis::Horizontal] == v) return;
  scroll_enabled_[Axis::Horizontal] = v;
  InvalidateTransform();
}

void ScrollView::SetVertical(bool v) {
  if(scroll_enabled_[Axis::Vertical] == v) return;
  scroll_enabled_[Axis::Vertical] = v;
  InvalidateTransform();
}

void ScrollView::SetHorizontalScrollBar(ScrollBar* sb) {
  if(scrollbar_[Axis::Horizontal]) scrollbar_[Axis::Horizontal]->Scroll -= ScrollEventHandler::Delegate(this,&ScrollView::OnScroll);

  scrollbar_[Axis::Horizontal] = sb;

  if(sb) sb->Scroll += ScrollEventHandler::Delegate(this,&ScrollView::OnScroll);

  InvalidateTransform();
}

void ScrollView::SetVerticalScrollBar(ScrollBar* sb) {
  if(scrollbar_[Axis::Vertical]) scrollbar_[Axis::Vertical]->Scroll -= ScrollEventHandler::Delegate(this,&ScrollView::OnScroll);

  scrollbar_[Axis::Vertical] = sb;

  if(sb) sb->Scroll += ScrollEventHandler::Delegate(this,&ScrollView::OnScroll);

  InvalidateTransform();
}

void ScrollView::SetOffset (const Vector2& offset) {
  if(offset_==offset) return;
  offset_ = offset;
  InvalidateTransform();
}

void ScrollView::SetHorizontalOffset(noz_float v) {
  if(offset_.x == v) return;
  offset_.x = v;
  InvalidateTransform();
}

void ScrollView::SetVerticalOffset(noz_float v) {
  if(offset_.y == v) return;
  offset_.y = v;
  InvalidateTransform();
}

void ScrollView::SetHorizontalScrollBarVisibility (ScrollBarVisibility vis) {
  if(scrollbar_visibility_[Axis::Horizontal] == vis) return;
  scrollbar_visibility_[Axis::Horizontal] = vis;
  InvalidateTransform();
}

void ScrollView::SetVerticalScrollBarVisibility (ScrollBarVisibility vis) {
  if(scrollbar_visibility_[Axis::Vertical] == vis) return;
  scrollbar_visibility_[Axis::Vertical] = vis;
  InvalidateTransform();
}

void ScrollView::OnScroll(UINode*) {
  Vector2 offset = offset_;
  if(scrollbar_[Axis::Horizontal]) offset.x = scrollbar_[Axis::Horizontal]->GetValue();
  if(scrollbar_[Axis::Vertical]) offset.y = scrollbar_[Axis::Vertical]->GetValue();
  if(offset==offset_) return;
  offset_ = offset;
  InvalidateTransform();
}

Vector2 ScrollView::GetViewportSize (void) {
  return GetRectangle().GetSize();;
}

void ScrollView::ScrollToTop (void) {
}

void ScrollView::ScrollToBottom (void) {
}

void ScrollView::ScrollToLeft (void) {
}

void ScrollView::ScrollToRight (void) {
}

void ScrollView::ScrollToHorizontalOffset(noz_float offset) {    
  if(offset_.x == offset) return;

  if(scrollbar_[Axis::Horizontal]) {
    scrollbar_[Axis::Horizontal]->SetValue(offset_.x);
  } else {
    offset_.x = offset;
    InvalidateTransform();
  }
}

Vector2 ScrollView::GetOffsetFromPosition (const Vector2& pos) {
  return pos - GetRectangle().GetMin() +  GetOffset();
}

Vector2 ScrollView::GetExtentSize (void) const {
  return content_ ? content_->GetMeasuredSize() : Vector2::Zero;
}

void ScrollView::ArrangeChildren (const Rect& r) {
  if(content_ == nullptr) return;

  Vector2 csize = content_->GetMeasuredSize();

  Node* n = content_;

  Rect crect;
  for(noz_uint32 i=0; i<2; i++) {
    if(scroll_enabled_[i]) {
      offset_[i] = Math::Clamp(offset_[i],0.0f,csize[i]-r[i+2]);
      crect[i] = r[i]-offset_[i];
      crect[i+2] = Math::Max(csize[i],r[i+2]);
    } else {
      crect[i] = r[i];
      crect[i+2] = r[i+2];
    }
  }

  content_->Arrange(crect);

  Vector2 rsize = r.GetSize();

  // Update the scrollbar value.
  for(noz_uint32 i=0; i<2; i++) {
    if(scrollbar_[i] == nullptr) continue;

    if(rsize[i]>=csize[i]) {
      if(scrollbar_visibility_[i] == ScrollBarVisibility::Auto) {
        scrollbar_[i]->SetVisibility(Visibility::Collapsed);
      } else {
        scrollbar_[i]->SetInteractive(false);
      }
      continue;
    } else if(scrollbar_visibility_[i] == ScrollBarVisibility::Auto && scrollbar_[i]->GetVisibility()==Visibility::Collapsed) {
      scrollbar_[i]->SetVisibility(Visibility::Visible);
    }
    
    scrollbar_[i]->SetInteractive(true);
    scrollbar_[i]->SetRange(0,csize[i]-rsize[i]);
    scrollbar_[i]->SetViewportSize(rsize[i]);
    scrollbar_[i]->SetValue(offset_[i]);
  }
}

void ScrollView::OnChildAdded(Node * child) {
  UINode::OnChildAdded(child);

  // If there is no content node yet create it now.
  if(nullptr==content_) {
    content_ = new Node;
    AddPrivateChild(content_);
  }

  // Add the child to the content node
  content_->AddChild(child);
}

void ScrollView::BringIntoView (Node* node) {
  if(content_==nullptr) return;

  const Rect& rect = GetRectangle();
  
  // Get the rectangle of the given node local to the content rectangle and 
  // adjusted to be zero based which is congruent with how offset is handled.
  Rect lrect = content_->ViewportToLocal(node->LocalToViewport(node->GetRectangle()));
  lrect.x -= content_->GetRectangle().x;
  lrect.y -= content_->GetRectangle().y;

  // Calculate the new new offset..
  Vector2 new_offset = offset_;
  for(noz_uint32 i=0; i<2; i++) {
    // If the current offset is already past the node then roll it back to the node
    if(lrect[i] < offset_[i]) {
      new_offset[i] = lrect[i];
    // If the node's rectangle extends past the scrollview window...
    } else if(lrect[i] + lrect[i+2] > offset_[i] + rect[i+2]) {
      // If the node wont fit entirely in the scrollview..
      if(lrect[i+2]>rect[i+2]) {
        // Scroll to the node minimum
        new_offset[i] = lrect[i];      
      } else {
        // Scroll to align the node maximum with the bottom of the scrollview
        new_offset[i] = lrect[i] + lrect[i+2] - rect[i+2];      
      }
    }
  }

  // If the offset changed..
  if(new_offset != offset_) {
    offset_ = new_offset;
    InvalidateTransform(); 
  }
}

void ScrollView::OnMouseDown (SystemEvent* e) {
  drag_ = false;
  if(movement_type_!=ScrollMovementType::None && e->GetButton() == MouseButton::Left) {
    SetCapture();
    e->SetHandled();
    return;
  }
}

void ScrollView::OnMouseOver (SystemEvent* e) {
  UINode::OnMouseOver(e);

  if(HasCapture()) {
    if(!drag_ && Input::GetMouseButtonDragDelta(MouseButton::Left).length_sqr() >= 25.0f) {
      drag_ = true;
      drag_start_ = offset_;
    }

    if(drag_) {
      SetOffset(drag_start_ - Input::GetMouseButtonDragDelta(MouseButton::Left));
    }
  }
}

void ScrollView::OnMouseUp (SystemEvent* e) {
  drag_ = false;
}

void ScrollView::SetScrollMovementType (ScrollMovementType smt) {
  if(movement_type_ == smt) return;
  movement_type_ = smt;
}
