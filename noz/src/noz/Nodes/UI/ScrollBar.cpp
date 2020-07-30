///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ScrollBar.h"
#include "RepeatButton.h"

using namespace noz;

ScrollBar::ScrollBar (void) {
  large_change_ = 1.0f;
  small_change_ = 1.0f;
  maximum_ = 1.0f;
  minimum_ = 0.0f;
  value_ = 0.0f;
  viewport_size_ = 0.0f;
}

ScrollBar::~ScrollBar(void) {
}

bool ScrollBar::OnApplyStyle (void) {  
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == thumb_track_) return false;

  thumb_track_->SetRange(minimum_,maximum_);
  thumb_track_->SetViewportSize(viewport_size_);
  thumb_track_->SetValue(value_);
  thumb_track_->ValueChanged += ValueChangedEventHandler::Delegate(this,&ScrollBar::OnThumbTrackValueChanged);

  // Optional buttons..
  if(line_inc_button_) line_inc_button_->Click += ClickEventHandler::Delegate(this,&ScrollBar::OnLineIncButton);
  if(line_dec_button_) line_dec_button_->Click += ClickEventHandler::Delegate(this,&ScrollBar::OnLineDecButton);

  return true;
}

void ScrollBar::SetSmallChange (noz_float change) {
  small_change_ = change;
}

void ScrollBar::SetLargeChange (noz_float change) {
  large_change_ = change;
}

void ScrollBar::OnLineDecButton(UINode* sender) {
  thumb_track_->SetValue(GetValue() - small_change_);
}

void ScrollBar::OnLineIncButton (UINode* sender) {
  thumb_track_->SetValue(GetValue() + small_change_);
}

void ScrollBar::OnThumbTrackValueChanged (UINode* sender) {
  value_ = thumb_track_->GetValue();
  Scroll(this);
}

void ScrollBar::SetMaximum (noz_float value) {
  if(value==maximum_) return;
  maximum_ = value;
  if(thumb_track_) thumb_track_->SetMaximum(maximum_);
}

void ScrollBar::SetMinimum (noz_float value) {
  if(value==minimum_) return;
  minimum_ = value;
  if(thumb_track_) thumb_track_->SetMinimum(minimum_);
}
  
void ScrollBar::SetValue (noz_float value) {
  if(value==value_) return;
  value_ = value;
  if(thumb_track_) thumb_track_->SetValue(value_);
}
  
void ScrollBar::SetViewportSize (noz_float value) {
  if(value==viewport_size_) return;
  viewport_size_ = value;
  if(thumb_track_) thumb_track_->SetViewportSize(viewport_size_);
}

void ScrollBar::SetRange (noz_float minimum, noz_float maximum) {
  if(minimum_ == minimum && maximum_ == maximum) return;
  minimum_ = minimum;
  maximum_ = maximum;
  if(thumb_track_) thumb_track_->SetRange(minimum_,maximum_);
}