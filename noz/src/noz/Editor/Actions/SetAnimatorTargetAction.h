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
    private: ObjectPtr<Object> undo_target_;
    private: ObjectPtr<AnimatorTarget> animator_target_;
    private: bool animator_target_added_;

    public: SetAnimatorTargetAction (Animator* animator, const Guid& target_guid, Object* target) {
      animator_ = animator;
      animator_target_ = animator->GetTarget(target_guid);
      animator_target_added_ = animator_target_ == nullptr;
      if(animator_target_added_) {
        animator_target_ = new AnimatorTarget(target_guid, target);
      } else {
        undo_target_= animator_target_->GetTarget();
      }
    }

    public: bool IsAnimatorTargetAdded (void) const {return animator_target_added_;}

    public: Object* GetTarget (void) const {return target_;}

    public: Animator* GetAnimator (void) const {return animator_;}

    public: AnimatorTarget* GetAnimatorTarget (void) const {return animator_target_;}

    public: virtual void Do (void) final;

    public: virtual void Undo (void) final;

  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_SetAnimatorTargetAction_h__
