///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_RemoveKeyFrameAction_h__
#define __noz_Editor_RemoveKeyFrameAction_h__

#include "Action.h"

namespace noz {
namespace Editor {

  class RemoveKeyFrameAction : public Action {
    NOZ_OBJECT()

    friend class AnimationView;

    protected: AnimationTarget* animation_target_;
    protected: AnimationTrack* animation_track_;
    protected: KeyFrame* kf_;
    protected: noz_uint32 kf_index_;
    protected: bool animation_track_removed_;

    public: RemoveKeyFrameAction (AnimationTrack* animation_track, noz_uint32 kf_index) {
      animation_track_ = animation_track;
      animation_target_ = animation_track->GetTarget();
      kf_index_ = kf_index;
      kf_ = animation_track->GetKeyFrame(kf_index);
      animation_track_removed_ = animation_track_->GetKeyFrameCount()==1;
    }

    public: virtual void Do (void) override {
      noz_assert(animation_track_);
      noz_assert(animation_track_->GetKeyFrameByTime(kf_->GetTime())==kf_);
      noz_assert(animation_target_);

      animation_track_->ReleaseKeyFrame(kf_);

      if(animation_track_removed_) animation_target_->ReleaseTrack(animation_track_);
    }

    public: virtual void Undo (void) override {
      noz_assert(animation_track_);
      noz_assert(animation_target_);

      animation_track_->AddKeyFrame(kf_);
      
      if(animation_track_removed_) animation_target_->AddTrack(animation_track_);
    }
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_RemoveKeyFrameAction_h__
