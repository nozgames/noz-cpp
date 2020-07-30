///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/ToggleButton.h>
#include <noz/Nodes/UI/TreeView.h>
#include <noz/Nodes/UI/TreeViewItem.h>
#include <noz/Nodes/UI/ListView.h>
#include <noz/Nodes/UI/Spacer.h>
#include <noz/Editor/Nodes/Render/GridNode.h>
#include <noz/Animation/EnumKeyFrame.h>
#include <noz/Animation/FloatKeyFrame.h>
#include <noz/Animation/SpriteKeyFrame.h>
#include <noz/Animation/EventKeyFrame.h>
#include "AnimationView.h"
#include <noz/Editor/Actions/AddAnimationTargetAction.h>
#include <noz/Editor/Actions/SetPropertyAction.h>
#include <noz/Editor/Actions/SetKeyFrameAction.h>
#include <noz/Editor/Actions/SetAnimatorTargetAction.h>
#include <noz/Editor/Actions/SetKeyFrameTangentsAction.h>
#include <noz/Editor/Actions/RemoveKeyFrameAction.h>

using namespace noz;
using namespace noz::Editor;

AnimationView::AnimationView(void) {
  animator_ = nullptr;
  animation_ = nullptr;
  zoom_ = 1.0f;
  drag_start_ = -1.0f;
  drag_delta_ = 0.0f;
  edit_event_key_ = nullptr;
  visible_track_count_ = 0;
  options_track_ = nullptr;

  if(!Application::IsInitializing()) {
    EditorApplication::PropertyChanged += PropertyChangedEventHandler::Delegate(this, &AnimationView::OnPropertyChanged);
  }
}

AnimationView::~AnimationView(void) {
}

void AnimationView::SetSource (Animator* animator) {
  if(source_ == animator) return;
  source_ = animator;
  CleanAnimator(source_);
  Refresh();    
}

bool AnimationView::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;

  if(animation_list_) animation_list_->SelectionChanged += 
    SelectionChangedEventHandler::Delegate(this, &AnimationView::OnAnimationListSelectionChanged);

  if(add_event_button_) add_event_button_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnAddEvent);
  if(add_frame_button_) add_frame_button_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnAddFrame);
  if(play_button_) play_button_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnPlayButtonClicked);
  if(record_button_) record_button_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnRecordButtonClicked);
  if(next_frame_) next_frame_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnNextFrame);
  if(prev_frame_) prev_frame_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnPrevFrame);
  if(first_frame_) first_frame_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnFirstFrame);
  if(last_frame_) last_frame_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnLastFrame);

  if(event_animation_state_) event_animation_state_->SelectionChanged += SelectionChangedEventHandler::Delegate(this, &AnimationView::OnEventStateChanged);
  if(event_method_) event_method_->SelectionChanged += SelectionChangedEventHandler::Delegate(this, &AnimationView::OnEventMethodChanged);

  if(link_track_menu_item_) link_track_menu_item_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnLinkTrackMenuItem);
  if(unlink_track_menu_item_) unlink_track_menu_item_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnUnlinkTrackMenuItem);
  if(delete_track_menu_item_) delete_track_menu_item_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnDeleteTrackMenuItem);
  if(delete_key_frame_menu_item_) delete_key_frame_menu_item_->Click += ClickEventHandler::Delegate(this, &AnimationView::OnDeleteKeyFrameMenuItem);

  // link the timeline to the view
  if(timeline_) {
    timeline_->ValueChanged += ValueChangedEventHandler::Delegate(this, &AnimationView::OnTimelineTimeChanged);
  }

  if(dope_sheet_) dope_sheet_->animation_view_ = this;

  if(dope_sheet_button_) {
    dope_sheet_button_->SetChecked(true);
    dope_sheet_button_->Click += ClickEventHandler::Delegate(this,&AnimationView::OnDopeSheetButton);
  }
  if(curve_editor_button_) {
    curve_editor_button_->SetChecked(false);
    curve_editor_button_->Click += ClickEventHandler::Delegate(this,&AnimationView::OnCurveEditorButton);
  }

  if(timeline_scrollbar_) {
    timeline_scrollbar_->Scroll += ScrollEventHandler::Delegate(this, &AnimationView::OnTimelineScroll);
  }

  Refresh();

  return true;
}

void AnimationView::OnPlayButtonClicked (UINode* sender) {
  UpdateAnimatorState ();
}

void AnimationView::OnRecordButtonClicked (UINode* sender) {
  // Stop playing when record is checked or unchecked.
  if(IsPlaying()) play_button_->SetChecked(false);

  UpdateAnimatorState ();
}

void AnimationView::BeginRecording (void) {
  if(!IsInteractive()) return;
  if(IsRecording()) return;
  if(record_button_) record_button_->SetChecked(true);
  UpdateAnimatorState();
}

void AnimationView::RefreshAnimationFrame (void) {
  animator_->Stop();
  UpdateAnimatorState();
}

void AnimationView::UpdateAnimatorState(void) {
  if(nullptr==animator_) return;
  if(play_button_->IsChecked() || record_button_->IsChecked()) {
    animator_->SetState("Manual");
    animator_->Advance(timeline_->GetTime());
  } else {
    animator_->Stop();
    animator_->Advance(0.0f);
  }
}

void AnimationView::UpdateDopeSheetVisibility(void) {
  if(dope_sheet_ == nullptr || curve_editor_ == nullptr) return;

  // Toggle the dop
  if(dope_sheet_button_->IsChecked()) {
    dope_sheet_->SetVisibility(Visibility::Visible);    
    curve_editor_->SetVisibility(Visibility::Hidden);

    // For each selected track make sure there is a selected key frame
    for(noz_uint32 i=selected_tracks_.size(); i>0; i--) {
      AnimationViewTrack* av_track = selected_tracks_[i-1];
      noz_assert(av_track);

      bool deselect = true;
      for(noz_uint32 ii=0,cc=selected_key_frames_.size(); ii<cc && deselect; ii++) {
        deselect = selected_key_frames_[ii].av_track_ != av_track;
      }

      if(deselect) SetTrackSelected(av_track, false);
    }

  } else {
    dope_sheet_->SetVisibility(Visibility::Hidden);
    curve_editor_->SetVisibility(Visibility::Visible);
    curve_editor_->UpdateTracks();
  }
}

void AnimationView::Refresh(void) {
  UpdateDopeSheetVisibility ();

  // If no source or source controller just clear the animation view.
  if(source_ == nullptr || source_->GetController() == nullptr) {    
    SetInteractive(false);
    if(record_button_) record_button_->SetChecked(false);
    if(play_button_) play_button_->SetChecked(false);
    if(source_name_) source_name_->SetText("");
    if(animation_list_) animation_list_->RemoveAllChildren();
    if(tracks_container_) tracks_container_->RemoveAllChildren();
    if(add_frame_button_) add_frame_button_->SetInteractive(false);
    animation_ = nullptr;
    UpdateZoom();
    return;
  } 
  
  SetInteractive(true);

  if(source_name_) {
    if(source_ && source_->GetNode()) {
      if(source_->GetNode()->GetName().IsEmpty()) {
        source_name_->SetText(String::Format("[%s]", source_->GetNode()->GetType()->GetName().ToCString()));
      } else {
        source_name_->SetText(source_->GetNode()->GetName());
      }
    } else {
      source_name_->SetText("");
    }
  } 

  if(animation_list_) {
    animation_list_->RemoveAllChildren();

    if(source_ && source_->GetController()) {
      std::set<Animation*> animations;
      for(noz_uint32 i=0,c=source_->GetController()->GetLayerCount(); i<c; i++) {
        AnimationLayer* layer = source_->GetController()->GetLayer(i);
        for(noz_uint32 ii=0,cc=layer->GetStateCount(); ii<cc; ii++) {
          if(layer->GetState(ii)->GetAnimation()) {
            animations.insert(layer->GetState(ii)->GetAnimation());
            CleanAnimation(layer->GetState(ii)->GetAnimation());
          }
        }
      }      

      if(animations.size()) {
        for(auto it=animations.begin(); it!=animations.end(); it++) {
          DropDownListItem* item = new DropDownListItem;
          item->SetText(AssetDatabase::GetFile((*it)->GetGuid())->GetName());
          item->SetUserData((*it));
          animation_list_->AddChild(item);
        }

        animation_list_->GetFirstChildItem()->Select();
      }
    }
  }
  RefreshAnimation();
}

