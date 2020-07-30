///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Components_Collider_h__
#define __noz_Components_Collider_h__

#include "PhysicsComponent.h"
#include <noz/Physics/Fixture.h>

namespace noz {

  class Collider : public PhysicsComponent {    
    NOZ_OBJECT(NoAllocator)

    protected: ObjectPtr<Fixture> fixture_;

    public: virtual void HandleEvent(SystemEvent* e);
  };

} // namespace noz


#endif // __noz_Components_Collider_h__

