///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Animation/FloatKeyFrame.h>
#include <noz/Editor/Actions/SetKeyFrameTangentsAction.h>
#include "AnimationView.h"

using namespace noz;
using namespace noz::Editor;

AnimationCurveEditor::AnimationCurveEditor(void) {
  margin_ = 0.0f;
  value_scale_ = 1.0f;
  loop_curve_color_= Color::White;
  grid_color_ = Color::Black;
  zoom_ = 1.0f;
  curve_color_4_ = Color(255,255,0,255);
  curve_color_1_ = Color(255,0,0,255);
  curve_color_2_ = Color(0,255,0,255);
  curve_color_3_ = Color(0,0,255,255);
  selected_key_frame_color_= Color::White;
  time_offset_ = 0.0f;
  grid_spacing_ = 100.0f;
  time_ = 0.0f;
  drag_mode_ = DragMode::None;
  drag_key_ = nullptr;
  time_marker_color_ = Color::White;
  value_rect_height_= 0.0f;
  animation_view_ = nullptr;
  SetClipChildren(true);
}

AnimationCurveEditor::~AnimationCurveEditor(void) {
}

Vector2 AnimationCurveEditor::MeasureChildren (const Vector2& a) {
  UINode::MeasureChildren(a);
  return a;
}

void AnimationCurveEditor::ArrangeChildren (const Rect& r) {
  if(value_rect_height_ != r.h) UpdateValueRange();  
  UINode::ArrangeChildren(r);
}

void AnimationCurveEditor::OnLineageChanged (void) {
  UINode::OnLineageChanged();

  // Cache the animation view pointer since we will be using it a lot.
  animation_view_ = GetAncestor<AnimationView>();
}

void AnimationCurveEditor::SetZoom (noz_float z) {
  if(zoom_==z) return;
  zoom_ = z;
  InvalidateTransform();
}

void AnimationCurveEditor::SetTime (noz_float t) {
  time_ = t;
}

void AnimationCurveEditor::SetGridSpacing (noz_float s) {
  grid_spacing_ = s;
}

void AnimationCurveEditor::SetTimeOffset (noz_float o) {
  if(time_offset_==o) return;
  time_offset_ = o;
  InvalidateTransform();
}

void AnimationCurveEditor::OnMouseDown (SystemEvent* e) {
  Vector2 lpos = WindowToLocal(e->GetPosition());

  for(noz_uint32 i=0, c=animation_view_->GetSelectedKeyFrameCount(); i<c; i++) {
    CurveKey* ck = GetKey(animation_view_->GetSelectedKeyFrame(i).av_track_, animation_view_->GetSelectedKeyFrame(i).kf_index_);
    noz_assert(ck);

    if(ck->handle_rect_[0].Contains(lpos)) {
      e->SetHandled();
      drag_mode_ = DragMode::TangentIn;
      drag_key_ = ck;
      SetCapture();
      return;
    }

    if(ck->handle_rect_[1].Contains(lpos)) {
      e->SetHandled();
      drag_mode_ = DragMode::TangentOut;
      drag_key_ = ck;
      SetCapture();
      return;
    }
  }

  CurveKey* ck = HitTestKey (lpos);

  if(ck==nullptr) {
    animation_view_->UnselectAllKeyFrames();
  } else if(e->IsControl()) {
    if(animation_view_->IsKeyFrameSelected(ck->av_track_, ck->kf_index_)) {
      animation_view_->UnselectKeyFrame(ck->av_track_, ck->kf_index_);
    } else {
      animation_view_->SelectKeyFrame(ck->av_track_, ck->kf_index_);
    }
  } else {
    if(!e->IsShift()) animation_view_->UnselectAllKeyFrames();
    animation_view_->SelectKeyFrame(ck->av_track_, ck->kf_index_);
  }    

  if(e->GetButton() == MouseButton::Right) {
    const Rect& r = GetRectangle();
    animation_view_->OpenKeyFramePopup(ck->av_track_, ck->kf_index_, ck->rect_.GetBottomLeft()-r.GetTopLeft());
  }

  e->SetHandled();
}

