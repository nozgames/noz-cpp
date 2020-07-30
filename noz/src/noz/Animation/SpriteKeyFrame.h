///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SpriteKeyFrame_h__
#define __noz_SpriteKeyFrame_h__

#include "KeyFrame.h"

namespace noz {

  class SpriteKeyFrame : public KeyFrame {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Value)
    private: ObjectPtr<Sprite> value_;

    public: SpriteKeyFrame(void);

    public: SpriteKeyFrame(noz_float time, Sprite* sprite);

    public: Sprite* GetValue(void) const {return value_;}

    /// Return the property type the SpriteKeyFrame requires
    public: virtual Type* GetPropertyType(void) const override {return typeof(ObjectPtrProperty);}

    public: virtual Type* GetValueType(void) const override { return typeof(Sprite); }

    public: void SetValue (Sprite* v) {value_ = v;}

    /// Sprites do not support weights.
    public: virtual bool IsWeighted (void) const override {return false;}
  };

}

#endif // __noz_SpriteKeyFrame_h__
