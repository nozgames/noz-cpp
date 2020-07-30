///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Animator.h"
#include "BlendTarget.h"

using namespace noz;


Animator::Animator(void) {
  timescale_ = 1.0f;
  clamped_ = false;
  advancing_ = false;
  NOZ_TODO("Can we do this only when an animation is active?");
  NOZ_FIXME()
  //EnableEvent(ComponentEvent::Animate);
}

Animator::~Animator(void) {
  // Delete all blend targets
  for(auto it_target=blends_.begin(); it_target!=blends_.end(); it_target++) {
    delete (*it_target);
  }
  blends_.clear();

  // Delete all layers
  for(auto it=layers_.begin(); it!=layers_.end(); it++) {
    delete (*it);
  }
  layers_.clear();
}

void Animator::Animate(void) {
  if(!IsEnabled()) return;
  Advance(Time::GetDeltaTime());
}

void Animator::OnAwake (void) {
  SetState("");
}

void Animator::Stop (void) {
  // Stop all targets.
  for(auto it_target=blends_.begin(); it_target!=blends_.end(); it_target++) {
    (*it_target)->Stop(nullptr, 0.0f);
  }

  current_states_.clear();

  clamped_ = false;

  Advance(0);

  // Free all targets
  while(blends_.size()) {
    delete blends_.back();
    blends_.pop_back();
  }
}

void Animator::Advance(noz_float elapsed) {  
  // Early out if we know all of our values are clamped.
  if(IsClamped()) return;

  advancing_ = true;

  // Assume we are clamped.
  clamped_ = true;

  // Process all targets..
  for(auto it=blends_.begin(); it!=blends_.end(); ) {
    BlendTarget* bt = (*it);
    noz_assert(bt);

    // Skip the target if it is clamped already.
    if(bt->IsClamped()) {
      it++; 
      continue;
    }

    // Advance the target
    bt->Advance(elapsed);        

    // If the target is no longer animating then remove it.
    if(!bt->IsAnimating()) {
      delete bt;
      it = blends_.erase(it);
      continue;
    }

    // Update the clamped value.
    clamped_ &= bt->IsClamped();

    // Next target.
    it++;
  }

  advancing_ = false;

  // Is there a pending state?
  if(!pending_state_.IsEmpty()) {
    Name temp = pending_state_;
    pending_state_ = Name::Empty;
    SetState(temp);
  }
}

