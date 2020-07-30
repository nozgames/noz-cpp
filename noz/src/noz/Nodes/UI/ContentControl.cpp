///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ContentControl.h"

using namespace noz;

ContentControl::ContentControl (void) {
}

bool ContentControl::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;

  // Text content
  if(text_node_) {
    text_node_->SetText(text_); 
    text_node_->SetVisibility(text_.IsEmpty() ? Visibility::Collapsed : Visibility::Visible);
  }

  // Sprite content
  if(sprite_node_) {
    sprite_node_->SetSprite(sprite_);
    sprite_node_->SetVisibility(sprite_ == nullptr ? Visibility::Collapsed : Visibility::Visible);
  }

  return true;
}

void ContentControl::OnStyleChanged (void) {
  Control::OnStyleChanged();

  text_node_ = nullptr;
  sprite_node_ = nullptr;
}

void ContentControl::SetText(const char* text) {
  if(text_.Equals(text)) return;

  text_=text;
  if(text_node_) {
    text_node_->SetText(text);
    text_node_->SetVisibility(text_.IsEmpty() ? Visibility::Collapsed : Visibility::Visible);
  }
}

void ContentControl::SetSprite(Sprite* sprite) {
  if(sprite == sprite_) return;

  sprite_ = sprite;
  if(sprite_node_) {
    sprite_node_->SetSprite(sprite);
    sprite_node_->SetVisibility(sprite_ == nullptr ? Visibility::Collapsed : Visibility::Visible);
  }
}
