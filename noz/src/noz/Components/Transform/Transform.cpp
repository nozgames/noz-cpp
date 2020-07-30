///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Transform.h"

using namespace noz;

Transform::Transform(void) {
  local_scale_.set(1.0f, 1.0f);
  local_rotation_ = 0.0f;
  pivot_.set(0.5f,0.5f);
}


void Transform::SetScale(const Vector2& scale) {
  // Prevent double setting scale
  if(local_scale_ == scale) return;

  // Set new scale value
  local_scale_  = scale;

  // Invalidate the scale transform
  InvalidateTransform();
}

void Transform::SetRotation (noz_float rotation) {
  // Ensure rotation changed
  if(local_rotation_ == rotation) return;

  // Set new rotation
  local_rotation_ = rotation;
  
  // Invalidate the transform
  InvalidateTransform();
}

void Transform::SetLocalPosition(const Vector2& pos) {
  if(local_position_ == pos) return;
  local_position_ = pos;
  InvalidateTransform();
}

void Transform::SetPivot(const Vector2& pivot) {
  // Prevent double setting the pivot
  if(pivot_ == pivot) return;

  pivot_ = pivot;

  // Invalidate the layout if there is one
  InvalidateTransform();
}

void Transform::Apply (Matrix3& local_to_world) {
  local_to_world.scale(local_scale_.x, local_scale_.y);
  local_to_world.rotate(Math::Deg2Rad(local_rotation_));
  local_to_world.translate(local_position_.x, local_position_.y);
}

Rect Transform::Update (const Rect& arrange_rect, const Vector2& measured_size) {
  Rect rect;
  rect.x = -measured_size.x * pivot_.x;
  rect.y = -measured_size.y * pivot_.y;
  rect.w = measured_size.x;
  rect.h = measured_size.y;
  return rect;
}
