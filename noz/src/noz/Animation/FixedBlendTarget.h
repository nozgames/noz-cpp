///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_FixedBlendTarget_h__
#define __noz_FixedBlendTarget_h__

#include "BlendTarget.h"

namespace noz {

  class FixedBlendTarget : public BlendTarget {
    NOZ_OBJECT()

    private: Blend* last_blend_;
    private: noz_int32 last_frame_;

    public: FixedBlendTarget(Object* target, Property* prop);

    public: virtual void Advance (noz_float elapsed) override;

	  protected: virtual void Set (KeyFrame* kf) = 0;

	  protected: virtual void SetDefault (void) = 0;

  };

}

#endif // __noz_FixedBlendTarget_h__
