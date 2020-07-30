//////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include "Viewport.h"

using namespace noz;

Viewport::Viewport(void) {
  attr_ = attr_ | NodeAttributes::Viewport;
  parent_viewport_ = nullptr;
  viewport_ = this;
  scene_ = nullptr;
  virtual_size_ = 0.0f;
  virtual_orientation_ = Orientation::Horizontal;
  virtual_scale_ = 1.0f;
  viewport_to_window_.identity();
  window_to_viewport_.identity();
  camera_container_ = nullptr;
  camera_transform_ = nullptr;
  SetLogicalChildrenOnly();
}

Viewport::~Viewport(void) {
}

void Viewport::OnChildAdded (Node* child) {
  Node::OnChildAdded(child);

  // Create the camera container if this is the first child.
  if(nullptr == camera_container_) {
    camera_container_ = new Node;
    camera_transform_ = new LayoutTransform;
    camera_container_->SetTransform(camera_transform_);
    AddPrivateChild(camera_container_);
  }

  camera_container_->AddChild(child);
}

void Viewport::RenderOverride(RenderContext* rc) {
  rc->PushMatrix();
  rc->SetMatrix(viewport_to_window_);
  Node::RenderOverride(rc);
  rc->PopMatrix();
}

void Viewport::OnLineageChanged (void) {
  // Cache the parent viewport if any
  parent_viewport_ = GetAncestor<Viewport> ();

  // Update the scene pointer as long as this is not the scene root which
  // has its scene pointer set directly on it.
  if(!IsSceneRoot()) {
    if(parent_viewport_) {
      scene_ = parent_viewport_->scene_;
    } else {
      scene_ = nullptr;
    }
  }
}

void Viewport::SetVirtualOrientation (Orientation o) {
  if(virtual_orientation_==o) return;
  virtual_orientation_ = o;
  InvalidateTransform();
}

void Viewport::SetVirtualSize (noz_float s) {
  if(virtual_size_==s) return;
  virtual_size_ = s;
  InvalidateTransform();
}

void Viewport::Arrange(const Rect& ar) {
  Node::Arrange(ar);
}

void Viewport::ArrangeChildren(const Rect& r) {
  if(parent_viewport_) {
    viewport_to_window_ = GetLocalToViewport() *  parent_viewport_->viewport_to_window_;
  } else {
    viewport_to_window_.identity();
  }

  window_to_viewport_ = viewport_to_window_;
  window_to_viewport_.invert();

  if(camera_transform_) {
    if(virtual_size_) {
      noz_float scale = 1.0f;
      if(virtual_orientation_ == Orientation::Horizontal) {
        scale = r.w / (virtual_size_ - 0.625f);
        camera_transform_->SetWidth(virtual_size_);
        camera_transform_->SetHeight(r.h * (virtual_size_ / r.w));
      } else {
        scale = r.h / (virtual_size_ - 0.625f);
        camera_transform_->SetWidth(r.w * (virtual_size_ / r.h));
        camera_transform_->SetHeight(virtual_size_);
      }
      camera_transform_->SetScale(Vector2(scale,scale));
    } else {
      camera_transform_->SetWidth(LayoutLength(LayoutUnitType::Auto,0.0f));
      camera_transform_->SetHeight(LayoutLength(LayoutUnitType::Auto,0.0f));
    }
  }

  Node::ArrangeChildren(r);
}

Vector2 Viewport::MeasureChildren(const Vector2& a) {
  if(virtual_size_==0) return Node::MeasureChildren(a);

  Vector2 vsize;
  if(virtual_orientation_ == Orientation::Horizontal) {
    vsize.x = virtual_size_;
    vsize.y = a.y * (virtual_size_ / a.x);
  } else {
    vsize.y = virtual_size_;
    vsize.x = a.x * (virtual_size_ / a.y);
  }

  return Node::MeasureChildren(vsize);
}

