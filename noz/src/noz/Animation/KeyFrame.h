///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_KeyFrame_h__
#define __noz_KeyFrame_h__

namespace noz {

  class AnimationTrack;
  class BlendTarget;

  class KeyFrame : public Object {
    NOZ_OBJECT()

    friend class AnimationTrack;

    /// Time of the frame in the timeline
    private: NOZ_PROPERTY(Name=Time) noz_float time_;

    public: KeyFrame(void);

    public: virtual void SetValue (Object* t, Property* p) {noz_assert(false);}

    public: void SetTime(noz_float t) {time_ = t;}

    public: noz_float GetTime(void) const {return time_;}

    public: virtual bool IsWeighted (void) const {return true;}

    public: virtual Type* GetValueType (void) const {return nullptr;}
    
    /// Return the property type the KeyFrame requires
    public: virtual Type* GetPropertyType(void) const = 0;
  };

}

#endif // __noz_KeyFrame_h__
