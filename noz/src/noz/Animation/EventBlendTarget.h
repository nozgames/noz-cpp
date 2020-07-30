///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_EventBlendTarget_h__
#define __noz_EventBlendTarget_h__

#include "BlendTarget.h"

namespace noz {

  class EventBlendTarget : public BlendTarget {
    NOZ_OBJECT()

    private: noz_int32 last_frame_;

    public: EventBlendTarget(Object* target, Property* prop);

    public: virtual void Advance (noz_float elapsed) override;

    private: void FireEvents (Blend* b, noz_int32 to_frame);
  };

}

#endif // __noz_EventBlendTarget_h__
