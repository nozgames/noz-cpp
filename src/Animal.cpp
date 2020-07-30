///////////////////////////////////////////////////////////////////////////////
// FarmerZ
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <FarmerZ.pch.h>
#include "Animal.h"

using namespace Troglobite;

Animal::Animal(void) {
}

void Animal::Update (void) {
  if(Input::GetTouchCount()>0 && Input::GetTouch(0).GetPhase()==TouchPhase::Began) {
  }
}

void Animal::OnMouseDown (SystemEvent* e) {
  GetNode()->SetAnimationState("Awake");
  e->SetHandled();  
}

void Animal::OnMouseUp (SystemEvent* e) {
  GetNode()->SetAnimationState("Asleep");
  e->SetHandled();
}
