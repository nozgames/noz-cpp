///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "FixedBlendTarget.h"

using namespace noz;

FixedBlendTarget::FixedBlendTarget(Object* target, Property* prop) : BlendTarget(target,prop) {
  last_blend_ = nullptr;
  last_frame_ = -1;
}

void FixedBlendTarget::Advance (noz_float elapsed) {
  clamped_ = false;

  Blend* bp;
  for(Blend* b=blend_last_; b; b=bp) {
    bp = b->prev_;

    // Advance the main timer.
    b->elapsed_ += elapsed;

    // Is the blend at the end of its duration?
    if(b->elapsed_ > b->duration_) {
      // Clamp to end?
      if(b->wrap_ == WrapMode::Clamp) {
        b->elapsed_ = b->duration_;

        clamped_ = true;

      // Loop ?
      } else if(b->wrap_ == WrapMode::Loop) {
        // Loop the time.
        while(b->elapsed_ > b->duration_) b->elapsed_ -= b->duration_;

      // Once?
      } else {
        Stop(b);
        continue;
      }
    }

    // Default..
    if(nullptr == b->target_) break;

    // Update the blend frame
    UpdateFrame (b);    

    AnimationTrack* track = b->target_->GetTrack(0);
    noz_assert(track);

    // Set the value
    if(b->elapsed_ >= track->GetKeyFrame(b->frame_[0])->GetTime() && 
       !(last_blend_ == b && last_frame_ == b->frame_[0]) ) {
      // Set the value
      Set(track->GetKeyFrames()[b->frame_[0]]);  

      // Save the last blend and frame that was executed
      last_blend_ = b;
      last_frame_ = b->frame_[0];
    }

    return;
  }

  // Early out so we dont keep setting the property over and over
  if(last_blend_ == nullptr) return;

  last_blend_ = nullptr;
  last_frame_ = -1;

  SetDefault();
}