void AnimationView::RefreshAnimation (void) {
  if(nullptr==tracks_container_) return;

  // Clear selection
  UnselectAllTracks();

  // If we have no source or selected animation we are done
  if (nullptr==source_ || nullptr==animation_list_->GetSelectedItem()) {
    animation_ = nullptr;
  } else {
    // Cache current animation
    animation_ = Cast<Animation>((Object*)animation_list_->GetSelectedItem()->GetUserData());
    noz_assert(animation_);

    // Clear the existing animator.
    if(animator_) {
      delete animator_;
      animator_ = nullptr;
    }

    // Create a new animator for the animation
    AnimationController* controller = new AnimationController;
    AnimationLayer* layer = new AnimationLayer;
    AnimationState* state = new AnimationState;
    state->SetName("Manual");
    state->SetAnimation(animation_);
    layer->AddState(state);
    controller->AddLayer(layer);

    animator_ = new Animator;
    for(noz_uint32 i=0,c=source_->GetTargetCount();i<c;i++) {
      animator_->AddTarget(source_->GetTarget(i)->GetGuid(), source_->GetTarget(i)->GetTarget());
    }    
  
    animator_->SetController(controller);  
    SetTime(0.0f, true);
  }

  if(timeline_) timeline_->SetTime(0.0f);

  RefreshTracks();
}

Animation* AnimationView::GetAnimation(void) const {
  if(nullptr==source_) return nullptr;
  if(nullptr==source_->GetController()) return nullptr;
  if(nullptr==animation_list_->GetSelectedItem()) return nullptr;
  Animation* animation = (Animation*)animation_list_->GetSelectedItem()->GetUserData();
  noz_assert(animation);
  return animation;
}

bool AnimationView::IsRecording (void) const {
  return record_button_ && record_button_->IsChecked();
}

bool AnimationView::IsPlaying (void) const {
  return play_button_ && play_button_->IsChecked();
}

void AnimationView::SetAnimationModified(void) {
  Animation* animation = GetAnimation();
  if(animation) modified_animations_.insert(animation);
}

void AnimationView::Save (void) {
  for(auto it=modified_animations_.begin(); it!=modified_animations_.end(); it++) {
    Animation* animation = *it;
    noz_assert(animation);

    AssetFile* file = AssetDatabase::GetFile(animation->GetGuid());
    if(nullptr == file) continue;

    FileStream fs;
    if(!fs.Open(file->GetPath(),FileMode::Truncate)) {
      Console::WriteError(animation, "failed to save animation");
      continue;
    }

    JsonSerializer().Serialize(animation,&fs);
    fs.Close(); 
  }

  modified_animations_.clear();  
}

#if 0
void AnimationView::Link (AnimationViewTargetItem* av_target) {
  noz_assert(av_target);
  noz_assert(av_target->GetAnimationTarget());
  
  if(nullptr == source_) return;
  if(nullptr == animation_) return;

  // Get the workspace the view is part of
  Workspace* workspace = Workspace::GetWorkspace(this);
  if(workspace == nullptr ) return;

  // Get the inspector for the workspace
  Inspector* inspector = workspace->GetInspector();
  if(nullptr == inspector) return;

  // Get the currently inspected object
  Object* o = inspector->GetTarget();
  if(nullptr == o) return;

  // Can we cast to the target type?
  if(!o->IsTypeOf(av_target->GetAnimationTarget()->GetTargetProperty()->GetParentType())) return;

  // Find the animator target that matches the view target track
  if(nullptr != av_target->GetAnimatorTarget()) {    
    // Already linked to this object?
    if(av_target->GetAnimatorTarget()->GetTarget() == o) return;

    // Remove old link
    source_->RemoveTarget(av_target->GetAnimatorTarget());
  }

  // Update the view target to reflect the new animator target
  av_target->animator_target_ = source_->AddTarget(av_target->GetAnimationTarget()->GetGuid(), o);
  av_target->Refresh();
}
#endif

void AnimationView::OnAnimationListSelectionChanged (UINode* sender) {
  RefreshAnimation();
}

void AnimationView::OnKeyDown(SystemEvent* e) {
  Control::OnKeyDown(e);
  
  switch(e->GetKeyCode()) {
    case Keys::Delete:
      RemoveSelectedKeyFrames ();
      break;
  }
}

void AnimationView::RemoveSelectedKeyFrames (void) {
  noz_assert(source_);

  // Nothing to do if no selected key frames.
  if(selected_key_frames_.empty()) return;

  EditorDocument* edoc = EditorDocument::GetActiveDocument(source_->GetNode());
  noz_assert(edoc);

  while(!selected_key_frames_.empty()) {
    SelectedKeyFrame& skf = selected_key_frames_.back();
    edoc->ExecuteAction(new RemoveKeyFrameAction(skf.av_track_->GetAnimationTrack(), skf.kf_index_), false);
  }    

  edoc->CommitUndoGroup();
}

AnimationView* AnimationView::GetAnimationView(Node* descendant) {
  Workspace* workspace = Workspace::GetWorkspace(descendant);
  if(nullptr == workspace) return nullptr;
  return workspace->GetAnimationView();
}

void AnimationView::RemoveTrack (AnimationTarget* animation_target) {
  AnimationViewTrack* av_track = FindTrack(animation_target);
  noz_assert(av_track);

  // Destroy all children
  for(noz_uint32 i=0,c=av_track->GetChildTrackCount(); i<c; i++) av_track->GetChildTrack(i)->Destroy();

  // Destroy the main track
  av_track->Destroy();

  UpdateTrackIndicies();
}

AnimationViewTrack* AnimationView::AddTrack(AnimationTarget* animation_target) {
  noz_assert(animation_target);
  noz_assert(animation_target->GetTargetProperty());

  // Find the matchign animator target 
  AnimatorTarget* animator_target = source_->GetTarget(animation_target->GetGuid());

  // If the target has multiple tracks...
  if(animation_target->GetTargetProperty()->GetAnimationTrackCount()>1) {
    // Add a header track that will contain the actual tracks.
    AnimationViewTrack* av_track = new AnimationViewTrack(animation_target, animator_target, 0xFFFFFFFF);
    tracks_container_->AddChild(av_track);

    // Add a track for each channel
    for(noz_uint32 i=0,c=animation_target->GetTargetProperty()->GetAnimationTrackCount(); i<c; i++) {
      AnimationTrack* animation_track = animation_target->GetTrackById(i);
      if(nullptr == animation_track) {
        animation_track = new AnimationTrack;
        animation_track->SetTrackId(i);
      }

      // Add the child track
      AnimationViewTrack* av_child_track = new AnimationViewTrack(animation_target, animator_target, animation_track);
      av_track->child_tracks_.push_back(av_child_track);
      av_child_track->parent_track_ = av_track;
      av_child_track->SetVisibility(Visibility::Collapsed);
      tracks_container_->AddChild(av_child_track);
    }

    return av_track;
  }
 
  AnimationTrack* animation_track = animation_target->GetTrackById(0);
  AnimationViewTrack* av_track = nullptr;
  if(nullptr == animation_track) {
    av_track = new AnimationViewTrack(animation_target, animator_target, (noz_uint32)0);
  } else {
    av_track = new AnimationViewTrack(animation_target, animator_target, animation_track);
    AddKeyFrames(av_track);
  }

  tracks_container_->AddChild(av_track);    
  return av_track;
}

