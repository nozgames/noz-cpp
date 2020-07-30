///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_WeightedBlendTarget_h__
#define __noz_WeightedBlendTarget_h__

#include "BlendTarget.h"

namespace noz {

  class WeightedBlendTarget : public BlendTarget {
    NOZ_OBJECT(NoAllocator)

    public: WeightedBlendTarget(Object* target, Property* prop);

	  protected: virtual void Add (noz_uint32 track_id, KeyFrame* from, KeyFrame* to, noz_float lerp, noz_float weight) = 0;

	  protected: virtual void Add (noz_uint32 track_id, KeyFrame* kf, noz_float weight) = 0;

	  protected: virtual void AddDefault (noz_uint32 track_id, noz_float weight) = 0;

    public: virtual void Advance (noz_float elapsed) override;
    
    protected: virtual bool BlendedStop (Blend* blend, noz_float blend_time) override;
  };

}

#endif // __noz_WeightedBlendTarget_h__
