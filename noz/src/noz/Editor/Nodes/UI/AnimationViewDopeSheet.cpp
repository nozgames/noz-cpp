///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationView.h"

using namespace noz;
using namespace noz::Editor;

AnimationViewDopeSheet::AnimationViewDopeSheet(void) {
  animation_view_ = nullptr;
  track_height_ = 22.0f;
  track_color_ = Color::White;
  SetClipChildren(true);
}

AnimationViewDopeSheet::~AnimationViewDopeSheet(void) {
}

void AnimationViewDopeSheet::OnLineageChanged (void) {
  UINode::OnLineageChanged();

  // Cache the animation view pointer since we will be using it a lot.
  animation_view_ = GetAncestor<AnimationView>();
}

void AnimationViewDopeSheet::RenderOverride (RenderContext* rc) {
  const Rect& r = GetRectangle();

  // Handle case where animation view is not yet defined.
  if(nullptr==animation_view_ || nullptr==animation_view_->GetTimeline()) {
    UINode::RenderOverride(rc);
    return;
  }

  // Cache timeline
  AnimationTimeline* timeline = animation_view_->GetTimeline();

  // Caculate zero point
  Vector2 zp = GetZeroPoint();

  rc->PushMatrix();
  rc->MultiplyMatrix(GetLocalToViewport());    

  // If the tracks mesh is empty then repopulate it now.
  if(tracks_mesh_.IsEmpty()) {
    for(noz_uint32 i=0,c=animation_view_->GetVisibleTrackCount(); i<c; i++) {
      tracks_mesh_.AddQuad(Vector2(r.x,r.y + track_height_ * i), Vector2(r.x+r.w,r.y+track_height_*i+track_height_-1), Vector2::Zero, Vector2::One, track_color_);
    }
  }

  if(!tracks_mesh_.IsEmpty()) rc->Draw(&tracks_mesh_);

  // Draw time-zero line
  rc->DrawDebugLine(Vector2(zp.x,r.y),Vector2(zp.x,r.y+r.h),grid_color_);

  // Draw grid
  noz_float gs = timeline->GetLabelInterval() * timeline->GetPixelsPerSecond();
  for(noz_float x=zp.x+gs;x<r.x+r.w;x+=gs) {
    if(x < r.x) continue;
    rc->DrawDebugLine(Vector2(x,r.y),Vector2(x,r.y+r.h),grid_color_);
  }

  // Draw current time.
  noz_float xt = zp.x + timeline->GetTime() * timeline->GetPixelsPerSecond();
  rc->DrawDebugLine(Vector2(xt,r.y),Vector2(xt,r.y+r.h),current_time_color_);

  // Render tracks and their keys
  if(keys_mesh_.IsEmpty() && nullptr != key_sprite_) {
    keys_mesh_.SetImage(key_sprite_->GetImage());

    for(noz_uint32 ti=0, i=0, c=animation_view_->GetTrackCount(); i<c; i++) {
      AnimationViewTrack* av_track = animation_view_->GetTrack(i);
      noz_assert(av_track);

      if(av_track->GetVisibility() != Visibility::Visible) continue;
      if(nullptr == av_track->GetAnimationTrack()) continue;

      for(noz_uint32 i=0,c=av_track->GetAnimationTrack()->GetKeyFrameCount(); i<c; i++) {
        const KeyFrame* kf = av_track->GetAnimationTrack()->GetKeyFrame(i);
        RenderKey(zp, kf->GetTime(), av_track->GetRenderIndex(), false);
      }  
    }
  }

  if(selected_keys_mesh_.IsEmpty() && nullptr != selected_key_sprite_) {
    selected_keys_mesh_.Clear();
    selected_keys_mesh_.SetImage(selected_key_sprite_->GetImage());
      
    for(noz_uint32 i=0,c=animation_view_->GetSelectedKeyFrameCount(); i<c; i++) {
      const AnimationView::SelectedKeyFrame& skf = animation_view_->GetSelectedKeyFrame(i);
      RenderKey(zp, skf.kf_->GetTime(), skf.av_track_->GetRenderIndex(), true);
    }
  }

  if(!keys_mesh_.IsEmpty()) rc->Draw(&keys_mesh_);
  if(!selected_keys_mesh_.IsEmpty()) rc->Draw(&selected_keys_mesh_);

  rc->PopMatrix();

  UINode::RenderOverride(rc);
}

void AnimationViewDopeSheet::RenderKey (const Vector2& zp, noz_float kf_time, noz_int32 track_render_index, bool selected) {
  noz_float x = zp.x + kf_time * animation_view_->GetPixelsPerSecond();
  noz_float y = zp.y + track_render_index * track_height_ + track_height_ * 0.5f;

  if(selected) {
    selected_keys_mesh_.AddQuad(
      Vector2(x - selected_key_sprite_->GetSize().x * 0.5f, y - selected_key_sprite_->GetSize().y * 0.5f),
      Vector2(x + selected_key_sprite_->GetSize().x * 0.5f, y + selected_key_sprite_->GetSize().y * 0.5f),
      selected_key_sprite_->GetS(),
      selected_key_sprite_->GetT(),
      selected_key_color_
    );
  } else {
    keys_mesh_.AddQuad(
      Vector2(x - key_sprite_->GetSize().x * 0.5f, y - key_sprite_->GetSize().y * 0.5f),
      Vector2(x + key_sprite_->GetSize().x * 0.5f, y + key_sprite_->GetSize().y * 0.5f),
      key_sprite_->GetS(),
      key_sprite_->GetT(),
      key_color_
    );
  }
}

