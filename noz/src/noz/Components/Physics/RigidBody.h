///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Components_RigidBody_h__
#define __noz_Components_RigidBody_h__

#include "PhysicsComponent.h"

namespace noz {

  class RigidBody : public PhysicsComponent {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=GravityScale)
    private: noz_float gravity_scale_;

    public: RigidBody(void);

    public: void SetGravityScale(noz_float scale);

    public: virtual void OnAwake(void);

    public: virtual void OnTransformChanged (void);
  };

} // namespace noz


#endif // __noz_Components_RigidBody_h__

