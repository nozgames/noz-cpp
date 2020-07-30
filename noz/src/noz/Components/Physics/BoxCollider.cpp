///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "BoxCollider.h"

using namespace noz;


BoxCollider::BoxCollider(void) {
  rect_ = Rect(-1,-1,1,1);
}

void BoxCollider::OnAwake(void) {
  PhysicsComponent::OnAwake();

  fixture_ = new Fixture(body_, rect_);
}

void BoxCollider::SetRectangle(Rect rect) {
  rect_ = rect;
  fixture_ = new Fixture(body_, rect);
}