void AnimationCurveEditor::OnMouseOver (SystemEvent* e) {
  if(drag_key_) {
    switch(drag_mode_) {
      case DragMode::TangentIn: {
        Vector2 lpos = WindowToLocal(e->GetPosition());
        Vector2 d = lpos - drag_key_->position_;
        KeyFrame* kf_prev = drag_key_->av_track_->GetKeyFrame(drag_key_->kf_index_-1);
        noz_float td = drag_key_->kf_->GetTime() - kf_prev->GetTime();
        d.x /= (pixels_per_second_*zoom_*td);
        d.y /= (value_scale_*zoom_);
        SetKeyFrameTangentsAction* action = new SetKeyFrameTangentsAction(
          (WeightedKeyFrame*)drag_key_->kf_, -d.y/d.x, false);
        EditorDocument::GetActiveDocument(this)->ExecuteAction(action,false);
        break;
      }

      case DragMode::TangentOut: {
        Vector2 lpos = WindowToLocal(e->GetPosition());
        Vector2 d = lpos - drag_key_->position_;
        KeyFrame* kf_next = drag_key_->av_track_->GetKeyFrame(drag_key_->kf_index_+1);
        noz_float td = kf_next->GetTime() - drag_key_->kf_->GetTime();
        d.x /= (pixels_per_second_*zoom_*td);
        d.y /= (value_scale_*zoom_);
        SetKeyFrameTangentsAction* action = new SetKeyFrameTangentsAction(
          (WeightedKeyFrame*)drag_key_->kf_, d.y/-d.x, true);
        EditorDocument::GetActiveDocument(this)->ExecuteAction(action,false);
        break;
      }
    }
  }
}

void AnimationCurveEditor::OnMouseUp (SystemEvent* e) {
  if(drag_key_) {
    ReleaseCapture();
    drag_key_ = nullptr;
    drag_mode_ = DragMode::None;
  }

  e->SetHandled();
}

void AnimationCurveEditor::RenderOverride (RenderContext* rc) {
  if(nullptr == animation_view_) return;

  const Rect& r = GetRectangle();

  rc->PushMatrix();
  rc->MultiplyMatrix(GetLocalToViewport());    

  Vector2 p=GetKeyFramePosition(r,0.0f,0.0f);
  rc->DrawDebugLine(Vector2(p.x,r.y),Vector2(p.x,r.y+r.h),grid_color_);

  for(p.x+=grid_spacing_;p.x<r.x+r.w;p.x+=grid_spacing_) {
    if(p.x < r.x) continue;
    rc->DrawDebugLine(Vector2(p.x,r.y),Vector2(p.x,r.y+r.h),grid_color_);
  }

  // Draw the curves
  for(noz_uint32 i=0,c=animation_view_->GetSelectedTrackCount(); i<c; i++) {
    AnimationViewTrack* av_track = animation_view_->GetSelectedTrack(i);      
    AnimationTrack* animation_track = av_track->GetAnimationTrack();
    noz_assert(animation_track);
    
    Color color = curve_color_4_;  
    if(animation_track->GetTrackId() != 3 && animation_track->GetTarget()->GetTargetProperty()->GetAnimationTrackCount()>1) {
      if(animation_track->GetTrackId()==0) color = curve_color_1_;
      else if(animation_track->GetTrackId()==1) color = curve_color_2_;
      else if(animation_track->GetTrackId()==2) color = curve_color_3_;
    }

    noz_float aw = animation_track->GetTarget()->GetAnimation()->GetDuration() * pixels_per_second_ * zoom_;

    if(animation_track->GetKeyFrameCount()==1) {
      KeyFrame* kf = animation_track->GetKeyFrame(0);
      Vector2 p = GetKeyFramePosition(r,kf);
      if(aw > 0) rc->DrawDebugLine(p,p+Vector2(aw,0.0f),color);
      if(p.x + aw < r.x+r.w) rc->DrawDebugLine(p + Vector2(aw,0.0f),Vector2(r.x+r.w,p.y),loop_curve_color_);
      continue;
    } else {
      for(noz_float xo=0.0f; xo < r.w; ) {
        noz_float w = 0.0f;
        for(noz_uint32 ii=0,cc=animation_track->GetKeyFrameCount()-1;ii<cc;ii++) {
          w += DrawCurve(rc,xo,animation_track->GetKeyFrame(ii),animation_track->GetKeyFrame(ii+1),color);
        }
        if(w < aw) {
          Vector2 p = GetKeyFramePosition(r, animation_track->GetKeyFrame(animation_track->GetKeyFrameCount()-1)) + Vector2(xo,0.0f);
          rc->DrawDebugLine(p,p+Vector2(aw-w,0.0f),color);
        }
        
        Vector2 p1 = GetKeyFramePosition(r, animation_track->GetKeyFrame(0)) + Vector2(xo+aw,0.0f);
        Vector2 p2 = GetKeyFramePosition(r, animation_track->GetKeyFrame(animation_track->GetKeyFrameCount()-1)) + Vector2(xo+(aw-w),0.0f);
        rc->DrawDebugLine(p1,p2,loop_curve_color_);
        xo += aw;
        color = loop_curve_color_;
      }
    }
  }

  // Draw current time.
  p=GetKeyFramePosition(r,time_,0.0f);
  rc->DrawDebugLine(Vector2(p.x,r.y),Vector2(p.x,r.y+r.h),time_marker_color_);

  // Draw the key frames.

 UpdateMeshes();
  if(!key_frame_mesh_.GetTriangles().empty()) rc->Draw(&key_frame_mesh_);
  if(!selected_key_frame_mesh_.GetTriangles().empty()) rc->Draw(&selected_key_frame_mesh_);
  
  if(!handle_mesh_.GetTriangles().empty()) {
    for(noz_uint32 i=0, c=animation_view_->GetSelectedKeyFrameCount(); i<c; i++) {
      CurveKey* ck = GetKey(animation_view_->GetSelectedKeyFrame(i).av_track_, animation_view_->GetSelectedKeyFrame(i).kf_index_);
      noz_assert(ck);
      rc->DrawDebugLine(ck->position_, ck->handle_rect_[0].GetCenter(), handle_color_);
      rc->DrawDebugLine(ck->position_, ck->handle_rect_[1].GetCenter(), handle_color_);
    }
    rc->Draw(&handle_mesh_);
  }

  rc->PopMatrix();

  // Render all children
  //UINode::RenderOverride(rc);
}

