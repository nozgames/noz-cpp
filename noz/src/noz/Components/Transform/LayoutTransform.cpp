///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Text/StringLexer.h>
#include <noz/IO/StringReader.h>
#include "LayoutTransform.h"

using namespace noz;

LayoutLength LayoutLength::Empty;

LayoutTransform::LayoutTransform(void) :
  maintain_aspect_ratio_(false),
  aspect_ratio_(1.0f,1.0f),
  margin_left_(0.0f), 
  margin_top_(0.0f), 
  margin_right_(0.0f), 
  margin_bottom_(0.0f),
  padding_left_(0.0f), 
  padding_top_(0.0f), 
  padding_right_(0.0f), 
  padding_bottom_(0.0f),
  min_width_(0.0f),
  min_height_(0.0f),
  max_height_(Float::Infinity),
  max_width_(Float::Infinity),
  width_(LayoutUnitType::Auto, 0.0f),
  height_(LayoutUnitType::Auto, 0.0f) {
}

Rect LayoutTransform::AdjustArrangeRect(const Rect& r) const {
  return Rect (
    r.x + padding_left_,
    r.y + padding_top_,
    r.w - (padding_left_ + padding_right_),
    r.h - (padding_top_ + padding_bottom_)
  );
}

noz_float CalcLength (const LayoutLength& ll, noz_float def, noz_float max) {
  if(ll.IsFixed()) return ll.value_;
  if(ll.IsPercentage()) return max * (ll.value_ / 100.0f);
  return def;
}

