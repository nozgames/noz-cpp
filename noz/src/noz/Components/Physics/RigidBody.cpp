///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RigidBody.h"

using namespace noz;

RigidBody::RigidBody(void) {
  gravity_scale_ = 1.0f;
}

void RigidBody::OnAwake(void) {
  PhysicsComponent::OnAwake();

  // Set default gravity
  body_->SetGravityScale(gravity_scale_);

  // Set to dynamic
  body_->SetDynamic();
}

void RigidBody::OnTransformChanged (void) {
  if(body_) {
    body_->SetPosition(GetNode()->GetPosition());
  }
}


void RigidBody::SetGravityScale(noz_float scale) {
  if(gravity_scale_==scale) return;
  if(gravity_scale_==0.0f) body_->SetAwake(true);
  
  gravity_scale_ = scale;
  body_->SetGravityScale(gravity_scale_);
}