noz_float AnimationCurveEditor::DrawCurve (RenderContext* rc, noz_float offset, KeyFrame* kf1, KeyFrame* kf2, Color color) {
  FloatKeyFrame* fkf1 = (FloatKeyFrame*)kf1;
  FloatKeyFrame* fkf2 = (FloatKeyFrame*)kf2;

  const Rect& r = GetRectangle();
  noz_int32 c = Math::Max(1,noz_int32((kf2->GetTime() - kf1->GetTime()) / (5.0f / (pixels_per_second_ * zoom_))));
  noz_float ts = 1.0f / c;
  noz_float t = 0.0f;

  Vector2 pp = GetKeyFramePosition(r,kf1,kf2,0.0f);
  t+=ts;
  for(noz_int32 i=0;i<c;i++,t+=ts) {
    Vector2 p = GetKeyFramePosition(r,kf1,kf2,t);
    rc->DrawDebugLine(pp+Vector2(offset,0.0f),p+Vector2(offset,0.0f),color);
    pp = p;
  }

  return (kf2->GetTime()-kf1->GetTime()) * pixels_per_second_ * zoom_;
}

void AnimationCurveEditor::UpdateValueRange (void) {
  value_min_ = 100000.0f;
  value_max_ = -100000.0f;

  for(auto it=keys_.begin(); it!=keys_.end(); it++) {
    const CurveKey& ck = it->second;

    value_min_ = Math::Min(value_min_, ((FloatKeyFrame*)ck.kf_)->GetValue());
    value_max_ = Math::Max(value_max_, ((FloatKeyFrame*)ck.kf_)->GetValue());
  }

  if(value_max_ == value_min_) {
    value_min_ -= 1.0f;
    value_max_ += 1.0f;
  }

  noz_float range = value_max_ - value_min_;
  value_min_ -= (range * 0.1f);
  value_max_ += (range * 0.1f);
  value_scale_ = (GetRectangle().h / zoom_) / (value_max_ - value_min_);
  value_rect_height_ = GetRectangle().h;
}  

Vector2 AnimationCurveEditor::GetKeyFramePosition (const Rect& r, KeyFrame* kf) const {
  FloatKeyFrame* fkf = (FloatKeyFrame*)kf;
  return GetKeyFramePosition(r,fkf->GetTime(),fkf->GetValue());
}

