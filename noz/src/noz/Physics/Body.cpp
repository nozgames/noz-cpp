///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Body.h"
#include "World.h"

#include <external/box2d-2.3.0/Box2D/Box2D.h>

using namespace noz;

Body::Body(Node* node) {
  node_ = node;

  const Vector2& position = node->GetPosition();

  b2BodyDef def;
  def.position = b2Vec2(position.x/10.0f, position.y/10.0f);
  def.type = b2_staticBody;
  def.userData = this;
  body_ = World::instance_->world_->CreateBody(&def);

  b2MassData md;
  md.mass = 1.0f;
  md.I = 0.0f;
  md.center = b2Vec2(0,0);

  body_->SetMassData(&md);  
}

Body::~Body(void) {
  World::instance_->world_->DestroyBody(body_);
}

Vector2 Body::GetPosition(void) const {
  const b2Vec2& pos = body_->GetPosition();
  return Vector2(pos.x*10.0f, pos.y*10.0f);
}

Vector2 Body::GetLinearVelocity(void) const {
  const b2Vec2& v = body_->GetLinearVelocity();
  return Vector2(v.x, v.y);
}

void Body::SetPosition(const Vector2& pos) {
  body_->SetTransform(b2Vec2(pos.x/10.0f,pos.y/10.0f),body_->GetAngle());
}

void Body::SetAngle(noz_float angle) {
  body_->SetTransform(body_->GetPosition(), angle);
}

void Body::SetGravityScale(noz_float scale) {
  body_->SetGravityScale(scale);
}

void Body::SetStatic(void) {
  body_->SetType(b2_staticBody);
}

void Body::SetDynamic(void) {
  body_->SetType(b2_dynamicBody);
}

void Body::SetAwake(bool awake) {
  body_->SetAwake(awake);
} 