AnimationViewTrack* AnimationView::FindTrack(AnimationTarget* animation_target) {
  for(noz_uint32 i=0,c=GetTrackCount(); i<c; i++) {
    AnimationViewTrack* av_track = GetTrack(i);
    if(av_track && av_track->GetAnimationTarget() == animation_target) return av_track;
  }
  return nullptr;
}

AnimationViewTrack* AnimationView::FindTrack(AnimatorTarget* animator_target) {
  for(noz_uint32 i=0,c=GetTrackCount(); i<c; i++) {
    AnimationViewTrack* av_track = GetTrack(i);
    noz_assert(av_track);
    if(av_track->GetParentTrack()) continue;
    if(av_track->GetAnimatorTarget() == animator_target) return av_track;
  }
  return nullptr;
}

AnimationViewTrack* AnimationView::FindTrack(AnimationTarget* animation_target, noz_uint32 track_id) {
  for(noz_uint32 i=0,c=GetTrackCount(); i<c; i++) {
    AnimationViewTrack* av_track = GetTrack(i);
    noz_assert(av_track);
    if(av_track->GetAnimationTarget() != animation_target) continue;
    if(av_track->GetTrackId() != track_id) continue;
    return av_track;
  }
  return nullptr;
}

void AnimationView::AddKeyFrames (AnimationViewTrack* av_track) {
  AnimationTrack* animation_track = av_track->GetAnimationTrack();
  if(nullptr == animation_track) return;

  // Add keys to track and dope sheet
  for (noz_uint32 i=0,c=animation_track->GetKeyFrameCount(); i<c; i++) {
    //AddKeyFrame(av_track,animation_track->GetKeyFrame(i));
  }
}

#if 0
AnimationViewItem* AnimationView::AddEventTrack (AnimationTrack* animation_track) {
  noz_assert(animation_track);

  AnimationViewItem* av_track = new AnimationViewItem(nullptr, animation_track);
  av_track->SetText("Events");

//  AddKeys(av_track);

  // Add the track to the tracks container
  tracks_container_->InsertChild(0, av_track);

  return av_track;
}
#endif

void AnimationView::CleanAnimation (Animation* animation) {
  // Remove all tracks that have no keys and targets that have no tracks
  for(noz_uint32 i=animation->GetTargetCount(); i>0; i--) {
    AnimationTarget* animation_target = animation->GetTarget(i-1);
    noz_assert(animation_target);

    for(noz_uint32 ii=animation_target->GetTrackCount(); ii>0; ii--) {
      AnimationTrack* animation_track = animation_target->GetTrack(ii-1);
      noz_assert(animation_track);

      if(animation_track->GetKeyFrameCount() == 0) {
        animation_target->RemoveTrack(animation_track);
      }
    }

    if(animation_target->GetTrackCount() == 0) {
      animation->RemoveTarget(animation_target);
    }
  }
}

void AnimationView::CleanAnimator (Animator* animator) {
  std::set<Guid> all_targets;

  if(nullptr == animator) return;
  if(nullptr == animator->GetController()) return;

  for(noz_uint32 i=0,c=animator->GetController()->GetLayerCount(); i<c; i++) {
    AnimationLayer* layer = animator->GetController()->GetLayer(i);
    for(noz_uint32 ii=0,cc=layer->GetStateCount(); ii<cc; ii++) {
      Animation* animation = layer->GetState(ii)->GetAnimation();
      if(animation == nullptr) continue;
      for(noz_uint32 iii=0,ccc=animation->GetTargetCount(); iii<ccc; iii++) {
        all_targets.insert(animation->GetTarget(iii)->GetGuid());
      }
    }
  }    

  // Remove all animator targets that are not reresented in the animation set
  for(noz_uint32 i=animator->GetTargetCount(); i>0; i--) {
    if(animator->GetTarget(i-1)->GetTarget()==nullptr) {
      animator->RemoveTarget(i-1);
      continue;
    }

    auto it = all_targets.find(animator->GetTarget(i-1)->GetGuid());
    if(it == all_targets.end()) {
      animator->RemoveTarget(i-1);
    }
  }
}

void AnimationView::Animate(void) {
  if(nullptr==animator_) return;
  if(nullptr==play_button_ || !play_button_->IsChecked()) return;
  
  noz_float new_time = timeline_->GetTime() + Time::GetDeltaTime();
  if(animation_ && new_time > animation_->GetDuration()) new_time = animation_->GetDuration();
  SetTime(new_time);
}

void AnimationView::OnMouseWheel(SystemEvent* e) {
  Control::OnMouseWheel(e);

  // If the mouse if over the node then zoom
  if(e->GetDelta().y > 0.0f) {
    SetZoom(zoom_ * 1.1f);
  } else {
    SetZoom(zoom_ * (1/1.1f));
  }
}

void AnimationView::SetZoom (noz_float zoom) {
  if(nullptr == dope_sheet_) return;

  zoom = Math::Clamp(zoom,0.25f,10.0f);
  if(zoom == zoom_) return;
  zoom_ = zoom;

  UpdateZoom();
}

void AnimationView::UpdateZoom(void) {
  if(timeline_) timeline_->SetZoom(zoom_);
  
  if(curve_editor_) {
    curve_editor_->SetZoom(zoom_);
    curve_editor_->SetGridSpacing(timeline_->GetLabelInterval() * timeline_->GetPixelsPerSecond());
  }
}

void AnimationView::SetLayoutTransformWidth (Node* n, noz_float w) {
  noz_assert(n);
  LayoutTransform* t = Cast<LayoutTransform>(n->GetTransform()); 
  if(nullptr==t) return;
  t->SetWidth(w);
}

void AnimationView::SetLayoutTransformHeight (Node* n, noz_float h) {
  noz_assert(n);
  LayoutTransform* t = Cast<LayoutTransform>(n->GetTransform()); 
  if(nullptr==t) return;
  t->SetHeight(h);
}

void AnimationView::SetTime (noz_float time, bool force) {
  if(!force && time==GetTime()) return;

  // Do not allow time to go below zero
  time = Math::Max(time,0.0f);
  
  if(force) {
    // Stop playing...
    if(play_button_ && play_button_->IsChecked()) play_button_->SetChecked(false);
  }

  bool playing = 
    (play_button_ && play_button_->IsChecked()) ||
    (record_button_ && record_button_->IsChecked());

  if(playing && animator_) {
    noz_assert(animation_);
    if(time < GetTime()) {
      animator_->Stop();
      animator_->SetState("Manual");
      animator_->Advance(time);
    } else {
      animator_->Advance(time-GetTime());
    }
  }
  
  timeline_->SetTime(time);
}

void AnimationView::OnMouseDown (SystemEvent* e, AnimationViewDopeSheet* dope_sheet) {
  noz_assert(e);
  noz_assert(dope_sheet);
  noz_assert(dope_sheet==dope_sheet_);

  e->SetHandled();
  SetFocus();

//  UnselectAllKeys();
}

void AnimationView::OnFirstFrame (UINode*) {  
  SetTime(0.0f,true);
}

void AnimationView::OnLastFrame (UINode*) {  
  if(nullptr==animation_) return;
  SetTime(animation_->GetDuration(),true);
}

void AnimationView::OnPrevFrame(UINode*) {
  SetTime(GetTime()-timeline_->snap_time_,true);
}

void AnimationView::OnNextFrame(UINode*) {
  if(nullptr==animation_) return;
  SetTime(Math::Min(animation_->GetDuration(), GetTime()+timeline_->snap_time_),true);
}

void AnimationView::OnAddEvent(UINode*) {
#if 0
  if(nullptr==animation_) return;
  EventKeyFrame* kf = new EventKeyFrame;
  kf->SetTime(time_);
  bool had_event_track = animation_->GetEventTrack() != nullptr;
  animation_->AddEvent(kf);
  AnimationViewItem* av_track = nullptr;
  if(!had_event_track) {
    av_track = AddEventTrack(animation_->GetEventTrack());
  } else {
    av_track = Cast<AnimationViewItem>(tracks_container_->GetLogicalChild(0));
    noz_assert(av_track);
    noz_assert(av_track->IsEventTrack());
  }

  /*
  AnimationViewKeyFrame* av_key = new AnimationViewKeyFrame(av_track, kf);
  AddKey(av_key);
  SetAnimationModified();
  UpdateZoom();
  UnselectAllKeys();
  SelectKey(av_key);

  // Automatically open the event popup for the new key
  BeginEditEvent(av_key);
  */
#endif
}