Vector2 AnimationViewDopeSheet::MeasureChildren (const Vector2& a) {
  if(animation_view_==nullptr) return Vector2::Empty;

  Vector2 size;
  for(noz_uint32 i=0, c=animation_view_->GetTrackCount(); i<c; i++) {
    AnimationViewTrack* av_track = animation_view_->GetTrack(i);
    noz_assert(av_track);

    AnimationTrack* animation_track = av_track->GetAnimationTrack();
    if(nullptr == animation_track) continue;

    noz_assert(animation_track);
    noz_assert(animation_track->GetKeyFrameCount()>0);

    // Get Last key frame in track.
    const KeyFrame* kf = animation_track->GetKeyFrame(animation_track->GetKeyFrameCount()-1);
    Rect kf_rect = GetKeyFrameRect(kf->GetTime(), av_track->GetRenderIndex());

    size.x = Math::Max(size.x, kf_rect.x + kf_rect.w);
  }

  // The animation view counts the number of visible tracks so the dopesheet's height is 
  // just a factor of the number of tracks and track height.
  size.y = track_height_ * AnimationView::GetAnimationView(this)->GetVisibleTrackCount();

  return size;
}

void AnimationViewDopeSheet::OnMouseDown (SystemEvent* e) {
  noz_assert(e);
  noz_assert(animation_view_);

  Vector2 lpos = WindowToLocal(e->GetPosition());  
  Vector2 zp = GetZeroPoint();

  // Determine the track index.
  noz_int32 track_index = (noz_int32)((lpos.y - zp.y) / track_height_);

  // Find the matching render track.
  AnimationViewTrack* av_track = nullptr;
  for(noz_uint32 i=0,c=animation_view_->GetTrackCount(); i<c && nullptr == av_track; i++) {
    av_track = animation_view_->GetTrack(i);
    noz_assert(av_track);
    if(av_track->GetRenderIndex() != track_index) av_track=nullptr;
  }

  if(nullptr == av_track) {
    // TODO: click in white space...
    return;
  }

  // Get associated animation track
  AnimationTrack* animation_track = av_track->GetAnimationTrack();
  if(animation_track==nullptr) return;

  noz_assert(animation_track);

  noz_uint32 kf_index = 0;
  for(noz_uint32 c=animation_track->GetKeyFrameCount(); kf_index<c; kf_index++) {
    const KeyFrame* kf = animation_track->GetKeyFrame(kf_index);
    noz_assert(kf);

    Rect kf_rect = GetKeyFrameRect(zp, kf->GetTime(), av_track->GetRenderIndex());
    if(kf_rect.Contains(lpos)) break;
  }

  // No matching key frame found?
  if(kf_index >= animation_track->GetKeyFrameCount()) {
    // TODO: click in white space...
    return;
  }

  if(e->GetButton() == MouseButton::Left || 
     (e->GetButton() == MouseButton::Right && !animation_view_->IsKeyFrameSelected(av_track, kf_index))) {
    if(e->IsControl()) {
      if(animation_view_->IsKeyFrameSelected(av_track, kf_index)) {
        animation_view_->UnselectKeyFrame(av_track, kf_index);
      } else {
        animation_view_->SelectKeyFrame(av_track, kf_index);
      }
    } else {
      if(!e->IsShift()) animation_view_->UnselectAllKeyFrames();
      animation_view_->SelectKeyFrame(av_track, kf_index);
    }  
  }

  if(e->GetButton() == MouseButton::Right) {
    AnimationTimeline* timeline = animation_view_->GetTimeline();
    noz_assert(timeline);

    Rect r = GetKeyFrameRect(Vector2(timeline->GetMargin(),0.0f), animation_track->GetKeyFrame(kf_index)->GetTime(), track_index);

    animation_view_->OpenKeyFramePopup(av_track, kf_index, r.GetBottomLeft());
  }
}

void AnimationViewDopeSheet::OnTrackSelected(AnimationViewTrack* av_track) {
  noz_assert(av_track);
  noz_assert(av_track->IsSelected());

  AnimationTrack* animation_track = av_track->GetAnimationTrack();
  noz_assert(animation_track);

  for(noz_uint32 i=0,c=animation_track->GetKeyFrameCount(); i<c; i++) {
    animation_view_->SelectKeyFrame(av_track, i);
  }
}

Rect AnimationViewDopeSheet::GetKeyFrameRect (noz_float kf_time, noz_int32 track_render_index) const {
  Rect r;
  r.x = animation_view_->GetPixelsPerSecond() * kf_time - key_sprite_->GetSize().x * 0.5f;
  r.y = track_render_index * track_height_ + track_height_ * 0.5f - key_sprite_->GetSize().y * 0.5f;
  r.w = key_sprite_->GetSize().x;
  r.h = key_sprite_->GetSize().y;
  return r;
}

Rect AnimationViewDopeSheet::GetKeyFrameRect (const Vector2& zp, noz_float kf_time, noz_int32 track_render_index) const {
  Rect r = GetKeyFrameRect (kf_time, track_render_index);
  r.x += zp.x;
  r.y += zp.y;
  return r;
}

Vector2 AnimationViewDopeSheet::GetZeroPoint (void) {
  AnimationTimeline* timeline = animation_view_->GetTimeline();
  noz_assert(timeline);

  Vector2 zp = GetRectangle().GetTopLeft();
  zp.x -= (timeline->GetTimeOffset() * timeline->GetPixelsPerSecond());
  zp.x += (timeline->GetMargin());    
  return zp;
}

void AnimationViewDopeSheet::InvalidateRender (void) {
  tracks_mesh_.Clear();
  keys_mesh_.Clear();
  selected_keys_mesh_.Clear();
}

void AnimationViewDopeSheet::InvalidateSelectionRender (void) {
  selected_keys_mesh_.Clear();
}

void AnimationViewDopeSheet::ArrangeChildren (const Rect& r) {
  InvalidateRender();
  UINode::ArrangeChildren(r);
}