Rect LayoutTransform::Update (const Rect& arrange_rect, const Vector2& measured_size) {
  // Start with the current local position.
  Vector2 pos;
  Rect rect;

  Vector2 measured_size_no_margins = measured_size;
  if(margin_left_.IsFixed()) measured_size_no_margins.x -= margin_left_;
  if(margin_right_.IsFixed()) measured_size_no_margins.x -= margin_right_;
  if(margin_top_.IsFixed()) measured_size_no_margins.y -= margin_top_;
  if(margin_bottom_.IsFixed()) measured_size_no_margins.y -= margin_bottom_;

  Vector2 margin_min;
  Vector2 margin_max;
  Vector2 actual_size;
  noz_float margin_auto_weight;
  noz_float margin_auto;

  /*
  margin_min.x = CalcLength(margin_left_, 0.0f, arrange_rect.w);
  margin_max.x = CalcLength(margin_right_, 0.0f, arrange_rect.w);
  margin_min.y = CalcLength(margin_top_, 0.0f, arrange_rect.h);
  margin_max.y = CalcLength(margin_bottom_, 0.0f, arrange_rect.h);
  */

  // X-Axis
  margin_min.x = CalcLength(margin_left_, 0.0f, arrange_rect.w);
  margin_max.x = CalcLength(margin_right_, 0.0f, arrange_rect.w);
  actual_size.x = Math::Max(CalcLength(width_, measured_size_no_margins.x, arrange_rect.w),min_width_);;
  margin_auto_weight = noz_float(margin_left_.IsAuto()) + noz_float(margin_right_.IsAuto());
  margin_auto = arrange_rect.w - actual_size.x - margin_min.x - margin_max.x;
  if(margin_left_.IsAuto()) margin_min.x = margin_auto / margin_auto_weight;
  if(margin_right_.IsAuto()) margin_max = margin_auto / margin_auto_weight;
  if(width_.IsAuto()) actual_size.x = arrange_rect.w-margin_max.x-margin_min.x;

  // Y-Axis
  margin_min.y = CalcLength(margin_top_, 0.0f, arrange_rect.h);
  margin_max.y = CalcLength(margin_bottom_, 0.0f, arrange_rect.h);
  actual_size.y = Math::Max(CalcLength(height_, measured_size_no_margins.y, arrange_rect.h),min_height_);
  margin_auto_weight = noz_float(margin_top_.IsAuto()) + noz_float(margin_bottom_.IsAuto());
  margin_auto = arrange_rect.h - actual_size.y - margin_min.y - margin_max.y;
  if(margin_top_.IsAuto()) margin_min.y = margin_auto / margin_auto_weight;
  if(margin_bottom_.IsAuto()) margin_max.y = margin_auto / margin_auto_weight;
  if(height_.IsAuto()) actual_size.y = arrange_rect.h-margin_max.y-margin_min.y;

  if(maintain_aspect_ratio_) {
    noz_float s_aspect_ratio = actual_size.x / actual_size.y;
    noz_float t_aspect_ratio = aspect_ratio_.x / aspect_ratio_.y;

    if(s_aspect_ratio < t_aspect_ratio) {
      actual_size.y = actual_size.x * (1.0f / t_aspect_ratio);
      margin_min.y = CalcLength(margin_top_, 0.0f, arrange_rect.h);
      margin_max.y = CalcLength(margin_bottom_, 0.0f, arrange_rect.h);
      margin_auto_weight = noz_float(margin_top_.IsAuto()) + noz_float(margin_bottom_.IsAuto());
      margin_auto = arrange_rect.h - actual_size.y - margin_min.y - margin_max.y;
      if(margin_top_.IsAuto()) margin_min.y = margin_auto / margin_auto_weight;
      if(margin_bottom_.IsAuto()) margin_max.y = margin_auto / margin_auto_weight;
    } else if (t_aspect_ratio < s_aspect_ratio) {
      margin_min.x = CalcLength(margin_left_, 0.0f, arrange_rect.w);
      margin_max.x = CalcLength(margin_right_, 0.0f, arrange_rect.w);
      actual_size.x = actual_size.y * t_aspect_ratio;
      margin_auto_weight = noz_float(margin_left_.IsAuto()) + noz_float(margin_right_.IsAuto());
      margin_auto = arrange_rect.w - actual_size.x - margin_min.x - margin_max.x;
      if(margin_left_.IsAuto()) margin_min.x = margin_auto / margin_auto_weight;
      if(margin_right_.IsAuto()) margin_max = margin_auto / margin_auto_weight;
    }
  }

  rect.w = Math::Min(actual_size.x,max_width_);
  rect.x = -rect.w * pivot_.x;
  pos.x = (arrange_rect.x * 2.0f + arrange_rect.w + margin_min.x - margin_max.x) * 0.5f + actual_size.x * 0.5f - (actual_size.x * (1.0f - pivot_.x));

  rect.h = Math::Min(actual_size.y,max_height_);
  rect.y = -rect.h * pivot_.y;
  pos.y = (arrange_rect.y * 2.0f + arrange_rect.h + margin_min.y - margin_max.y) * 0.5f + actual_size.y * 0.5f - (actual_size.y * (1.0f - pivot_.y));

  local_position_ = pos;

  return rect;
}

Vector2 LayoutTransform::AdjustAvailableSize (const Vector2& available_size) {
  Vector2 reduced_available_size = Transform::AdjustAvailableSize(available_size);  

  // Left margin
  if(margin_left_.IsFixed()) reduced_available_size.x -= margin_left_;
  if(margin_right_.IsFixed()) reduced_available_size.x -= margin_right_;
  if(margin_top_.IsFixed()) reduced_available_size.y -= margin_top_;
  if(margin_bottom_.IsFixed()) reduced_available_size.y -= margin_bottom_;

  if(width_.IsFixed()) reduced_available_size.x = width_.value_;
  if(height_.IsFixed()) reduced_available_size.y = height_.value_;

  reduced_available_size.x -= padding_left_;
  reduced_available_size.x -= padding_right_;
  reduced_available_size.y -= padding_top_;
  reduced_available_size.y -= padding_bottom_;  

  return reduced_available_size;
}

Vector2 LayoutTransform::AdjustMeasuredSize (const Vector2& measured_size) {
  Vector2 size = measured_size;

  // Override any calculated measured size values with fixed values.
  if(!width_.IsAuto()) size.x = width_;
  if(!height_.IsAuto()) size.y = height_;

  // Minimum/Maximum size.
  size.x = Math::Max(size.x,min_width_);
  size.x = Math::Min(size.x,max_width_);
  size.y = Math::Max(size.y,min_height_);
  size.y = Math::Min(size.y,max_height_);

  // Add padding size
  if(width_.IsAuto()) {
    size.x += padding_left_;
    size.x += padding_right_;
  }

  if(height_.IsAuto()) {
    size.y += padding_top_;
    size.y += padding_bottom_;
  }

  // Add in any fixed margin values
  if(margin_left_.IsFixed()) size.x += margin_left_;
  if(margin_right_.IsFixed()) size.x += margin_right_;
  if(margin_top_.IsFixed()) size.y += margin_top_;
  if(margin_bottom_.IsFixed()) size.y += margin_bottom_;

  return size;
}