void AnimationView::OnAddFrame (UINode*) {
  // TODO: add a frame.
}

void AnimationView::OnEventStateChanged(UINode*) {
/*
  if(nullptr == edit_event_key_ ) return;

  // Get the event key frame
  EventKeyFrame* kf = Cast<EventKeyFrame>(edit_event_key_->GetKeyFrame());
  if(nullptr == kf) return;

  if(event_animation_state_->GetSelectedItem()==nullptr || event_animation_state_->GetSelectedItemIndex()==0) {
    kf->SetAnimationState(Name::Empty);
  } else {
    kf->SetAnimationState(event_animation_state_->GetSelectedItem()->GetText());
  }
*/
}

void AnimationView::OnEventMethodChanged(UINode*) {
/*
  if(nullptr == edit_event_key_ ) return;

  // Get the event key frame
  EventKeyFrame* kf = Cast<EventKeyFrame>(edit_event_key_->GetKeyFrame());
  if(nullptr == kf) return;

  if(event_method_->GetSelectedItem()==nullptr || event_method_->GetSelectedItemIndex()==0) {
    kf->SetMethod(nullptr);
  } else {
    kf->SetMethod((Method*)event_method_->GetSelectedItem()->GetUserData());
  }
*/
}

void AnimationView::GetMethods (Object* o, std::set<Method*>& methods) {
  for(Type* t = o->GetType(); t; t=t->GetBase()) {
    for(noz_uint32 i=0,c=t->GetMethods().size(); i<c; i++) {
      methods.insert(t->GetMethods()[i]);
    }
  }

  if(o->IsTypeOf(typeof(Node))) {
    Node* n = (Node*)o;
    for(noz_uint32 i=0,c=n->GetComponentCount(); i<c; i++) {
      GetMethods(n->GetComponent(i), methods);
    }
  }
}

void AnimationView::BeginEditEvent (AnimationViewKeyFrame* av_key) {
/*
  noz_assert(av_key);

  if(nullptr==edit_event_popup_) return;

  // Get the event key frame
  EventKeyFrame* kf = Cast<EventKeyFrame>(av_key->GetKeyFrame());
  if(nullptr == kf) return;

  edit_event_key_ = nullptr;

  // Populate the methods drop down.
  Node* n = source_->GetNode();
  if(n) {
    event_method_->RemoveAllChildren();

    // Special case for no method.
    DropDownListItem* none = new DropDownListItem;
    none->SetText("<none>");
    event_method_->AddChild(none);
    none->Select();

    std::set<Method*> methods;
    GetMethods(n,methods);

    for(auto it=methods.begin(); it!=methods.end(); it++) {
      DropDownListItem* item = new DropDownListItem;
      item->SetText(String::Format("%s::%s ()", (*it)->GetParentType()->GetName().ToCString(), (*it)->GetName().ToCString()));
      item->SetUserData(*it);
      event_method_->AddChild(item);

      if(kf->GetMethod() == *it) item->Select();
    }
  }

  // Popuplate the animation state dropdown
  AnimationController* controller = source_->GetController();
  if(controller) {
    event_animation_state_->RemoveAllChildren();

    // Special case for no state.
    DropDownListItem* none = new DropDownListItem;
    none->SetText("<none>");
    event_animation_state_->AddChild(none);
    none->Select();

    std::set<Name> states;
    for(noz_uint32 i=0,c=controller->GetLayerCount(); i<c; i++) {
      for(noz_uint32 ii=0,cc=controller->GetLayer(i)->GetStateCount(); ii<cc; ii++) {
        AnimationState* state = controller->GetLayer(i)->GetState(ii);
        noz_assert(state);
        states.insert(state->GetName());
      }
    }

    for(auto it=states.begin(); it!=states.end(); it++) {
      DropDownListItem* item = new DropDownListItem;
      item->SetText(*it);
      event_animation_state_->AddChild(item);
      if(kf->GetAnimationState()==*it) item->Select();
    }
  }

  // Initialize event sound
  event_audio_clip_->SetTarget(kf, kf->GetProperty("AudioClip"));

  edit_event_key_ = av_key;

  edit_event_popup_->SetPlacementTarget(av_key);
  edit_event_popup_->Open();
*/
}

void AnimationView::RefreshTracks(void) {
  // Wipe out any existing tracks, keys, and selection
  tracks_container_->RemoveAllChildren();
  dope_sheet_->RemoveAllChildren();
  selected_key_frames_.clear();

  if(curve_editor_) curve_editor_->UpdateTracks();

  // If there is not selected animation then leave the view empty
  if(nullptr == animation_) {
    UpdateZoom();
    return;
  }

  // Add all of the targets
  for (noz_uint32 i=0, c=animation_->GetTargetCount(); i<c; i++) {
    AddTrack (animation_->GetTarget(i));
  }

  UpdateTrackIndicies();
  UpdateZoom();

/*
// Add all of the targets.
for(noz_uint32 i=0,c=animation_->GetTargetCount(); i<c; i++) {
AnimationTarget* animation_target = animation_->GetTarget(i);
noz_assert(animation_target);
AddTarget (animation_target);
}

if(animation_->GetEventTrack()) {
AddEventTrack (animation_->GetEventTrack());
}

// Add all of the tracks..
for(noz_uint32 i=0,c=animation_->GetTrackCount(); i<c; i++) {
AnimationTrack* animation_track = animation_->GetTrack(i);
noz_assert(animation_track);
AddTrack (animation_track);
}

UpdateZoom();
*/

}

void AnimationView::UpdateTrackIndicies(void) {
  noz_int32 render_index = 0;
  for(noz_uint32 i=0,c=GetTrackCount(); i<c; i++) {
    AnimationViewTrack* av_track = GetTrack(i);
    noz_assert(av_track);

    if(av_track->GetVisibility() != Visibility::Visible) {
      av_track->render_index_ = -1;
      continue;      
    }

    av_track->render_index_ = render_index++;
  }

  visible_track_count_ = render_index;

  if(dope_sheet_) dope_sheet_->InvalidateRender ();
}

void AnimationView::OnNamedPropertyChanged (Node* n) {
  noz_assert(n);
  noz_assert(animation_);
  noz_assert(source_);

  // Scan the animation looking for targets that match the node
  for(noz_uint32 i=0,c=animation_->GetTargetCount(); i<c; i++) {
    AnimationTarget* animation_target = animation_->GetTarget(i);
    noz_assert(animation_target);

    AnimatorTarget* animator_target = source_->GetTarget(animation_target->GetGuid());    
    if(nullptr == animator_target) continue;

    // The animator target can be a node or a component.  If the target is a component
    // then we need to look for a comparison of its parent node as well.
    Object* t = n;
    if(animator_target->GetTarget()->IsTypeOf(typeof(Component))) {
      t = ((Component*)animator_target->GetTarget())->GetNode();
    }

    if(n == t) {
      AnimationViewTrack* av_track = FindTrack(animation_target);
      noz_assert(av_track);
      av_track->Refresh();

      animation_target->SetName(av_track->GetText());
    }
  }
}

void AnimationView::OnPropertyChanged(PropertyChangedEventArgs* args) {
  noz_assert(args);
  noz_assert(args->GetTarget());
  noz_assert(args->GetProperty());

  // Need a current animation..
  if(nullptr==animation_) return;

  // If a nodes name is being changed then check to see if any targets need to be renamed.
  if(args->GetProperty() == typeof(Node)->GetProperty("Name")) {
    noz_assert(args->GetTarget());
    noz_assert(args->GetTarget()->IsTypeOf(typeof(Node)));
    OnNamedPropertyChanged((Node*)args->GetTarget());
    return;
  }
}




