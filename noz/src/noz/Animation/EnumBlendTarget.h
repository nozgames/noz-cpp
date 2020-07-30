///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_EnumBlendTarget_h__
#define __noz_EnumBlendTarget_h__

#include "FixedBlendTarget.h"

namespace noz {

  class EnumBlendTarget : public FixedBlendTarget {
    NOZ_OBJECT(NoAllocator)

    private: Name default_value_;

    public: EnumBlendTarget(Object* target, Property* prop);

    public: virtual void Advance (noz_float elapsed) override;

	  protected: virtual void Set (KeyFrame* kf) override;

	  protected: virtual void SetDefault (void) override;
  };

}

#endif // __noz_EnumBlendTarget_h__