Vector2 AnimationCurveEditor::GetKeyFramePosition (const Rect& r, noz_float t, noz_float v) const {
  return Vector2 (
    r.x + t * pixels_per_second_ * zoom_ + margin_ - time_offset_ * pixels_per_second_ * zoom_,
    r.y + r.h * 0.5f - (value_max_-value_min_) * value_scale_ * zoom_ * 0.5f + (value_max_ - v) * value_scale_ * zoom_
  );
}

Vector2 AnimationCurveEditor::GetKeyFramePosition (const Rect& r, const KeyFrame* kf1, const KeyFrame* kf2, noz_float t) const {  
  FloatKeyFrame* fkf1 = (FloatKeyFrame*)kf1;
  FloatKeyFrame* fkf2 = (FloatKeyFrame*)kf2;
  return GetKeyFramePosition(r,
    kf1->GetTime() + (kf2->GetTime()-kf1->GetTime()) * t,
    Math::EvaluateCurve(t,fkf1->GetValue(),fkf1->GetTangentOut(), fkf2->GetValue(),fkf2->GetTangentIn())
  );
}

void AnimationCurveEditor::UpdateMeshes(void) {
  UpdateSelectionMesh();
  UpdateHandleMesh();
  UpdateKeyMesh();
}

void AnimationCurveEditor::UpdateKeyMesh(void) {
  if(nullptr == key_frame_sprite_) return;

  const Rect& r = GetRectangle();

  key_frame_mesh_.Clear();
  key_frame_mesh_.SetImage(key_frame_sprite_->GetImage());

  for(auto it=keys_.begin(); it!=keys_.end(); it++) {
    CurveKey& ck = it->second;
    ck.position_ = GetKeyFramePosition(r,ck.kf_);
    ck.rect_ = Rect(
      ck.position_.x-key_frame_sprite_->GetSize().x*0.5f,
      ck.position_.y-key_frame_sprite_->GetSize().y*0.5f,
      key_frame_sprite_->GetSize().x,
      key_frame_sprite_->GetSize().y
    );

    key_frame_mesh_.AddQuad(ck.rect_.GetTopLeft(), ck.rect_.GetBottomRight(), key_frame_sprite_->GetS(), key_frame_sprite_->GetT(), ck.color_);
  }
}

void AnimationCurveEditor::UpdateSelectionMesh(void) {
  if(nullptr == selected_key_frame_sprite_) return;  

  selected_key_frame_mesh_.Clear();
  selected_key_frame_mesh_.SetImage(selected_key_frame_sprite_->GetImage());

  for(noz_uint32 i=0, c=animation_view_->GetSelectedKeyFrameCount(); i<c; i++) {
    CurveKey* ck = GetKey(animation_view_->GetSelectedKeyFrame(i).av_track_, animation_view_->GetSelectedKeyFrame(i).kf_index_);
    noz_assert(ck);

    Rect r (
      ck->position_.x-selected_key_frame_sprite_->GetSize().x*0.5f,
      ck->position_.y-selected_key_frame_sprite_->GetSize().y*0.5f,
      selected_key_frame_sprite_->GetSize().x,
      selected_key_frame_sprite_->GetSize().y
    );
    selected_key_frame_mesh_.AddQuad(r.GetTopLeft(), r.GetBottomRight(), selected_key_frame_sprite_->GetS(), selected_key_frame_sprite_->GetT(), selected_key_frame_color_);
  }
}

