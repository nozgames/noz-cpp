///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include "ViewBox.h"

using namespace noz;

ViewBox::ViewBox (void) {
  stretch_ = Stretch::Uniform;
  SetLogicalChildrenOnly();
}

void ViewBox::SetStretch (Stretch stretch) {
  if(stretch_ == stretch) return;
  stretch_ = stretch;
  InvalidateTransform();
}

Vector2 ViewBox::MeasureChildren (const Vector2& a) {
  if(nullptr==scale_node_) return Vector2::Empty;

  if(stretch_ == Stretch::Uniform) {    
    Vector2 m1 = scale_node_->Measure(a);
    Rect rr = Math::StretchRect(stretch_, m1, Rect(0,0,a.x,a.y));
    Vector2 m2 = scale_node_->Measure(Math::StretchRect(stretch_, m1, Rect(0,0,a.x,a.y)).GetSize());
    return rr.GetSize();
  }

  return Node::MeasureChildren(a);
}

void ViewBox::ArrangeChildren (const Rect& ar) {
  if(scale_node_==nullptr) return;

  Vector2 s = scale_node_->GetMeasuredSize();
  Rect r = Math::StretchRect(stretch_, s, ar);

  LayoutTransform* t = scale_node_->GetTransform<LayoutTransform>();
  noz_assert(t);
  t->SetScale(Math::Min(r.w / s.x, r.h / s.y));

  scale_node_->Arrange(ar);
}

void ViewBox::OnChildAdded (Node* n) {
  if(nullptr == scale_node_) {
    LayoutTransform* t = new LayoutTransform;
    //t->SetMargin(LayoutLength(LayoutUnitType::Auto, 0.0f));
    scale_node_ = new Node;
    scale_node_->SetTransform(t);
    AddPrivateChild(scale_node_);
  }

  noz_assert(scale_node_ != nullptr);

  scale_node_->AddChild(n);
}
