///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AnimationState_h__
#define __noz_AnimationState_h__

#include "Animation.h"

namespace noz {

  class AnimationState : public Object {
    NOZ_OBJECT()
    
    NOZ_PROPERTY(Name=Name,Set=SetName)
    private: Name name_;

    NOZ_PROPERTY(Name=Animation,Set=SetAnimation)
    private: ObjectPtr<Animation> animation_;

    NOZ_PROPERTY(Name=Speed)
    private: noz_float speed_;

    NOZ_PROPERTY(Name=BlendTime)
    private: noz_float blend_time_;

    /// Default constructor
    public: AnimationState(void);

    public: void SetName (const char* text);
    public: void SetName (const Name& name) {SetName(name.ToCString());}

    public: void SetAnimation (Animation* animation);

    public: noz_float GetBlendTime (void) const {return blend_time_;}

    public: const Name& GetName (void) const {return name_;}

    public: Animation* GetAnimation (void) const {return animation_;}
  };
}

#endif // __noz_AnimationState_h__
