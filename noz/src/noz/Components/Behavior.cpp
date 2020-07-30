///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Behavior.h"

using namespace noz;


Behavior::Behavior(void) {
  started_ = false;
}

void Behavior::Update(void) {
  if(!started_) {
    OnStart();
    started_ = true;
  }
  
  OnUpdate();     
}

void Behavior::LateUpdate(void) {
  OnLateUpdate();
}

void Behavior::FixedUpdate(void) {
  OnFixedUpdate();
}

