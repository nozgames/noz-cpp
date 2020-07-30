///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AddAnimationTargetAction_h__
#define __noz_Editor_AddAnimationTargetAction_h__

#include "Action.h"

namespace noz {
namespace Editor {

  class AddAnimationTargetAction : public Action {
    NOZ_OBJECT()

    private: ObjectPtr<Animation> animation_;
    private: ObjectPtr<AnimationTarget> animation_target_;

    public: AddAnimationTargetAction (Animation* animation, AnimationTarget* animation_target) :
      animation_(animation), animation_target_(animation_target) {
      noz_assert(animation);
      noz_assert(animation_target_);
    }

    public: ~AddAnimationTargetAction(void) {
      // TODO: free animation target if not in animation.  The problem is that 
    }

    public: AnimationTarget* GetAnimationTarget (void) const {return animation_target_;}

    /// Execute the action
    public: virtual void Do (void) final;

    public: virtual void Undo (void) final;
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_AddAnimationTargetAction_h__
