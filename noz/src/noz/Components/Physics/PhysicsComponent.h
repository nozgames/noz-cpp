///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Components_PhysicsComponent_h__
#define __noz_Components_PhysicsComponent_h__

#include <noz/Physics/Body.h>

namespace noz {

  class PhysicsComponent : public Component {
    NOZ_OBJECT(Abstract)

    protected: ObjectPtr<Body> body_;

    public: virtual void OnAwake(void);

    public: virtual void OnTransformChanged(void);
  };

} // namespace noz


#endif // __noz_Components_PhysicsComponent_h__

