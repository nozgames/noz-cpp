///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Components_CircleCollider_h__
#define __noz_Components_CircleCollider_h__

#include "Collider.h"

namespace noz {

  class CircleCollider : public Collider {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Radius)
    private: noz_float radius_;

    public: CircleCollider(void);

    public: void SetRadius(noz_float radius);

    public: virtual void OnAwake(void) override;
  };

} // namespace noz


#endif // __noz_Components_CircleCollider_h__

