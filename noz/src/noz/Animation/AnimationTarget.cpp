///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationTarget.h"

using namespace noz;
  
AnimationTarget::AnimationTarget (void) {
  target_property_ =nullptr;
  animation_ = nullptr;
  duration_ = 0.0f;
}

AnimationTarget::AnimationTarget (Property* p, const Guid& guid) {
  noz_assert(p);

  target_property_ = p;
  guid_ = guid;
  animation_ = nullptr;
  duration_ = 0.0f;
}

void AnimationTarget::AddTrack(AnimationTrack* track) {
  noz_assert(track);
  noz_assert(track->target_==nullptr);

  // Link the track to the target
  track->target_ = this;

  // Update the duration to include the duration of this track
  duration_ = Math::Max(track->GetDuration(),duration_);

  // Add the track to the list of tracks.
  tracks_.push_back(track);
}

void AnimationTarget::ReleaseTrack (AnimationTrack* track) {
  for(noz_uint32 i=0, c=tracks_.size(); i<c; i++) {
    if(tracks_[i] == track) {
      track->target_ = nullptr;
      tracks_.erase(tracks_.begin()+i);
      UpdateDuration();
      return;
    }
  }

  // If we are here the track didnt belong to the target
  noz_assert(false);
}

void AnimationTarget::RemoveTrack (AnimationTrack* track) {
  ReleaseTrack(track);
  delete track;
}


void AnimationTarget::UpdateDuration(void) {
  duration_ = 0.0f;
  for(noz_uint32 i=0,c=tracks_.size();i<c;i++) {
    duration_ = Math::Max(duration_,tracks_[i]->GetDuration());
  }

  if(animation_) animation_->UpdateDuration();
}

AnimationTrack* AnimationTarget::GetTrackById (noz_uint32 id) const {
  for(noz_uint32 i=0,c=tracks_.size();i<c;i++) {
    if(tracks_[i]->track_id_ == id) return tracks_[i];
  }
  return nullptr;
}

BlendTarget* AnimationTarget::CreateBlendTarget(Object* target) {
  noz_assert(target);
  if(nullptr == target_property_) return nullptr;
  return target_property_->CreateBlendTarget(target);
}

void AnimationTarget::OnDeserialized (void) {
  Object::OnDeserialized();

  for(noz_uint32 i=0,c=tracks_.size(); i<c; i++) {
    for(noz_uint32 ii=c-1; ii>i; ii--) {
      if(tracks_[i]->track_id_ == tracks_[ii]->track_id_) {
        delete tracks_[ii];
        tracks_.erase(tracks_.begin()+ii);
        c--;
      }
    }
  }
}
