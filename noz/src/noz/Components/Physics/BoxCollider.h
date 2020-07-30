///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Components_BoxCollider_h__
#define __noz_Components_BoxCollider_h__

#include "Collider.h"

namespace noz {

  class BoxCollider : public Collider {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Rect)
    private: Rect rect_;

    public: BoxCollider(void);

    public: void SetRectangle(Rect rect);

    public: virtual void OnAwake(void) override;
  };

} // namespace noz


#endif // __noz_Components_BoxCollider_h__

