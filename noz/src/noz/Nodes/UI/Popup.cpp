///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Popup.h"

using namespace noz;

Popup::Popup(void) {
  placement_ = PopupPlacement::Bottom;
  SetLogicalChildrenOnly();
}

Popup::~Popup(void) {
  if(window_) delete window_;

  if(content_) content_->Destroy();
}

void Popup::Open(void) {
  if(nullptr==content_) return;

  Node* target = placement_target_;
  if(nullptr==target) target = GetParent();  

  // Create a window for the popup
  Window* w = window_;
  if(nullptr == w) {
    w = new Window(WindowAttributes::Popup, target->GetWindow());
    w->popup_ = this;
    Scene* scene = new Scene;
    scene->GetRootNode()->AddChild(content_);
    w->GetRootNode()->AddChild(scene->GetRootNode());
    window_ = w;
  }

  Vector2 ss = target->GetWindow()->GetScreenSize();

  Vector2 ms = content_->Measure(ss);
  Rect r;
  Vector2 v;

  PopupPlacement placement = placement_;

  while(true) {
    switch(placement) {
      case PopupPlacement::Relative: {
        v = target->LocalToScreen(target->GetRectangle().GetTopLeft()) + placement_offset_;
        if(v.y + ms.y > ss.y) v.y = ss.y - ms.y;
        if(v.x+ms.x>ss.x) v.x = ss.x - ms.x;
        if(v.x<0) v.x = 0;
        break;
      }

      case PopupPlacement::Bottom: {
        v = target->LocalToScreen(target->GetRectangle().GetBottomLeft()) + placement_offset_;
        if(v.y + ms.y > ss.y) {
          v = target->LocalToScreen(target->GetRectangle().GetTopLeft());
          v.x += placement_offset_.x;
          v.y -= placement_offset_.y;
          v.y -= ms.y;
          if(v.y - ms.y < 0) v.y = 0;
        }
        if(v.x+ms.x>ss.x) v.x = ss.x - ms.x;
        if(v.x<0) v.x = 0;
        break;
      }
      
      default: noz_assert(false);
    }
    break;
  }

  w->MoveTo(Rect(v.x,v.y,ms.x,ms.y),nullptr);
  w->Show();
  w->SetFocus();
}


void Popup::Close (void) {
  if(nullptr == window_) return;
  if(content_) content_->Orphan();
  Window* w = window_;
  w->popup_ = nullptr;
  window_= nullptr;
  delete w;
}

void Popup::OnChildAdded (Node* child) {
  UINode::OnChildAdded(child);

  if(nullptr==content_) {
    content_ = new Node;
  }

  content_->AddChild(child);
}

void Popup::SetPlacement (PopupPlacement placement) {
  placement_ = placement;
}

void Popup::SetPlacementTarget (Node* target) {
  placement_target_ = target;
}

void Popup::SetPlacementOffset (const Vector2& offset) {
  placement_offset_ = offset;
}

void Popup::Arrange (const Rect& r) {
  ArrangeHide();
}
