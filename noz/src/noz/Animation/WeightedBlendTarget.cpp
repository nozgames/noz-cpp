///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "WeightedBlendTarget.h"
#include "WeightedKeyFrame.h"

using namespace noz;
  
WeightedBlendTarget::WeightedBlendTarget(Object* target, Property* prop) : BlendTarget(target,prop) {
}

void WeightedBlendTarget::Advance(noz_float elapsed) {
  noz_float track_weight[MaxAnimationTracks];
  noz_float layer_weight[MaxAnimationTracks];

  memset(track_weight,0,sizeof(noz_float)*MaxAnimationTracks);
  memset(layer_weight,0,sizeof(noz_float)*MaxAnimationTracks);

  clamped_ = true;

  Blend* bn;
  Blend* bp = nullptr;
  for(Blend* b=blend_last_; b; bp=b, b=bn) {
    // Next blend to proces..
    bn = b->prev_;

    // Reset layer weights if layer switched..
    if(bp && bp->layer_ != b->layer_) {
      memset(layer_weight,0,sizeof(noz_float)*MaxAnimationTracks);
    }

    // Advance the elapsed time of the entire blend.
    b->elapsed_ += elapsed;

    // Blend in?
    noz_float blend_inout = 1.0f;
    if(b->blend_duration_ > 0.0f) {
      b->blend_elapsed_ += elapsed;
      if(b->blend_elapsed_ < b->blend_duration_) {
        blend_inout = (b->blend_elapsed_ / b->blend_duration_);
        clamped_ = false;
      } else {
        // Blend in finished..
        b->blend_duration_ = 0.0f;
        b->blend_elapsed_ = 0.0f;
      }
    // Blend out?
    } else if(b->blend_duration_ < 0.0f) {
      b->blend_elapsed_ -= elapsed;
      if(b->blend_elapsed_ > b->blend_duration_) {
        blend_inout = 1.0f - (b->blend_elapsed_ / b->blend_duration_);
        clamped_ = false;
      } else {
        // Blend out is finished so stop the blend entirely
        Stop(b);
        continue;
      }
    }

    // Check elapsed time on blend
    if(b->elapsed_ > b->duration_) {
      switch(b->wrap_) {
        case WrapMode::Clamp:
          b->elapsed_ = b->duration_;
          break;

        case WrapMode::Loop:
          clamped_ = false;

          // Loop the time.
          if(b->duration_ > 0) {          
            while(b->elapsed_ > b->duration_) b->elapsed_ -= b->duration_;
          } else {
            b->elapsed_ = 0;
          }
          break;

        case WrapMode::Once:  
          clamped_ = false;
          b->elapsed_ = b->duration_;
          if(blend_inout == 1.0f) {
            Stop(b);
            continue;
          }
          break;
      }
    } else {
      clamped_ = false;
    }

    // Update blend to correct frame.
    UpdateFrame(b);

    // Process each track in the target..
    for(noz_uint32 i=0,c=b->target_->GetTrackCount(); i<c; i++) {
      AnimationTrack* track = b->target_->GetTrack(i);
      noz_assert(track);

      // Skip the track if the first frame is not realized yet.
      if (b->elapsed_ < track->GetKeyFrames()[0]->GetTime()) continue;

      // Determine the amount the target contributes to the layer
      noz_float layer_contribution = (1.0f - layer_weight[i]) * blend_inout;      
      if(layer_contribution<=0.0f) continue;

      // Track total layer contributions.
      layer_weight[i] = Math::Min(1.0f,layer_weight[i]+layer_contribution);

      // Determine the weight of the blend..
      noz_float weight = b->layer_->GetWeight() * layer_contribution * (1.0f - track_weight[i]);
      if(weight<=0.0f) continue;

      // Is the frame for the blend not the last frame?
      if (b->frame_[i] < (noz_int32)track->GetKeyFrames().size()-1) {
        WeightedKeyFrame* kf1 = (WeightedKeyFrame*)track->GetKeyFrames()[b->frame_[i]];
        WeightedKeyFrame* kf2 = (WeightedKeyFrame*)track->GetKeyFrames()[b->frame_[i]+1];
        noz_float lerp = (b->elapsed_ - kf1->GetTime()) / (kf2->GetTime()-kf1->GetTime());      
        Add(track->GetTrackId(), kf1, kf2, lerp, weight);
        track_weight[i] += weight;

      // Weighted
      } else {
        Add(track->GetTrackId(), track->GetKeyFrames()[b->frame_[i]], weight);
        track_weight[i] += weight;
      }    
    }
  }
  
  // Add in default values for each animation track.
  for(noz_uint32 i=0,c=GetTargetProperty()->GetAnimationTrackCount(); i<c; i++) {
    if(track_weight[i] < 1.0f) AddDefault(i, 1.0f - track_weight[i]);
  }
}

bool WeightedBlendTarget::BlendedStop (Blend* blend, noz_float blend_time) {
  noz_assert(blend);
  noz_assert(blend_time>0.0f);
  
  // Already performing a blended stop?
  if(blend->blend_duration_ < 0.0f) {
    return true;
  }

  if(blend->blend_duration_ > 0.0f) {
    noz_float f = -(blend_time / blend->blend_duration_);
    blend->blend_elapsed_ = (blend->blend_duration_ - blend->blend_elapsed_) * f;
    blend->blend_duration_ *= f; 
  } else {
    // Negative blend time indicates a blend out.
    blend->blend_duration_ = -blend_time;
    blend->blend_elapsed_ = 0.0f;
  }

  clamped_ = false;

  return true;
}
