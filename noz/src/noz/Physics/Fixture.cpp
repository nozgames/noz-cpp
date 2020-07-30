///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Fixture.h"
#include "Body.h"
#include "World.h"

#include <external/box2d-2.3.0/Box2D/Box2D.h>

using namespace noz;


Fixture::Fixture(Body* body, Rect rect) {
  b2PolygonShape shape;
  shape.SetAsBox(
    rect.w / 20.0f, 
    rect.h / 20.0f, 
    b2Vec2((rect.x+rect.w*0.5f) / 10.0f, (rect.y+rect.h*0.5f)/ 10.0f), 
    0.0f);

  b2FixtureDef def;
  def.shape = &shape;
  def.density = 0.0f;
  def.friction = 0.0f;
  fixture_ = body->body_->CreateFixture(&def);
}

Fixture::Fixture(Body* body, noz_float radius, const Vector2& center) {
  b2CircleShape shape;
  shape.m_p.Set(center.x/ 10.0f, center.y/ 10.0f);
  shape.m_radius = radius/ 10.0f;

  b2FixtureDef def;
  def.shape = &shape;
  def.density = 0.0f;
  def.friction = 0.0f;
  fixture_ = body->body_->CreateFixture(&def);
}

Fixture::~Fixture (void) {
  noz_assert(fixture_);
  fixture_->GetBody()->DestroyFixture(fixture_);  
}

bool Fixture::TestPoint (const Vector2& pt) {
  noz_assert(fixture_);
  return fixture_->TestPoint(b2Vec2(pt.x/10.0f,pt.y/10.0f));
}
