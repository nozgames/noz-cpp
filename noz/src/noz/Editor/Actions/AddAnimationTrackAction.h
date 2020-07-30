///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_SetAnimatorTargetAction_h__
#define __noz_Editor_SetAnimatorTargetAction_h__

#include "Action.h"

namespace noz {
namespace Editor {

  class SetAnimatorTargetAction : public Action {
    NOZ_OBJECT()

    private: ObjectPtr<Animator> animator_;
    private: ObjectPtr<Object> target_;
    private: Guid target_guid_;

    public: SetAnimatorTargetAction (Animator* animator, const Guid& target_guid, Object* target) : 
      animator_(animator), target_guid_(target_guid), target_(target) { 
      noz_assert(animator_);
      noz_assert(!target_guid.IsEmpty());
    }

    public: Object* GetTarget (void) const {return target_;}

    public: Animator* GetAnimator (void) const {return animator_;}

    /// Execute the action
    public: virtual bool Execute (void) final {
      noz_assert(animator_);
      noz_assert(!target_guid.IsEmpty());

      // Null target indicates the target should be remoted
      if(nullptr == target_) {
        animator_->RemoveTarget(target_guid_);
        return true;
      }

      AnimatorTarget* animator_target = animator_->GetTarget(target_guid);
      if(nullptr == animator_target) {
        
      }

      return true;
    }

    public: virtual Action* CreateUndoAction (void) const final {
      noz_assert(animator_);
      noz_assert(!target_guid.IsEmpty());
      return new SetAnimatorTargetAction(animator_,target_guid,animator_target);
    }
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_SetAnimatorTargetAction_h__
