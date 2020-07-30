//////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ImageNode.h"

using namespace noz;

ImageNode::ImageNode(void) : mesh_(4,6), tiling_(1.0f,1.0f) {
  stretch_ = Stretch::None;
}

void ImageNode::SetImage(Image* image) {
  if(image_ == image) return;
  image_ = image;
  Invalidate();
}

void ImageNode::SetColor(Color color) {
  if(color_==color) return;
  color_ = color;
  mesh_.SetColor(color_);  
}
    
void ImageNode::UpdateMesh (const Rect& rr) {
  mesh_.Clear();

  if(image_ == nullptr) return;

  Rect r = Math::StretchRect(stretch_, image_->GetSize(), rr);

  mesh_.SetImage(image_);

  mesh_.AddVertex(Vector2(r.x,r.y), Vector2::Zero, color_);
  mesh_.AddVertex(Vector2(r.x+r.w,r.y), Vector2(tiling_.x,0.0f), color_);
  mesh_.AddVertex(Vector2(r.x+r.w,r.y+r.h), tiling_, color_);
  mesh_.AddVertex(Vector2(r.x,r.y+r.h), Vector2(0.0f,tiling_.y), color_);

  mesh_.AddTriangle(0,1,2);
  mesh_.AddTriangle(0,2,3);
}

Vector2 ImageNode::MeasureMesh (const Vector2& avail) {
  if(nullptr == image_) return Vector2::Zero;

  switch(stretch_) {
    case Stretch::Uniform:
    case Stretch::UniformToFill:
      return Math::StretchRect(stretch_,image_->GetSize(), Rect(0,0,avail.x,avail.y)).GetSize();

    case Stretch::None:
    case Stretch::Fill:
      return image_->GetSize();

    default: break;
  }

  return Vector2::Zero;
}

bool ImageNode::DrawMesh (RenderContext* rc) {
  return rc->Draw(&mesh_);
}

void ImageNode::SetStretch (Stretch stretch) {
  if(stretch_ == stretch) return;
  stretch_ = stretch;
  Invalidate();
}

void ImageNode::SetStretchAlignment(Alignment a) {
}

void ImageNode::SetTiling (const Vector2& tiling) {
  if(tiling_ == tiling) return;
  tiling_ = tiling;
  Invalidate();
}