#if 0
KeyFrame* AnimationView::GetKeyFrame (Object* t, Property* p, noz_uint32 track_id, noz_float time, Type* create_type) {
  // Get the active animation.
  Animation* animation = GetAnimation();
  if(nullptr == animation) return nullptr;
  
  // Node...
  Node* node = nullptr;
  if(t->IsTypeOf(typeof(Node))) {
    node = (Node*)t;
  // Component..
  } else if (t->IsTypeOf(typeof(Component))) {
    node = ((Component*)t)->GetNode();
  }

  // Node must be a descendant of the source
  if(nullptr == node || (node != source_->GetNode() && !node->IsDescendantOf(source_->GetNode()))) return nullptr;

  // Retrieve the animator target that matches target object
  AnimatorTarget* animator_target = source_->GetTarget(animation, t);
  AnimationTarget* animation_target = nullptr;

  // No existing target?
  if(nullptr == animator_target) {
    // If no create type was given we cannot create the track for the frame
    if(nullptr == create_type) return nullptr;
  
    // Add a new animation target for the property
    animation_target = animation->AddTarget (p);

    // Add a new animator target to the source animator
    animator_target = source_->AddTarget(animation_target ->GetGuid(),t);
    if(animator_) animator_->AddTarget(animation_target ->GetGuid(),t);

    // Add a target track to the UI
    AddTargetItem (animation_target);

    SetAnimationModified();
  } else {
    // Find the matching animation target.
    animation_target = animation->GetTarget(animator_target->GetGuid());
    noz_assert(animation_target);
  }

  // Find the animation track that matches the given property
  AnimationTrack* animation_track = animation_target->GetTrackById(track_id)
  AnimationViewItem* av_track = nullptr;
  if(nullptr == animation_track) {
    // Create a new animation track and add to animation
    animation_track = new AnimationTrack;
    animation_track->SetTarget(animation_target);
    animation_track->SetTargetProperty(p->GetName());
    animation_track->SetTargetComponentType(component?component->GetType() : nullptr);
    animation->AddTrack(animation_track);
    av_track = AddTrack (animation_track);
  } else {
    av_track = GetTrack (animation_track);
    noz_assert(av_track);
  }

  for(noz_uint32 i=0,c=animation_track->GetKeyFrameCount(); i<c; i++) {
    KeyFrame* kf = animation_track->GetKeyFrame(i);
    noz_assert(kf);

    // Matching key frame?
    if(kf->GetTime() == time) return kf;
  }

  if(create_type) {
    // Create the new key frame
    KeyFrame* kf = create_type->CreateInstance<KeyFrame>();
    if(nullptr == kf) return nullptr;
    kf->SetTime(time);
    animation_track->AddKeyFrame(kf);
       
//    AnimationViewKeyFrame* av_key = new AnimationViewKeyFrame(av_track, kf);
//    AddKey(av_key);

    SetAnimationModified();
    UpdateZoom();
    return kf;
  }

  return nullptr;
}
#endif

void AnimationView::OnMouseDown (SystemEvent* e, AnimationViewTrack* av_track) {
  noz_assert(e);
  noz_assert(av_track);

  if(av_track->GetAnimatorTarget() && av_track->GetAnimatorTarget()->GetTarget()) {
    Node* node = Cast<Node>(av_track->GetAnimatorTarget()->GetTarget());
    if(nullptr == node) {
      Component* component = Cast<Component>(av_track->GetAnimatorTarget()->GetTarget());
      if(component) {
        node = component->GetNode();
      }
    }

    if(node) {
      // Get the workspace the view is part of
      Workspace* workspace = Workspace::GetWorkspace(this);
      if(workspace && workspace->GetHierarchy()) {
        workspace->GetHierarchy()->SetSelected(node);
      }
    }
  }

  // Prevent further processing.
  e->SetHandled();

  // Set focus to hte animation view
  SetFocus();

  // If shift isnt pressed then clear the selection.
  if(!e->IsShift()) UnselectAllTracks();

  // Select the track clicked on.
  SelectTrack (av_track);

  // Update the tracks in the curve eidtor..
  if(curve_editor_) curve_editor_->UpdateTracks ();
}

void AnimationView::OnOptionsButton (AnimationViewTrack* av_track) {
  if(nullptr == track_options_popup_) return;

  if(unlink_track_menu_item_) unlink_track_menu_item_->SetInteractive(av_track->GetAnimatorTarget()!=nullptr);
  
  options_track_ = av_track;
  track_options_popup_->SetPlacementTarget(av_track->options_button_);
  track_options_popup_->Open();
}

void AnimationView::OnLinkTrackMenuItem(UINode*) {
  track_options_popup_->Close();

  noz_assert(source_);
  noz_assert(animation_);
  noz_assert(options_track_);

  AnimationTarget* animation_target = options_track_->GetAnimationTarget();
  noz_assert(animation_target);

  // Get the workspace the view is part of
  Workspace* workspace = Workspace::GetWorkspace(this);
  if(workspace == nullptr ) return;

  // Get the inspector for the workspace
  Inspector* inspector = workspace->GetInspector();
  if(nullptr == inspector) return;

  // Get the currently inspected object
  Object* o = inspector->GetTarget();
  if(nullptr == o) return;

  // Can we cast to the target type?
  Type* target_type = animation_target->GetTargetProperty()->GetParentType();
  if(!o->IsTypeOf(target_type)) {
    // Is the property a component property?
    if(o->IsTypeOf(typeof(Node)) && target_type->IsCastableTo(typeof(Component))) {
      o = ((Node*)o)->GetComponent(target_type);
      if(nullptr == o) return;
    } else {
      return;
    }
  }

  // Already linked to this object?
  if(nullptr != options_track_->GetAnimatorTarget() && 
     options_track_->GetAnimatorTarget()->GetTarget() == o) {
    return;
  }

  EditorDocument* edoc = EditorDocument::GetActiveDocument(source_->GetNode());
  noz_assert(edoc);

  edoc->ExecuteAction(new SetAnimatorTargetAction(source_, animation_target->GetGuid(), o));

  RefreshAnimationFrame ();
}

void AnimationView::OnUnlinkTrackMenuItem(UINode*) {
  track_options_popup_->Close();

  noz_assert(source_);
  noz_assert(animator_);
  noz_assert(options_track_->GetAnimatorTarget());

  source_->RemoveTarget(source_->GetTarget(options_track_->GetAnimatorTarget()->GetGuid()));
  animator_->RemoveTarget(animator_->GetTarget(options_track_->GetAnimatorTarget()->GetGuid()));

  options_track_->SetAnimatorTarget(nullptr);
  options_track_->Refresh();
  options_track_->UpdateAnimationState();

  for(noz_uint32 i=0,c=options_track_->GetLogicalChildCount(); i<c; i++) {
    AnimationViewTrack* av_child_track = (AnimationViewTrack*)options_track_->GetLogicalChild(i);
    av_child_track->SetAnimatorTarget(nullptr);
    av_child_track->Refresh();
    av_child_track->UpdateAnimationState();
  }

  RefreshAnimationFrame ();
}

void AnimationView::OnDeleteTrackMenuItem(UINode*) {
  track_options_popup_->Close();
}

