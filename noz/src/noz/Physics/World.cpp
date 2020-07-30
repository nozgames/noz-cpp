///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "World.h"
#include "Body.h"

#include <external/box2d-2.3.0/Box2D/Box2D.h>


using namespace noz;

World* World::instance_ = nullptr;


void World::Initialize(void) {
  if(instance_!=nullptr) {
    return;
  }
  instance_ = new World;
}

void World::Uninitialize(void) {
  delete instance_;
  instance_ = nullptr;
}

World::World(void) {
  // Create teh box2d world
  world_ = new b2World(b2Vec2(0.0f,-9.8f));
}

World::~World(void) {
  delete world_;
}

void World::Update(void) {
  if(nullptr==instance_) {
    return;
  }

  instance_->Update_();
}

void World::Update_(void) {
  instance_->world_->Step(Time::GetFixedDeltaTime(),8,3);

  // Update the transform of all bodies
  for(b2Body* body=instance_->world_->GetBodyList(); body; body=body->GetNext()) {
    const b2Vec2& p = body->GetPosition();
    ((Body*)body->GetUserData())->GetNode()->SetPosition(Vector2(p.x*10.0f,p.y*10.0f));
  }
}

void World::SetGravity (const Vector2& gravity) {
  if(instance_) {
    instance_->world_->SetGravity(b2Vec2(gravity.x,gravity.y));
  }
}
