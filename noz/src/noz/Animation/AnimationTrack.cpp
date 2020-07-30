///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationTrack.h"

using namespace noz;
  
AnimationTrack::AnimationTrack(void) {
  duration_ = 0;
  target_ = nullptr;
  track_id_ = 0;
}

void AnimationTrack::AddKeyFrame(KeyFrame* frame) {
  // When deserializing just add the keys unsorted
  if(IsDeserializing()) {
    frames_.push_back(frame);
    return;
  }

  // Update the track duration.
  duration_ = Math::Max(duration_,frame->time_);

  // Update the animation duration.
  if(target_) target_->UpdateDuration();

  if(frames_.empty() || frames_.back()->time_ < frame->time_) {
    frames_.push_back(frame);
    return;
  }

  for(noz_uint32 i=0,c=frames_.size(); i<c; i++) {
    if(frames_[i]->GetTime() > frame->GetTime()) {
      frames_.insert(frames_.begin()+i, frame);
      return;
    }
  }

  noz_assert(false);
}

void AnimationTrack::OnDeserialized (void) {
  Object::OnDeserialized();

  std::vector<KeyFrame*> frames = frames_;
  frames_.clear();
  for(noz_uint32 i=0,c=frames.size();i<c;i++) AddKeyFrame(frames[i]);
}

void AnimationTrack::UpdateDuration (void) {
  if(frames_.empty()) {
    duration_ = 0.0f;
  } else {
    duration_ = frames_.back()->GetTime();
  }

  if(target_) target_->UpdateDuration();
}

void AnimationTrack::ReleaseKeyFrame (KeyFrame* kf) {
  for(noz_uint32 i=0,c=GetKeyFrameCount();i<c;i++) {
    if(GetKeyFrame(i)==kf) {
      if(i==frames_.size()-1) {
        if(frames_.size()>1) {
          duration_ = frames_[frames_.size()-2]->time_;
        } else {
          duration_ = 0.0f;
        }
      }
      frames_.erase(frames_.begin()+i);
      if(target_) target_->UpdateDuration();
      return;
    }
  }

  noz_assert(false);
}

void AnimationTrack::RemoveKeyFrame (KeyFrame* kf) {
  ReleaseKeyFrame(kf);
  delete kf;
}

KeyFrame* AnimationTrack::GetKeyFrameByTime (noz_float time) const {
  for(noz_uint32 i=0,c=frames_.size(); i<c; i++) {
    if(frames_[i]->GetTime() == time) return frames_[i];
  }
  return nullptr;
}
