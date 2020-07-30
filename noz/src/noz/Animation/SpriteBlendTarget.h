///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SpriteBlendTarget_h__
#define __noz_SpriteBlendTarget_h__

#include "FixedBlendTarget.h"

namespace noz {

  class SpriteBlendTarget : public FixedBlendTarget {
    NOZ_OBJECT(NoAllocator)

    private: ObjectPtr<Sprite> default_value_;

    public: SpriteBlendTarget(Object* target, Property* prop);

    public: virtual void Advance (noz_float elapsed) override;

	  protected: virtual void Set (KeyFrame* kf) override;

	  protected: virtual void SetDefault (void) override;

  };

}

#endif // __noz_SpriteBlendTarget_h__
