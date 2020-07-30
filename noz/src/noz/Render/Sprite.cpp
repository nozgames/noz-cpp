///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Sprite.h"

using namespace noz;


Sprite::Sprite(void) {
  t_.set(1.0f,1.0f);
}


Sprite::Sprite(Image* image, const Rect& rect) {
  t_.set(1.0f,1.0f);
  image_ = image;
  rect_ = rect;
  UpdateST();
}

void Sprite::SetRectangle(const Rect& rect) {
  rect_ = rect;
  UpdateST();
}

void Sprite::UpdateST(void) {
  if(image_ == nullptr) {
    return;
  }

  size_ = Vector2(rect_.w, rect_.h);
  
  s_.x = (rect_.x+0.5f) / (noz_float)image_->GetWidth();
  s_.y = (rect_.y+0.5f) / (noz_float)image_->GetHeight();
  t_.x = (rect_.x+rect_.w-0.1f) / (noz_float)image_->GetWidth();
  t_.y = (rect_.y+rect_.h-0.1f) / (noz_float)image_->GetHeight();  
}

void Sprite::SetImage (Image* image) {
  image_ = image;
  if(rect_.IsEmpty()) {
    rect_.x = 0;
    rect_.y = 0;
    rect_.w = (noz_float)image_->GetWidth();
    rect_.h = (noz_float)image_->GetHeight();
  }
  UpdateST();
}
