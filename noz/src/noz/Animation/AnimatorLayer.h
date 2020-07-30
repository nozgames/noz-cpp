///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Animation_AnimatorLayer_h__
#define __noz_Animation_AnimatorLayer_h__

#include "Animation.h"

namespace noz {

  class AnimatorState : public Object {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Name)
    public: Name name_;

    NOZ_PROPERTY(Name=Animation)
    public: ObjectPtr<Animation> animation_;

    NOZ_PROPERTY(Name=BlendTime)
    public: noz_float blend_time_;

    public: AnimatorState(void) {blend_time_ = 0.0f;}
  };

  class AnimatorLayer : public Object {
    NOZ_OBJECT()

    friend class Animator;

    NOZ_PROPERTY(Name=States)
    private: std::vector<AnimatorState*> states_;

    // TODO: Additive or Override

    /// Index of the layer within the Animator
    private: noz_int32 index_;

    private: noz_float weight_;

    private: AnimatorState* current_state_;

    private: noz_float elapsed_;

    public: AnimatorLayer(void);

    public: ~AnimatorLayer(void);

    public: AnimatorState* GetState(const Name& name) const;

    public: noz_int32 GetIndex(void) const {return index_;}

    public: noz_float GetWeight(void) const {return weight_;}
  };

} // namespace noz


#endif // __noz_Animation_AnimatorLayer_h__


