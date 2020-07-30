///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "BlendTarget.h"

using namespace noz;
  
BlendTarget::BlendTarget(Object* target, Property* property) {
  noz_assert(target);
  noz_assert(property);

  blend_last_ = nullptr;
  blend_first_ = nullptr;
  target_ = target;
  target_property_ = property;
  clamped_ = false;
}

BlendTarget::~BlendTarget(void) {
  Blend* bn;
  for(Blend* b=blend_first_; b; b=bn) {
    bn = b->next_;
    delete b;
  }
}

void BlendTarget::Play (AnimationLayer* layer, AnimationTarget* target, noz_float duration, WrapMode wrap, noz_float blend_duration) {
  noz_assert(layer);
  noz_assert(target);

  clamped_ = false;

  duration = duration >= 0 ? duration : (target?target->GetDuration():0.0f);

  // To accomidate targets with multiple tracks we determine the adjusted size of the 
  // Blend structure that contains frame indicators for each track.
  noz_uint32 blend_size = sizeof(Blend) + ((target->GetTrackCount()-1) * sizeof(noz_int32));
  
  // Allocate the blend and initialize its members.
  Blend* blend = (Blend*) new noz_byte[blend_size];
  memset(blend,0,blend_size);
  blend->layer_ = layer;
  blend->target_ = target;
  blend->duration_ = duration;
  blend->blend_duration_ = blend_duration;
  blend->wrap_ = wrap;

  // Find the blend to insert after.
  Blend* after=blend_last_;
  while (after && after->layer_->GetIndex() > layer->GetIndex()) after = after->prev_;

  // Add the blend into the blend list.
  if(after) {
    if(after->next_) after->next_->prev_ = blend;
    blend->next_ = after->next_;
    after->next_ = blend;
    blend->prev_ = after;
    if(blend_last_==after) blend_last_ = blend;
  } else {
    blend->next_ = blend_first_;
    if(blend_first_) blend_first_->prev_ = blend;
    blend_first_ = blend;
    if(nullptr==blend_last_) blend_last_ = blend;
  }
}

void BlendTarget::Stop (AnimationLayer* layer, noz_float blend_time) {
  // Find the last blend for the given layer.
  Blend* bp = nullptr;
  for(Blend* b=blend_last_; b; b=bp) {
    bp = b->prev_;
    if(b->layer_ == layer || layer==nullptr) {
      if(blend_time<=0.0f || !BlendedStop(b,blend_time)) {
        Stop(b);
      }
    }
  }
}

void BlendTarget::Stop (Blend* blend) {
  // Patch up next and previous pointers
  if(blend->prev_) blend->prev_->next_ = blend->next_;
  if(blend->next_) blend->next_->prev_ = blend->prev_;

  // Patch up first and last blends pointers
  if(blend_first_==blend) blend_first_ = blend->next_;
  if(blend_last_==blend) blend_last_ = blend->prev_;

  // Delete the blend.
  delete blend;

  clamped_ = false;
}

void BlendTarget::UpdateFrame (Blend* b) {
  if(b->target_ == nullptr) return;

  for(noz_uint32 i=0,c=b->target_->GetTrackCount();i<c;i++) {
    const std::vector<KeyFrame*>& frames = b->target_->GetTrack(i)->GetKeyFrames();

    // Cache..
    noz_int32 num_frames = (noz_int32)frames.size();

    // Forward..
    while(b->frame_[i]+1 < num_frames && b->elapsed_ >= frames[b->frame_[i]+1]->GetTime()) b->frame_[i]++;

    // Reverse
    while(b->frame_[i]>0 && b->elapsed_ < frames[b->frame_[i]]->GetTime()) b->frame_[i]--;
  }
}
