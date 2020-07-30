///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "EventBlendTarget.h"
#include "EventKeyFrame.h"

using namespace noz;

EventBlendTarget::EventBlendTarget(Object* target, Property* prop) : BlendTarget(target,prop) {
  noz_assert(target);
  noz_assert(nullptr==prop);
  last_frame_ = -1;
}

void EventBlendTarget::Advance (noz_float elapsed) {
#if 0
  // Assume not clamped..
  clamped_ = false;

  Blend* bp;
  for(Blend* b=blend_last_; b; b=bp) {
    bp = b->prev_;

    noz_float old_elapsed = b->elapsed_;

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
        if(b->elapsed_ > b->duration_) {
          FireEvents(b, b->track_->GetKeyFrameCount()-1);

          // Loop the time.
          while(b->elapsed_ > b->duration_) b->elapsed_ -= b->duration_;

          last_frame_ = -1;
        }

      // Once?
      } else {
        Stop(b);
        continue;
      }
    }

    // Fire events
    if(b->track_) {
      // Update the blend frame
      UpdateFrame (b);

      noz_float ff = b->track_->GetKeyFrame(b->frame_)->GetTime();
      FireEvents (b, b->elapsed_ >= b->track_->GetKeyFrame(b->frame_)->GetTime() ? b->frame_ : b->frame_ - 1);
    }

    return;
  }
#endif
}

void EventBlendTarget::FireEvents (Blend* b, noz_int32 f) {
#if 0
  if(last_frame_ == f) return;

  for(noz_int32 i=Math::Max(0,last_frame_);i<=f;i++) {
    EventKeyFrame* kf = Cast<EventKeyFrame>(b->track_->GetKeyFrame(i));
    if(nullptr == kf) continue;
    kf->Fire((Object*)GetTarget());
  }

  last_frame_ = f;
#endif
}
