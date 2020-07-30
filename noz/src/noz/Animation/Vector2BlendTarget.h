///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Vector2BlendTarget_h__
#define __noz_Vector2BlendTarget_h__

#include "WeightedBlendTarget.h"

namespace noz {

  class Vector2BlendTarget : public WeightedBlendTarget {
    NOZ_OBJECT(NoAllocator)

    private: Vector2 default_value_;

    private: Vector2 accumulated_value_;

    public: Vector2BlendTarget (Object* target, Property* prop);

	  protected: virtual void Add (noz_uint32 track_id, KeyFrame* from, KeyFrame* to, noz_float lerp, noz_float weight) override;

	  protected: virtual void Add (noz_uint32 track_id, KeyFrame* kf, noz_float weight) override;

	  protected: virtual void AddDefault (noz_uint32 track_id, noz_float weight) override;

    public: virtual void Advance (noz_float elapsed) override;
  };

}

#endif // __noz_Vector2BlendTarget_h__
