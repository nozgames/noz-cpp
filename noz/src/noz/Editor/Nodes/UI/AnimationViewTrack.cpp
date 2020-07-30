///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationView.h"

using namespace noz;
using namespace noz::Editor;

AnimationViewTrack::AnimationViewTrack(AnimationTarget* animation_target, AnimatorTarget* animator_target, AnimationTrack* animation_track) :
  AnimationViewTrack(animation_target,animator_target,animation_track->GetTrackId()) {
  animation_track_ = animation_track;
}

AnimationViewTrack::AnimationViewTrack(AnimationTarget* animation_target, AnimatorTarget* animator_target, noz_uint32 track_id) {
  parent_track_ = nullptr;
  animation_track_ = nullptr;
  animation_target_ = animation_target;
  animator_target_ = animator_target;

  SetLogicalChildrenOnly();

  render_index_ = -1;
  track_id_ = track_id;

  selected_ = false;
  expanded_ = false;

  noz_assert(animation_target_);
}

AnimationViewTrack::~AnimationViewTrack(void) {
}

bool AnimationViewTrack::OnApplyStyle(void) {
  if(!ContentControl::OnApplyStyle()) return false;

  Refresh();

  if(expand_button_) {
    expand_button_->SetChecked(expanded_);
    expand_button_->Click += ClickEventHandler::Delegate(this,&AnimationViewTrack::OnExpandButtonClick);
  }

  if(options_button_) {
    if(GetParentTrack()==nullptr) {
      options_button_->Click += ClickEventHandler::Delegate(this,&AnimationViewTrack::OnOptionsButtonClick);
    } else {
      options_button_->SetVisibility(Visibility::Collapsed);
    }
  }

  if(tracks_container_) {
    for(noz_uint32 i=0,c=GetLogicalChildCount();i<c;i++) tracks_container_->AddChild(GetLogicalChild(i));
    tracks_container_->SetVisibility(IsExpanded() ? Visibility::Visible : Visibility::Collapsed);
  }

  // Force an update of the expanded state
  bool expanded = expanded_;
  expanded_ = !expanded_;
  SetExpanded(expanded);

  return true;
}

void AnimationViewTrack::OnStyleChanged (void) {
  ContentControl::OnStyleChanged();

  for(noz_uint32 i=0,c=GetLogicalChildCount();i<c;i++) {
    GetLogicalChild(i)->Orphan(false);
  }

  tracks_container_ = nullptr;
}

void AnimationViewTrack::UpdateAnimationState(void) {
  ContentControl::UpdateAnimationState();
  
  // Selected state
  SetAnimationState (selected_ ? UI::StateSelected : UI::StateUnSelected );

  // Check state indicates the track is unlinked 
  SetAnimationState (GetAnimatorTarget() ? UI::StateUnChecked : UI::StateChecked);
}

void AnimationViewTrack::Refresh(void) {
  if(nullptr != parent_track_) {
    SetText(String::Format("%s.%s", 
      animation_target_->GetTargetProperty()->GetName().ToCString(),
      animation_target_->GetTargetProperty()->GetAnimationTrackName(track_id_).ToCString())
    );
    if(expand_button_) expand_button_->SetVisibility(Visibility::Hidden);
  } else {
    String target_name;
    Type* target_type = nullptr;
    if(animator_target_ && animator_target_->GetTarget()) {
      Object* t = animator_target_->GetTarget();
      String node_name;
      String component_name;
      if(t->IsTypeOf(typeof(Node))) {
        if(((Node*)t)->GetName().IsEmpty()) {
          target_name = ((Node*)t)->GetType()->GetEditorName().ToCString();
        } else {
          target_name = ((Node*)t)->GetName();
        }
      } else if (t->IsTypeOf(typeof(Component))) {
        const Name& node_name = ((Component*)t)->GetNode()->GetName();
        if(node_name.IsEmpty()) {
          target_name = String::Format("%s.%s", ((Component*)t)->GetNode()->GetType()->GetEditorName().ToCString(), t->GetType()->GetEditorName().ToCString());
        } else {
          target_name = String::Format("%s.%s", node_name.ToCString(), t->GetType()->GetEditorName().ToCString());
        }
      } else {
        target_name = t->GetType()->GetEditorName();
      }
      target_type = t->GetType();
    } else {
      target_type = animation_target_->GetTargetProperty()->GetParentType();

      if(animation_track_ && !animation_track_->GetName().IsEmpty()) {
        target_name = animation_track_->GetName();
      } else {
        target_name = target_type->GetEditorName();    
      }
    }

    SetSprite(EditorFactory::CreateTypeIcon(target_type));
    SetText(String::Format("%s.%s", 
      target_name.ToCString(),
      animation_target_->GetTargetProperty()->GetName().ToCString())
    );
    if(expand_button_) expand_button_->SetVisibility(child_tracks_.empty() ? Visibility::Hidden : Visibility::Visible);
  }  

  // Refresh all child tracks as well.
  for(noz_uint32 i=0,c=child_tracks_.size(); i<c; i++) child_tracks_[i]->Refresh();
}

void AnimationViewTrack::OnChildAdded (Node* child) {
  ContentControl::OnChildAdded(child);

  if(tracks_container_) tracks_container_->AddChild(child);
}

void AnimationViewTrack::OnExpandButtonClick(UINode*) {
  SetExpanded(expand_button_->IsChecked());
}

void AnimationViewTrack::SetExpanded (bool v) {
  if(expanded_ == v) return;
  if(expand_button_) expand_button_->SetChecked(v);
  expanded_ = v;

  for(noz_uint32 i=0,c=child_tracks_.size(); i<c; i++) {
    AnimationViewTrack* av_track = child_tracks_[i];
    noz_assert(av_track);

    av_track->SetVisibility(expanded_ ? Visibility::Visible : Visibility::Collapsed);
  }

  GetAncestor<AnimationView>()->OnTrackExpanded(this);
}

void AnimationViewTrack::OnMouseDown(SystemEvent* e) {
  noz_assert(e);
  noz_assert(AnimationView::GetAnimationView(this));
  AnimationView::GetAnimationView(this)->OnMouseDown(e,this);
}

void AnimationViewTrack::OnOptionsButtonClick(UINode*) {
  AnimationView::GetAnimationView(this)->OnOptionsButton(this);
}

void AnimationViewTrack::SetAnimatorTarget(AnimatorTarget* t) {
  noz_assert(nullptr==parent_track_ || parent_track_->animator_target_ == t);

  animator_target_ = t;
  for(noz_uint32 i=0,c=child_tracks_.size(); i<c; i++) {
    child_tracks_[i]->SetAnimatorTarget(t);
  }
}

void AnimationViewTrack::SetAnimationTrack(AnimationTrack* t) {
  noz_assert(nullptr == animation_track_ || animation_track_->GetTrackId() == track_id_);
  animation_track_ = t;
}

