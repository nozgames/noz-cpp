///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Animation.h"
#include "EventKeyFrame.h"

using namespace noz;


Animation::Animation(void) {
  duration_ = 0.0f;
  looping_ = false;
  event_track_ = nullptr;
  wrap_ = WrapMode::Clamp;
}

void Animation::AddTarget(AnimationTarget* target) {
  noz_assert(target);
  noz_assert(target->animation_==nullptr);

  // Add the target to the list of targets
  targets_.push_back(target);

  // Link the target to the animation
  target->animation_ = this;
}

void Animation::ReleaseTarget(AnimationTarget* target) {
  // Remove all tracks associated with the target.
  for(noz_uint32 i=0,c=targets_.size(); i<c; i++) {
    if(targets_[i] == target) {
      targets_[i]->animation_ = nullptr;
      targets_.erase(targets_.begin()+i);      
      return;
    }
  }  

  // If here the target did not belong to the animation
  noz_assert(false);
}

void Animation::RemoveTarget(AnimationTarget* target) {
  ReleaseTarget(target);
  delete target;
}

AnimationTarget* Animation::GetTarget (const Guid& guid) const {
  for(noz_uint32 i=0,c=targets_.size(); i<c; i++) {
    if(guid == targets_[i]->GetGuid()) return targets_[i];
  }
  return nullptr;
}


AnimationTarget* Animation::AddTarget (Property* p, const Guid& guid) {
  noz_assert(p);  

  AnimationTarget* animation_target = new AnimationTarget(p, guid.IsEmpty() ? Guid::Generate() : guid);
  targets_.push_back(animation_target);
  return animation_target;
}

void Animation::UpdateDuration (void) {
  duration_ = 0.0f;
  for(noz_uint32 i=0,c=targets_.size(); i<c; i++) {
    duration_ = Math::Max(duration_,targets_[i]->duration_);
  }
  if(event_track_) {
    duration_ = Math::Max(duration_,event_track_->duration_);
  }
}

void Animation::SetEventTrack(AnimationTrack* track) {
  if(event_track_) delete event_track_;
  event_track_ = track;
  if(event_track_) {
    event_track_->UpdateDuration();
  }
}

void Animation::AddEvent (EventKeyFrame* kf) {
#if 0
  noz_assert(kf);
  if(nullptr==event_track_) {
    event_track_ = new AnimationTrack;
    event_track_->animation_ = this;
  }

  event_track_->AddKeyFrame(kf);
#endif
}
