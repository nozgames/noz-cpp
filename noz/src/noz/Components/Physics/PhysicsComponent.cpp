///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RigidBody.h"

using namespace noz;


void PhysicsComponent::OnAwake(void) {
  // Check to see if there is already a physics component on the node..
  PhysicsComponent* pc = GetNode()->GetComponent<PhysicsComponent>();
  if(pc && pc!=this) {
    // Share the body with the other physics components.
    body_ = pc->body_;
  } else {
    body_ = new Body(GetNode());
  }
}


void PhysicsComponent::OnTransformChanged(void) {
  if(body_) {
    body_->SetPosition(GetNode()->GetPosition());
  }
}
