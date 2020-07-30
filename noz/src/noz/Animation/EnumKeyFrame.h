///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_EnumKeyFrame_h__
#define __noz_EnumKeyFrame_h__

#include "KeyFrame.h"

namespace noz {

  class EnumKeyFrame : public KeyFrame {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Value)
    private: Name value_;

    public: EnumKeyFrame(void);

    public: EnumKeyFrame (noz_float time, const Name& value);

    public: const Name& GetValue(void) const {return value_;}

    public: void SetValue (const Name& value) {value_ = value;}

    /// Return the property type the EnumKeyFrame requires
    public: virtual Type* GetPropertyType(void) const override {return typeof(EnumProperty);}

    /// Sprites do not support weights.
    public: virtual bool IsWeighted (void) const override {return false;}
  };

}

#endif // __noz_EnumKeyFrame_h__