bool AnimationView::ExecuteAction (Action* action) {
  // Do not execute the action if not recording..
  if(!IsRecording()) return false;

  // We only care about the property set actions of animatable properties
  if(!action->IsTypeOf(typeof(SetColorPropertyAction)) &&
     !action->IsTypeOf(typeof(SetVector2PropertyAction)) &&
     !action->IsTypeOf(typeof(SetFloatPropertyAction)) &&
     !action->IsTypeOf(typeof(SetEnumPropertyAction)) && 
     !action->IsTypeOf(typeof(SetObjectPtrPropertyAction))   ) {
    return false;
  }
 
  // For ObjectPtr properties we only care about those which are setting a sprite..
  if(action->IsTypeOf(typeof(SetObjectPtrPropertyAction))) {
    Type* t = ((ObjectPtrProperty*)((SetObjectPtrPropertyAction*)action)->GetTargetProperty())->GetObjectType();
    if(!t->IsCastableTo(typeof(Sprite))) {
      return false;
    }
  }

  SetPropertyAction* a = (SetPropertyAction*)action;

  // Ensure the property type is animatable.  
  if(a->GetTargetProperty()->GetAnimationTrackCount()==0) return false;

  // Ensure the target is a node or a component of a node..
  Node* node = nullptr;
  if(a->GetTarget()->IsTypeOf(typeof(Node))) {
    node = (Node*)a->GetTarget();
  } else if(a->GetTarget()->IsTypeOf(typeof(Component))) {
    node = ((Component*)a->GetTarget())->GetNode();
  } else {
    return false;
  }

  // Node must be a descendant of the source
  if((node != source_->GetNode() && !node->IsDescendantOf(source_->GetNode()))) return false;

  // Get editor document the target is associated with.
  EditorDocument* edoc = EditorDocument::GetActiveDocument(a->GetTarget());
  if(nullptr == edoc) {
    return false;
  }

  // Fiend the animator target that matches target object
  AnimatorTarget* animator_target = nullptr;
  AnimationTarget* animation_target = nullptr;

  for(noz_uint32 i=0,c=source_->GetTargetCount(); nullptr == animation_target && i<c; i++) {
    animator_target  = source_->GetTarget(i);    
    animation_target = animation_->GetTarget(animator_target->GetGuid());

    if(animation_target==nullptr) {
      animator_target = nullptr;
      continue;
    }

    if(animation_target->GetTargetProperty() != a->GetTargetProperty()) {
      animation_target = nullptr;
      animator_target = nullptr;
      continue;
    }
  }

     
  // No existing animator target?
  if(nullptr == animator_target) {
    // Add a new animation target for the property
    animation_target = new AnimationTarget(a->GetTargetProperty(), Guid::Generate());

    // Execute action to add the target to the animation.
    edoc->ExecuteAction(new AddAnimationTargetAction(animation_, animation_target),false);

    SetAnimatorTargetAction* sata = new SetAnimatorTargetAction(source_, animation_target->GetGuid(), a->GetTarget());
    animator_target = sata->GetAnimatorTarget();
    edoc->ExecuteAction(sata,false);

    // Execute action to add the target to the animator
    if(animator_) animator_->AddTarget(animation_target ->GetGuid(),a->GetTarget());
  } else {
    // Find the matching animation target.
    animation_target = animation_->GetTarget(animator_target->GetGuid());
    noz_assert(animation_target);
  }

  // Multi-track property?
  if(a->GetTargetProperty()->GetAnimationTrackCount()>1) {
    GroupAction* ga = new GroupAction;
    for(noz_uint32 i=0,c=a->GetTargetProperty()->GetAnimationTrackCount(); i<c; i++) {
      if(a->IsTypeOf(typeof(SetColorPropertyAction))) {
        noz_float v = ((SetColorPropertyAction*)a)->GetValue().ToVector4()[i];
        ga->AddAction(new SetFloatKeyFrameAction(animation_target, i, GetTime(), v));
      } else if (a->IsTypeOf(typeof(SetVector2PropertyAction))) {
        noz_float v = ((SetVector2PropertyAction*)a)->GetValue()[i];
        ga->AddAction(new SetFloatKeyFrameAction(animation_target, i, GetTime(), v));
      }
    }
    edoc->ExecuteAction(ga,false);
  } else if(a->IsTypeOf(typeof(SetEnumPropertyAction))) {
    edoc->ExecuteAction(new SetEnumKeyFrameAction(animation_target, 0, GetTime(), ((SetEnumPropertyAction*)a)->GetValue()),false);
  } else if(a->IsTypeOf(typeof(SetFloatPropertyAction))) {
    edoc->ExecuteAction(new SetFloatKeyFrameAction(animation_target, 0, GetTime(), ((SetFloatPropertyAction*)a)->GetValue()),false);
  }

  delete action;


  /*
  AnimationViewTrack* av_track = FindTrack(animation_target);
  if(nullptr == av_track) {
    av_track = AddTrack(animation_target);
  }

  // Multi-track property?
  if(a->GetTargetProperty()->GetAnimationTrackCount()>1) {
    noz_assert(av_track->GetLogicalChildCount() == a->GetTargetProperty()->GetAnimationTrackCount());

    GroupAction* ga = new GroupAction;
    for(noz_uint32 i=0,c=a->GetTargetProperty()->GetAnimationTrackCount(); i<c; i++) {
      // Get the existing track..
      AnimationViewTrack* av_child_track = Cast<AnimationViewTrack>(av_track->GetLogicalChild(i));
      AnimationTrack* animation_track = av_child_track->GetAnimationTrack();
      noz_assert(animation_track);

      // Add the track to the target now that it is going to have a key frame.
      if(animation_track->GetTarget() == nullptr) {
        animation_target->AddTrack(animation_track);
      }

      // Is there an existing key from in this track at the current time?
      KeyFrame* kf = animation_track->GetKeyFrameByTime(GetTime());

      if(a->IsTypeOf(typeof(SetColorPropertyAction))) {
        noz_float v = ((SetColorPropertyAction*)a)->GetValue().ToVector4()[i];
        if(nullptr == kf) {
          kf = new FloatKeyFrame(GetTime(), v);
          animation_track->AddKeyFrame(kf);
          //AddKeyFrame(av_child_track,kf);         
        } else {
          noz_assert(kf->IsTypeOf(typeof(FloatKeyFrame)));
          ((FloatKeyFrame*)kf)->SetValue(v);
        }
        continue;
      }
    }
  } else {
    AnimationTrack* animation_track = av_track->GetAnimationTrack();
    noz_assert(animation_track);

    // Add the track to the target now that it is going to have a key frame.
    if(animation_track->GetTarget() == nullptr) {
      animation_target->AddTrack(animation_track);
    }

    // Is there an existing key from in this track at the current time?
    KeyFrame* kf = animation_track->GetKeyFrameByTime(GetTime());

    if(a->IsTypeOf(typeof(SetObjectPtrPropertyAction))) {
      Object* o = ((SetObjectPtrPropertyAction*)a)->GetValue();
      Type* ot = ((ObjectPtrProperty*)((SetObjectPtrPropertyAction*)action)->GetTargetProperty())->GetObjectType();
        
      if(ot->IsCastableTo(typeof(Sprite))) {
        if(nullptr == kf) {
          kf = new SpriteKeyFrame(GetTime(), (Sprite*)o);
          animation_track->AddKeyFrame(kf);
          //AddKeyFrame(av_track,kf);         
        } else {
          noz_assert(kf->IsTypeOf(typeof(SpriteKeyFrame)));
          ((SpriteKeyFrame*)kf)->SetValue((Sprite*)o);
        }
      }
    
    } else if (a->IsTypeOf(typeof(SetEnumPropertyAction))) {
      const Name& v = ((SetEnumPropertyAction*)a)->GetValue();

      if(nullptr == kf) {
        kf = new EnumKeyFrame(GetTime(), v);
        animation_track->AddKeyFrame(kf);
//        AddKeyFrame(av_track,kf);         
      } else {
        noz_assert(kf->IsTypeOf(typeof(EnumKeyFrame)));
        ((EnumKeyFrame*)kf)->SetValue(v);
      }
    }

  }

  SetAnimationModified();
  RefreshAnimationFrame();
  
  delete action;
    */

#if 0

  // Create an action to create the animation target if it does not exist
  AddAnimationTargetAction* aat_action = nullptr;
  if(nullptr == animation_target) {
    animation_target = new AnimationTarget (a->GetTargetProperty(), Guid::Generate());
    aat_action = new AddAnimationTargetAction(animation_, animation_target);
  }

  // Create an action to set the animator target if not yet set
  SetAnimatorTargetAction* sat_action = nullptr;
  if(animator_target == nullptr) {
    sat_action = new SetAnimatorTargetAction(source_, animation_target->GetGuid(), a->GetTarget());    
  }

  Action* sp_action = nullptr;
  if(action->IsTypeOf(typeof(SetFloatPropertyAction))) {
  }

  if(action->IsTypeOf(typeof(SetColorPropertyAction))) {
    sp_action = new SetColorKeyFrameAction(animation_target, 
  }

  return action;
#endif

  return true;
}

void AnimationView::OnDopeSheetButton (UINode*) {
  curve_editor_button_->SetChecked(!dope_sheet_button_->IsChecked());
  UpdateDopeSheetVisibility();
}

void AnimationView::OnCurveEditorButton (UINode*) {
  dope_sheet_button_->SetChecked(!curve_editor_button_->IsChecked());
  UpdateDopeSheetVisibility();
}

void AnimationView::OnTimelineScroll (UINode*) {
  if(nullptr == timeline_) return;

  timeline_->SetTimeOffset(timeline_scrollbar_->GetValue());
  if(curve_editor_) curve_editor_->SetTimeOffset(timeline_scrollbar_->GetValue());
  UpdateTimelineScrollBar();
}

void AnimationView::OnTimelineTimeChanged(UINode*) {
  if(nullptr == timeline_scrollbar_) return;
  if(curve_editor_) curve_editor_->SetTime(timeline_->GetTime());
  UpdateTimelineScrollBar();
  RefreshAnimationFrame();
}

void AnimationView::UpdateTimelineScrollBar (void) {
  if(nullptr == timeline_scrollbar_) return;
  if(Application::GetCapture() && Application::GetCapture()->IsDescendantOf(timeline_scrollbar_)) {
    return;
  }

  // If no animation then disable the scrollbar.
  if(nullptr == timeline_ || nullptr == animation_) {
    timeline_scrollbar_->SetInteractive(false);
    return;
  }  

  noz_float time_max = Math::Max(animation_->GetDuration(),timeline_->GetTime());
  noz_float time_vis = (timeline_->GetRectangle().w-timeline_->GetMargin()*2.0f) / timeline_->GetPixelsPerSecond();

  if(time_max < time_vis) {
    timeline_scrollbar_->SetInteractive(false);
  } else {
    timeline_scrollbar_->SetInteractive(true);
    timeline_scrollbar_->SetRange(0,Math::Max(timeline_scrollbar_->GetValue(),time_max-time_vis));
    timeline_scrollbar_->SetViewportSize(time_vis);
  }
}

void AnimationView::ArrangeChildren (const Rect& r) {
  Control::ArrangeChildren(r);

  UpdateTimelineScrollBar();
}

void AnimationView::OnTrackExpanded (AnimationViewTrack* av_track) {
  UpdateTrackIndicies();
/*
  // If the track was selected and it was collapsed then unselect the track
  // and reselect it to force all sub tracks to be selected
  if(av_track->IsSelected() && !av_track->IsExpanded()) {
    UnselectTrack(av_track);
    SelectTrack(av_track);
  }
*/
}

void AnimationView::UnselectAllTracks (void) {
  while(!selected_tracks_.empty()) {
    AnimationViewTrack* av_track = selected_tracks_.back();
    noz_assert(av_track);
    noz_assert(av_track->IsSelected());

    SetTrackSelected(av_track, false);

    // If track is being unselected and dope sheet is active then also unselect all of the keys
    for(noz_uint32 i=0,c=av_track->GetKeyFrameCount(); i<c; i++) {
      UnselectKeyFrame(av_track,i);
    }
  }

  selected_tracks_.clear();
}

void AnimationView::UnselectTrack (AnimationViewTrack* av_track) {
  noz_assert(av_track);

  if(!av_track->IsSelected()) return;


}

void AnimationView::SelectTrack (AnimationViewTrack* av_track) {
  noz_assert(av_track);
  
  // Ignore call if already selected..
  if(av_track->IsSelected()) return;

  // Special case parent tracks
  if(av_track->IsParentTrack()) {
    for(noz_uint32 i=0,c=av_track->GetChildTrackCount(); i<c; i++) {
      AnimationViewTrack* av_child_track = av_track->GetChildTrack(i);
      noz_assert(av_child_track);
      SelectTrack(av_child_track);     
    }

    return;
  }

  SetTrackSelected(av_track, true);

  // Inform the dope sheet that the track was selected.
  if(IsDopeSheetActive()) dope_sheet_->OnTrackSelected(av_track);
}

void AnimationView::SelectKeyFrame (AnimationViewTrack* av_track, noz_uint32 kf_index) {
  noz_assert(av_track);
  noz_assert(av_track->GetAnimationTrack());
  noz_assert(kf_index < av_track->GetAnimationTrack()->GetKeyFrameCount());

  SetTrackSelected(av_track, true);

  SelectedKeyFrame selected;
  selected.av_track_ = av_track;
  selected.kf_index_ = kf_index;
  selected.kf_ = av_track->GetAnimationTrack()->GetKeyFrame(kf_index);
  selected_key_frames_.push_back(selected);

  dope_sheet_->InvalidateSelectionRender();
}

void AnimationView::UnselectAllKeyFrames(void) {
  selected_key_frames_.clear();
  if(IsDopeSheetActive()) UnselectAllTracks();
}

void AnimationView::UnselectKeyFrame(AnimationViewTrack* av_track, noz_uint32 kf_index) {
  noz_assert(av_track);

  // Remove the key from the selection vector  
  for(noz_uint32 i=0, c=selected_key_frames_.size(); i<c; i++) {
    SelectedKeyFrame& skf = selected_key_frames_[i];
    if(skf.av_track_ == av_track && skf.kf_index_ == kf_index) {
      selected_key_frames_.erase(selected_key_frames_.begin()+i);
      break;
    }
  }

  // If the dope sheet is active unselect the track if no keys are selected
  if(IsDopeSheetActive()) {
    bool deselect= true;
    for(noz_uint32 i=0, c=selected_key_frames_.size() && deselect; i<c; i++) {
      SelectedKeyFrame& skf = selected_key_frames_[i];
      if(skf.av_track_ == av_track) deselect=false;
    }

    if(deselect) SetTrackSelected(av_track,false);
  }
}

void AnimationView::SetTrackSelected (AnimationViewTrack* av_track, bool selected) {
  noz_assert(av_track);

  if(selected) {
    if(av_track->IsSelected()) return;
    if(av_track->GetAnimationTrack()) selected_tracks_.push_back(av_track);

    av_track->selected_ = true;
    av_track->UpdateAnimationState();

    if(av_track->GetParentTrack() && !av_track->GetParentTrack()->IsSelected()) {
      av_track->GetParentTrack()->selected_ = true;
      av_track->GetParentTrack()->UpdateAnimationState();
    }    
  } else {
    if(!av_track->IsSelected()) return;

    // Remove from selected tracks vector
    for(noz_uint32 i=0,c=selected_tracks_.size(); i<c; i++) {
      if(selected_tracks_[i] == av_track) {
        selected_tracks_.erase(selected_tracks_.begin() + i);
        break;
      }
    }

    av_track->selected_ = false;
    av_track->UpdateAnimationState();

    AnimationViewTrack* av_parent_track = av_track->GetParentTrack();
    if(av_parent_track) {
      bool deselect = true;
      for(noz_uint32 i=0,c=av_parent_track->GetChildTrackCount(); i<c && deselect; i++) {
        deselect = !av_parent_track->GetChildTrack(i)->IsSelected();
      }

      if(deselect) {
        av_parent_track->selected_ = false;
        av_parent_track->UpdateAnimationState();
      }
    }
  }
}

/*
void AnimationView::RemoveKey (AnimationViewKeyFrame* av_key) {
  noz_assert(animation_);
  noz_assert(av_key);
  noz_assert(av_key->GetAnimationViewTrack());
  noz_assert(av_key->GetAnimationViewTrack()->GetAnimationTrack());
  noz_assert(av_key->GetAnimationViewTrack()->GetAnimationTarget());
  noz_assert(av_key->HasKeyFrame());

  // If the key is selected then remove it from the selected key list.
  if(av_key->IsSelected()) {
    for(noz_uint32 i=0,c=selected_keys_.size(); i<c; i++) {
      if(selected_keys_[i]==av_key) {
        selected_keys_.erase(selected_keys_.begin()+i);
        break;
      }
    }
  }

  // Cache the parent key
  AnimationViewKeyFrame* av_parent_key = av_key->parent_key_;

  // Remove the key frame from the track.
  AnimationViewTrack* av_track = av_key->GetAnimationViewTrack();
  av_track->RemoveKeyFrame(av_key);

  // Is the track empty?
  if(!av_track->HasKeyFrames()) {
    noz_uint32 animation_track_id = av_track->GetAnimationTrack()->GetTrackId();

    // Remove the animation track from the animation target which will free the track
    av_track->GetAnimationTarget()->RemoveTrack(av_track->GetAnimationTrack());

    // Disassociate the animation track with the animation view track
    av_track->animation_track_ = nullptr;

    // If the target no longer has any actual animation tracks..
    if(av_track->GetAnimationTarget()->GetTrackCount()==0) {
      // Remove the target from the animation..
      animation_->RemoveTarget(av_track->GetAnimationTarget());

      // If the view track has a parent then destroy that as well.
      if(av_track->GetParentTrack()) av_track->GetParentTrack()->Destroy();

      // Destroy the view track.
      av_track->Destroy();

    // Is the empty track a child track?
    } else if (av_track->GetParentTrack()) {
      // Create an empty animation track for the view track.
      AnimationTrack* animation_track = new AnimationTrack;
      animation_track->SetTrackId(animation_track_id);
      av_track->SetAnimationTrack(animation_track);
    
    // Is the track a standalone track?
    } else {
      av_track->Destroy();
    }
  }

  // If the parent key is empty destroy it as well.
  if(av_parent_key) {
    av_parent_key->RemoveChildKeyFrame(av_key);
    if(av_parent_key->child_keys_.empty()) {
      av_parent_key->Destroy();
    } else {
      av_parent_key->UpdateAnimationState();
    }
  }
}

void AnimationView::RemoveSelectedKeys (void) {
  // Ensure there are actually selected keys.
  if(selected_keys_.empty()) return;

  // Remove all of the selected keys individually
  while(!selected_keys_.empty()) {
    AnimationViewKeyFrame* av_key = selected_keys_[0];
    noz_assert(av_key);
    noz_assert(av_key->HasKeyFrame());
    RemoveKey(av_key);
  }

  // Make the animation as modified
  SetAnimationModified();
  RefreshAnimationFrame();
  UpdateTrackIndicies();
  UpdateZoom();
}
*/

bool AnimationView::IsKeyFrameSelected (AnimationViewTrack* av_track, noz_uint32 kf_index) const {
  for(noz_uint32 i=0,c=selected_key_frames_.size(); i<c; i++) {
    if(selected_key_frames_[i].av_track_ == av_track && selected_key_frames_[i].kf_index_ == kf_index) return true;
  }
  return false;
}

void AnimationView::OnAction (Action* action, bool undo) {
  noz_assert(action);

  if(action->IsTypeOf(typeof(GroupAction))) {
    if(undo) {
      for(noz_uint32 i=((GroupAction*)action)->GetActionCount(); i>0; i--) {
        OnAction(((GroupAction*)action)->GetAction(i-1),true);
      }
    } else {
      for(noz_uint32 i=0,c=((GroupAction*)action)->GetActionCount(); i<c; i++) {
        OnAction(((GroupAction*)action)->GetAction(i),false);
      }
    }
  } else if(action->IsTypeOf(typeof(SetKeyFrameAction))) {
    OnAction((SetKeyFrameAction*)action, undo);
  } else if(action->IsTypeOf(typeof(SetAnimatorTargetAction))) {
    OnAction((SetAnimatorTargetAction*)action, undo);
  } else if(action->IsTypeOf(typeof(AddAnimationTargetAction))) {
    OnAction((AddAnimationTargetAction*)action, undo);
  } else if(action->IsTypeOf(typeof(SetKeyFrameTangentsAction))) {
    OnAction((SetKeyFrameTangentsAction*)action, undo);
  } else if(action->IsTypeOf(typeof(RemoveKeyFrameAction))) {
    OnAction((RemoveKeyFrameAction*)action, undo);
  }
}

void AnimationView::OnAction (SetKeyFrameAction* action, bool undo) {
  AnimationTrack* animation_track = action->GetAnimationTrack();

  AnimationViewTrack* av_track = FindTrack(action->GetAnimationTarget(), animation_track->GetTrackId());
  noz_assert(av_track);

  if(action->animation_track_added_) {
    if(undo) {
      av_track->SetAnimationTrack(nullptr);
    } else {
      av_track->SetAnimationTrack(action->GetAnimationTrack());
    }
  }
  av_track->Refresh();

  dope_sheet_->InvalidateRender();
  SetAnimationModified();
  RefreshAnimationFrame();  
}

void AnimationView::OnAction (SetAnimatorTargetAction* action, bool undo) {
  noz_assert(action);
  
  // Find the animation target that matches the given animator target..
  AnimationTarget* animation_target = animation_->GetTarget(action->GetAnimatorTarget()->GetGuid());
  noz_assert(animation_target);

  // Find the animation view track that matches the animation target.
  AnimationViewTrack* av_track = FindTrack(animation_target);
  noz_assert(av_track);
  noz_assert(av_track->IsParentTrack());

  // Keep the local animator in sync.
  if(animator_) animator_->RemoveTarget(animator_->GetTarget(animation_target->GetGuid()));

  if(undo && action->IsAnimatorTargetAdded()) {
    av_track->SetAnimatorTarget(nullptr);
  } else {
    av_track->SetAnimatorTarget(action->GetAnimatorTarget());

    if(animator_) animator_->AddTarget(animation_target->GetGuid(), action->GetAnimatorTarget()->GetTarget());
  }

  av_track->Refresh();
  av_track->UpdateAnimationState();

  SetAnimationModified();
}

void AnimationView::OnAction (AddAnimationTargetAction* action, bool undo) {
  noz_assert(action);
  
  if(undo) {
    RemoveTrack(action->GetAnimationTarget());
  } else {
    AddTrack(action->GetAnimationTarget());
  }

  SetAnimationModified();
}

void AnimationView::OnAction (SetKeyFrameTangentsAction* action, bool undo) {
  noz_assert(action);
  curve_editor_->UpdateHandleMesh();
  SetAnimationModified();
}

void AnimationView::OnAction (RemoveKeyFrameAction* action, bool undo) {
  noz_assert(action);
  SetAnimationModified();

  dope_sheet_->InvalidateRender();
  curve_editor_->UpdateTracks();

  AnimationViewTrack* av_track = FindTrack(action->animation_target_, action->animation_track_->GetTrackId());
  noz_assert(av_track);

  if(undo) {
    SelectKeyFrame(av_track, action->kf_index_);
  } else {
    UnselectKeyFrame(av_track, action->kf_index_);
  }

  if(action->animation_target_->GetTrackCount()==0) {
    RemoveTrack(action->animation_target_);
  }
}

void AnimationView::OpenKeyFramePopup (AnimationViewTrack* av_track, noz_uint32 kf_index, const Vector2& offset) {
  if(nullptr==key_frame_popup_) return;
  
  key_frame_popup_->SetPlacementTarget(IsDopeSheetActive() ? (Node*)dope_sheet_ : (Node*)curve_editor_);
  key_frame_popup_->SetPlacementOffset(offset);
  key_frame_popup_->Open();
}

void AnimationView::OnDeleteKeyFrameMenuItem (UINode*) {
  RemoveSelectedKeyFrames();
  key_frame_popup_->Close();
}
