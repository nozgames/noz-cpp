///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_SetKeyFrameTangentsAction_h__
#define __noz_Editor_SetKeyFrameTangentsAction_h__

#include <noz/Animation/WeightedKeyFrame.h>
#include "Action.h"

namespace noz {
namespace Editor {

  class SetKeyFrameTangentsAction : public Action {
    NOZ_OBJECT()

    private: WeightedKeyFrame* kf_;
    private: noz_float tangent_;
    private: noz_float undo_tangent_;
    private: bool tangent_out_;

    public: SetKeyFrameTangentsAction (WeightedKeyFrame* kf, noz_float tangent, bool out) {
      kf_ = kf;
      undo_tangent_ = out ? kf->GetTangentOut() : kf->GetTangentIn();
      tangent_ = tangent;
      tangent_out_ = out;
    }

    public: virtual void Do (void) override {
      noz_assert(kf_);
      if(tangent_out_) {
        kf_->SetTangentOut(tangent_);
      } else {
        kf_->SetTangentIn(tangent_);
      }
    }

    public: virtual void Undo (void) override {
      noz_assert(kf_);
      if(tangent_out_) {
        kf_->SetTangentOut(undo_tangent_);
      } else {
        kf_->SetTangentIn(undo_tangent_);
      }
    }

    public: virtual bool CanMerge (Action* action) const override {
      if(!action->IsTypeOf(typeof(SetKeyFrameTangentsAction))) return false;
      if(((SetKeyFrameTangentsAction*)action)->tangent_out_ != tangent_out_) return false;
      return true;      
    }

    public: virtual void Merge (Action* action) override {
      noz_assert(action);
      noz_assert(action->IsTypeOf(typeof(SetKeyFrameTangentsAction)));
      tangent_ = ((SetKeyFrameTangentsAction*)action)->tangent_;
    }
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_SetKeyFrameTangentsAction_h__
