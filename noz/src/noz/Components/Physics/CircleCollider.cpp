///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "CircleCollider.h"

using namespace noz;


CircleCollider::CircleCollider(void) {
  radius_ = 0.5f;
}

void CircleCollider::OnAwake(void) {
  PhysicsComponent::OnAwake();

  // Create a default fixture..
  fixture_ = new Fixture(body_,radius_,Vector2());
}

void CircleCollider::SetRadius (noz_float radius) {
  radius_ = radius;
  fixture_ = new Fixture(body_,radius,Vector2());  
}
