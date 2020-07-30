///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Collider.h"

using namespace noz;

void Collider::HandleEvent(SystemEvent* e) {
  noz_assert(e);

  switch(e->GetType()->GetTypeCode()) {
    case TypeCode::MouseButtonEvent:
      if(fixture_ && fixture_->TestPoint(((MouseButtonEvent*)e)->GetPosition())) {
        NOZ_FIXME()
        //GetNode()->OnMouseDown(((MouseButtonEvent*)e));
        e->SetHandled();
      }
      break;
    
    default: break;
  }
}

