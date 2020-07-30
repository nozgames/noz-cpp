///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_FloatKeyFrame_h__
#define __noz_FloatKeyFrame_h__

#include "WeightedKeyFrame.h"

namespace noz {

  class BlendTarget;

  class FloatKeyFrame : public WeightedKeyFrame {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Value)
    private: noz_float value_; 

    public: FloatKeyFrame(void);

    public: FloatKeyFrame(noz_float time, noz_float value);

    /// Return the property type the FloatKeyFrame requires
    public: virtual Type* GetPropertyType(void) const override {return typeof(FloatProperty);}

    public: noz_float GetValue(void) const {return value_;}

    public: void SetValue (noz_float v) {value_ = v;}
  };

}

#endif // __noz_FloatKeyFrame_h__
