///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_SetKeyFrameAction_h__
#define __noz_Editor_SetKeyFrameAction_h__

#include "Action.h"
#include <noz/Animation/EnumKeyFrame.h>
#include <noz/Animation/FloatKeyFrame.h>

namespace noz {
namespace Editor {

  class SetKeyFrameAction : public Action {
    NOZ_OBJECT(Abstract)

    friend class AnimationView;

    protected: AnimationTarget* animation_target_;
    protected: AnimationTrack* animation_track_;
    protected: bool animation_track_added_;
    protected: bool key_frame_added_;

    public: AnimationTrack* GetAnimationTrack(void) const {return animation_track_;}

    public: AnimationTarget* GetAnimationTarget(void) const {return animation_target_;}
  };

  template <typename KT, typename VT> class SetKeyFrameActionT : public SetKeyFrameAction {
    NOZ_TEMPLATE()

    private: KT* kf_;
    private: VT value_;
    private: VT undo_value_;

    public: SetKeyFrameActionT (AnimationTarget* animation_target, noz_uint32 track_id, noz_float time, const VT& value) {
      animation_target_ = animation_target;
      value_ = value;

      animation_track_ = animation_target->GetTrackById(track_id);
      animation_track_added_ = nullptr == animation_track_;
      if(animation_track_added_) {
        animation_track_ = new AnimationTrack;
        animation_track_->SetTrackId(track_id);
      }

      kf_ = (KT*)animation_track_->GetKeyFrameByTime(time);
      key_frame_added_ = kf_ == nullptr;
      if(key_frame_added_) {
        kf_ = new KT(time, value);
      } else {
        undo_value_ = kf_->GetValue();
      }
    }

    public: virtual void Do (void) override {
      noz_assert(animation_target_);

      // Add the key frame..
      if(key_frame_added_) {
        animation_track_->AddKeyFrame(kf_);

      // Set the key frame..
      } else {
        kf_->SetValue(value_);
      }

      // Add the animation track to the target if this is the first key frame in the track
      if(animation_track_added_) animation_target_->AddTrack(animation_track_);
    }

    public: virtual void Undo (void) override {
      // Remove the key frame..
      if(key_frame_added_) {
        animation_track_->ReleaseKeyFrame(kf_);

      // Set the previous key frame value.
      } else {
        kf_->SetValue(undo_value_);
      }

      // Remove the track from the target if it was added
      if(animation_track_added_) animation_target_->ReleaseTrack(animation_track_);
    }

    public: virtual bool CanMerge (Action* a) const override {
      return (a->GetType() == GetType());      
    }

    public: virtual void Merge (Action* a) override {
      noz_assert(a->GetType() == GetType());
      value_ = ((SetKeyFrameActionT*)a)->value_;
    }

    public: const VT& GetValue(void) const {return value_;}
  };

  class SetEnumKeyFrameAction : public SetKeyFrameActionT<EnumKeyFrame, Name> {
    NOZ_OBJECT()
    public: SetEnumKeyFrameAction (AnimationTarget* animation_target, noz_uint32 track_id, noz_float time, const Name& value) : SetKeyFrameActionT (animation_target, track_id, time, value) {}
  };

  class SetFloatKeyFrameAction : public SetKeyFrameActionT<FloatKeyFrame, noz_float> {
    NOZ_OBJECT()
    public: SetFloatKeyFrameAction (AnimationTarget* animation_target, noz_uint32 track_id, noz_float time, noz_float value) : SetKeyFrameActionT (animation_target, track_id, time, value) {}
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_SetKeyFrameAction_h__