void LayoutTransform::SetWidth(const LayoutLength& width) {
  if(width_ == width) return;
  width_ = width;
  InvalidateTransform();
} 

void LayoutTransform::SetMinWidth(noz_float width) {
  if(min_width_ == width) return;
  min_width_ = width;
  InvalidateTransform();
}

void LayoutTransform::SetMaxWidth(noz_float width) {
  if(max_width_ == width) return;
  max_width_ = width;
  InvalidateTransform();
}

void LayoutTransform::SetHeight(const LayoutLength& height) {
  if(height_ == height) return;
  height_ = height;
  InvalidateTransform();
}

void LayoutTransform::SetMinHeight(noz_float height) {
  if(min_height_ == height) return;
  min_height_ = height;
  InvalidateTransform();
}

void LayoutTransform::SetMaxHeight(noz_float height) {
  if(max_height_ == height) return;
  max_height_ = height;
  InvalidateTransform();
}

void LayoutTransform::SetMarginLeft(const LayoutLength& margin) {
  if(margin_left_==margin) return;
  margin_left_= margin;
  InvalidateTransform();
}

void LayoutTransform::SetMarginRight(const LayoutLength& margin) {
  if(margin_right_==margin) return;
  margin_right_ = margin;
  InvalidateTransform();
}

void LayoutTransform::SetMarginTop (const LayoutLength& margin) {
  if(margin_top_==margin) return;
  margin_top_= margin;
  InvalidateTransform();
}

void LayoutTransform::SetMarginBottom(const LayoutLength& margin) {
  if(margin_bottom_==margin) return;
  margin_bottom_ = margin;
  InvalidateTransform();
}

void LayoutTransform::SetPadding (noz_float padding) {
  padding_left_ = padding_top_ = padding_right_ = padding_bottom_ = padding;
  InvalidateTransform();
}

void LayoutTransform::SetPaddingLeft(noz_float margin) {
  if(padding_left_==margin) return;
  padding_left_= margin;
  InvalidateTransform();
}

void LayoutTransform::SetPaddingRight(noz_float margin) {
  if(padding_right_==margin) return;
  padding_right_ = margin;
  InvalidateTransform();
}

void LayoutTransform::SetPaddingTop (noz_float margin) {
  if(padding_top_==margin) return;
  padding_top_= margin;
  InvalidateTransform();
}

void LayoutTransform::SetPaddingBottom(noz_float margin) {
  if(padding_bottom_ ==margin) return;
  padding_bottom_ = margin;
  InvalidateTransform();
}

void LayoutTransform::SetMargin (const LayoutLength& margin) {
  margin_left_ = margin_top_ = margin_right_ = margin_bottom_ = margin;
  InvalidateTransform();
}

bool LayoutTransform::IsDependentOnMeasure (void) const {
  // Not dependent on measure if both width and height are fixed...
  if(!width_.IsAuto() && !height_.IsAuto()) return false;

  // Not dependent on measure if all margins are fixed..
  if(!margin_left_.IsAuto() && 
     !margin_right_.IsAuto() && 
     !margin_top_.IsAuto() && 
     !margin_bottom_.IsAuto()) return false;

  // Transform is dependent on the measure size.
  return true;
}

void LayoutTransform::SetMaintainAspectRatio (bool v) {
  if(maintain_aspect_ratio_==v) return;
  maintain_aspect_ratio_ = v;
  InvalidateTransform();
}

void LayoutTransform::SetAspectRatio (const Vector2& v) {
  if(aspect_ratio_==v) return;
  aspect_ratio_ = v;
  InvalidateTransform();
}

