///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "EventKeyFrame.h"
#include "EventBlendTarget.h"

using namespace noz;
  
EventKeyFrame::EventKeyFrame(void) {
  method_ = nullptr;
}

void EventKeyFrame::Fire (Object* target) {
  noz_assert(target);

  if(audio_clip_) audio_clip_->Play();

  if(!animation_state_.IsEmpty()) {
    if(target->IsTypeOf(typeof(Node))) ((Node*)target)->SetAnimationState(animation_state_);
  }

  if(method_) {
    if(target->IsTypeOf(typeof(Node)) && method_->GetParentType()->IsCastableTo(typeof(Component))) {
      Component* component = ((Node*)target)->GetComponent(method_->GetParentType());
      if(component) {
        method_->Invoke(component,0,nullptr); 
      }
    } else {
      method_->Invoke(target,0,nullptr); 
    }
  }
}