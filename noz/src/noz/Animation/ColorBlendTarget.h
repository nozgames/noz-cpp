///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ColorBlendTarget_h__
#define __noz_ColorBlendTarget_h__

#include "WeightedBlendTarget.h"

namespace noz {

  class ColorBlendTarget : public WeightedBlendTarget {
    NOZ_OBJECT(NoAllocator)

    private: Vector4 default_value_;

    private: Vector4 accumulated_value_;

    public: ColorBlendTarget (Object* target, Property* prop);

	  protected: virtual void Add (noz_uint32 track_id, KeyFrame* from, KeyFrame* to, noz_float lerp, noz_float weight) override;

	  protected: virtual void Add (noz_uint32 track_id, KeyFrame* kf, noz_float weight) override;

	  protected: virtual void AddDefault (noz_uint32 track_id, noz_float weight) override;

    public: virtual void Advance (noz_float elapsed) override;
  };

}

#endif // __noz_ColorBlendTarget_h__