void Animator::SetState (const Name& name) {
  if(nullptr==controller_) return;

  if(advancing_) {
    pending_state_ = name;
    return;
  }

  // Ensure the curent state vector matches the controller layer count
  if(current_states_.size() != controller_->GetLayerCount()) current_states_.resize(controller_->GetLayerCount());

  for(noz_uint32 i=0,c=controller_->GetLayerCount(); i<c; i++) {
    AnimationLayer* layer = controller_->GetLayer(i);

    // Get the state for this layer, if it does not have one skip this layer
    AnimationState* state = layer->GetState(name);
    if(nullptr==state) continue;

    // If the state is already set on this layer then we are done
    if(current_states_[i] == state) continue;

    // No longer clamped if the state changed.
    clamped_ = false;

    // Save the current state.
    current_states_[i] = state;

    // Now express all of the state tracks as blends..
    Animation* animation = state->GetAnimation();

    // Stop the layer on all targets..
    for(auto it_target=blends_.begin(); it_target!=blends_.end(); it_target++) {
      (*it_target)->Stop(layer, state->GetBlendTime());
    }

    // No animation...
    if(nullptr == animation) continue;
    
    // Event track?
    if(animation->GetEventTrack()) {
      // Find an existing blend target for the mapped object and property
      BlendTarget* bt = nullptr;
      for(auto it_target=blends_.begin(); it_target!=blends_.end(); it_target++) {
        if((*it_target)->GetTarget()==GetNode() && (*it_target)->GetTargetProperty()==nullptr) {
          bt = (*it_target);
          break;
        }
      }

#if 0
      // If no blend target was found create a new one.
      if(nullptr==bt && GetNode()) bt = animation->GetEventTrack()->CreateBlendTarget(GetNode(),nullptr);

      if(nullptr!=bt) {
        blends_.push_back(bt);
        bt->Play (layer,animation->GetEventTrack(),animation->GetDuration(),animation->GetWrap(),0.0f);
      }
#endif
    }

    // Create blend targets for all animation targets
    for(noz_uint32 i=0,c=animation->GetTargetCount(); i<c; i++) {
      AnimationTarget* animation_target = animation->GetTarget(i);
      noz_assert(animation_target);
      if(nullptr == animation_target->GetTargetProperty()) continue;

      // Get the matching animator target
      AnimatorTarget* animator_target = GetTarget(animation_target->GetGuid());
      if(nullptr == animator_target) continue;

      // Ignore the track if the target is not mapped to an object
      if(nullptr == animator_target->object_) continue;

      // Ensure the target property is a property of the target object type
      if(!animator_target->object_->IsTypeOf(animation_target->GetTargetProperty()->GetParentType())) continue;

      // Find an existing blend target for the mapped object and property
      BlendTarget* bt = nullptr;
      for(auto it_target=blends_.begin(); it_target!=blends_.end(); it_target++) {
        // Match?
        if((*it_target)->GetTarget()==animator_target->object_ && (*it_target)->GetTargetProperty()==animation_target->GetTargetProperty()) {
          bt = (*it_target);
          break;
        }
      }

      // If no blend target was found create a new one.
      if(nullptr==bt) {
        // Create the blend target using the target property
        bt = animation_target->GetTargetProperty()->CreateBlendTarget(animator_target->object_);

        // Ensure the property supports animation
        if(nullptr==bt) continue;

        // Add the new blend target to the list.
        blends_.push_back(bt);
      }

      bt->Play (layer, animation_target, animation->GetDuration(), animation->GetWrap(), state->GetBlendTime());
    }
  }

  // Issue an advance of zero to immediately realize the state change.
  Advance(0);
}

void Animator::AddLayer (AnimatorLayer* layer) {
  layers_.push_back(layer);
  layer->index_ = layers_.size()-1;
}

void Animator::SetController (AnimationController* controller) {
  if(controller == controller_) return;
  NOZ_TODO("handle stopping and restarting current states");
  controller_ = controller;
}

void Animator::AddTarget (AnimatorTarget* animator_target) {
  targets_.push_back(animator_target);
}

AnimatorTarget* Animator::AddTarget (const Guid& guid, Object* target) {
  AnimatorTarget* result = new AnimatorTarget (guid, target);
  targets_.push_back(result);
  return result;
}

void Animator::ReleaseTarget (AnimatorTarget* target) {
  for(noz_uint32 i=0,c=targets_.size(); i<c; i++) {
    if(targets_[i] == target) {
      targets_.erase(targets_.begin() + i);
      return;
    }
  }  
}

void Animator::RemoveTarget (noz_uint32 index) {
  delete targets_[index];
  targets_.erase(targets_.begin() + index);  
}

void Animator::RemoveTarget (AnimatorTarget* target) {
  ReleaseTarget(target);
  delete target;
}

AnimatorTarget* Animator::GetTarget (const Guid& guid) const {
  for(noz_uint32 i=0,c=targets_.size(); i<c; i++) {
    if(targets_[i]->guid_ == guid) return targets_[i];
  }
  return nullptr;
}

AnimatorTarget* Animator::GetTarget (Animation* animation, Object* o) const {
  for(noz_uint32 i=0,c=targets_.size(); i<c; i++) {
    if(targets_[i]->object_ == o) {
      if(nullptr != animation->GetTarget(targets_[i]->GetGuid())) {
        return targets_[i];
      }
    }
  }
  return nullptr;
}
