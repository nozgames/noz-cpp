///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AnimationTrack_h__
#define __noz_AnimationTrack_h__

#include "KeyFrame.h"

namespace noz {

  class Animation;
  class BlendTarget;
  class AnimationTarget;

  class AnimationTrack : public Object {
    NOZ_OBJECT()

    friend class Animation;
    friend class AnimationTarget;

    /// Sorted vector of key frames.
    protected: NOZ_PROPERTY(Name=KeyFrames,Add=AddKeyFrame) std::vector<KeyFrame*> frames_;

    /// Optional track name
    protected: NOZ_PROPERTY(Name=Name) Name name_;

    /// Unique identifier used by blend target
    protected: NOZ_PROPERTY(Name=TrackId) noz_uint32 track_id_;

    /// Calculated duration in seconds for the entire track
    protected: noz_float duration_;

    /// Backwards reference to target that owns the track.
    protected: AnimationTarget* target_;

    /// Construct a new animation track
    public: AnimationTrack(void);

    public: void AddKeyFrame (KeyFrame* frame);

    public: const Name& GetName (void) const {return name_;}

    /// Return the duration of the animation track.
    public: noz_float GetDuration(void) const {return duration_;}

    public: AnimationTarget* GetTarget (void) const {return target_;}

    public: noz_uint32 GetTrackId (void) const {return track_id_;}

    /// Return the key frames in the track.
    public: const std::vector<KeyFrame*>& GetKeyFrames(void) const {return frames_;}

    public: noz_uint32 GetKeyFrameCount(void) const {return frames_.size();}

    public: KeyFrame* GetKeyFrame (noz_uint32 i) const {return frames_[i];}

    public: KeyFrame* GetKeyFrameByTime (noz_float time) const;

    public: void SetTrackId (noz_uint32 id) {track_id_ = id;}

    public: void SetName (const Name& name) {name_ = name;}

    public: void RemoveKeyFrame (KeyFrame* kf);

    public: void ReleaseKeyFrame (KeyFrame* kf);

    public: virtual void OnDeserialized (void) override;

    public: void UpdateDuration (void);
  };

}

#endif // __noz_Animation_Track_h__