void AnimationCurveEditor::UpdateHandleMesh(void) {
  if(nullptr == handle_sprite_) return;

  const Rect& r = GetRectangle();

  handle_mesh_.Clear();
  handle_mesh_.SetImage(handle_sprite_->GetImage());

  for(noz_uint32 i=0, c=animation_view_->GetSelectedKeyFrameCount(); i<c; i++) {
    CurveKey* ck = GetKey(animation_view_->GetSelectedKeyFrame(i).av_track_, animation_view_->GetSelectedKeyFrame(i).kf_index_);
    noz_assert(ck);

    KeyFrame* kf_prev = ck->kf_index_ > 0 ? ck->av_track_->GetKeyFrame(ck->kf_index_-1) : nullptr;
    KeyFrame* kf_next = (ck->av_track_->GetKeyFrameCount()>1&&ck->kf_index_<ck->av_track_->GetKeyFrameCount()-1) ? ck->av_track_->GetKeyFrame(ck->kf_index_+1) : nullptr;

    ck->handle_rect_[0] = ck->position_;
    ck->handle_rect_[1] = ck->position_;

    if(kf_prev) {
      CurveKey& ck_prev = keys_[kf_prev];
      
      Vector2 v(pixels_per_second_ * (ck->kf_->GetTime()-kf_prev->GetTime()),((FloatKeyFrame*)ck->kf_)->GetTangentIn() * value_scale_);
      v = v.Normalized() * 50.0f;

      ck->handle_rect_[0] = Rect (
        ck->position_.x-handle_sprite_->GetSize().x*0.5f-v.x,
        ck->position_.y-handle_sprite_->GetSize().y*0.5f+v.y,
        handle_sprite_->GetSize().x,
        handle_sprite_->GetSize().y
      );

      handle_mesh_.AddQuad(ck->handle_rect_[0].GetTopLeft(), ck->handle_rect_[0].GetBottomRight(), handle_sprite_->GetS(), handle_sprite_->GetT(), handle_color_);
    }

    if(kf_next) {
      CurveKey& ck_prev = keys_[kf_next];
      
      Vector2 v(pixels_per_second_ * (kf_next->GetTime()-ck->kf_->GetTime()),((FloatKeyFrame*)ck->kf_)->GetTangentOut() * value_scale_);
      v = v.Normalized() * 50.0f;

      ck->handle_rect_[1] = Rect (
        ck->position_.x-handle_sprite_->GetSize().x*0.5f+v.x,
        ck->position_.y-handle_sprite_->GetSize().y*0.5f-v.y,
        handle_sprite_->GetSize().x,
        handle_sprite_->GetSize().y
      );

      handle_mesh_.AddQuad(ck->handle_rect_[1].GetTopLeft(), ck->handle_rect_[1].GetBottomRight(), handle_sprite_->GetS(), handle_sprite_->GetT(), handle_color_);
    }
  }
}

AnimationCurveEditor::CurveKey* AnimationCurveEditor::HitTestKey (const Vector2& pt) {
  for(auto it=keys_.begin(); it!=keys_.end(); it++) {
    CurveKey& ck = it->second;
    if(ck.rect_.Contains(pt)) return &ck;
  }

  return nullptr;
}

void AnimationCurveEditor::AddKey (AnimationViewTrack* av_track, noz_uint32 kf_index) {
  noz_assert(av_track);
  noz_assert(kf_index < av_track->GetKeyFrameCount());

  AnimationTrack* animation_track = av_track->GetAnimationTrack();
  noz_assert(animation_track);

  KeyFrame* kf = animation_track->GetKeyFrame(kf_index);
  noz_assert(kf);
  
  CurveKey& ck = keys_[kf];
  ck.av_track_ = av_track;
  ck.kf_ = kf;
  ck.kf_index_ = kf_index;

  ck.color_ = curve_color_4_;
  if(animation_track->GetTrackId() != 3 && animation_track->GetTarget()->GetTargetProperty()->GetAnimationTrackCount()>1) {
    if(animation_track->GetTrackId()==0) ck.color_ = curve_color_1_;
    else if(animation_track->GetTrackId()==1) ck.color_ = curve_color_2_;
    else if(animation_track->GetTrackId()==2) ck.color_ = curve_color_3_;
  }
}

void AnimationCurveEditor::UpdateTracks(void) {
  keys_.clear();

  for(noz_uint32 i=0,c=animation_view_->GetSelectedTrackCount(); i<c; i++) {
    AnimationViewTrack* av_track = animation_view_->GetSelectedTrack(i);
    noz_assert(av_track);

    AnimationTrack* animation_track = av_track->GetAnimationTrack();
    noz_assert(animation_track);

    for(noz_uint32 ii=0,cc=animation_track->GetKeyFrameCount(); ii<cc; ii++) {
      AddKey(av_track,ii);
    }
  }

  value_rect_height_ = 0.0f;
  InvalidateTransform();
}

AnimationCurveEditor::CurveKey* AnimationCurveEditor::GetKey (AnimationViewTrack* av_track, noz_uint32 kf_index) {
  for(auto it=keys_.begin(); it!=keys_.end(); it++) {
    CurveKey& ck = it->second;
    if(ck.av_track_ == av_track && ck.kf_index_ == kf_index) return &ck;
  }
  return nullptr;
}

void AnimationCurveEditor::InvalidateRender (void) {
  
}
