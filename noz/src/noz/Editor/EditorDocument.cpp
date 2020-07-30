///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Actions/Action.h>
#include <noz/Editor/Nodes/UI/AnimationView.h>
#include "EditorDocument.h"

using namespace noz;
using namespace noz::Editor;


EditorDocument::EditorDocument(void) {
  undo_action_ = 0;
  undo_group_comitted_ = true;
  current_group_action_ = nullptr;
}

EditorDocument::~EditorDocument(void) {
}

void EditorDocument::SetInspector (Inspector* inspector) {
  if(inspector_ == inspector) return;
  if(inspector_) inspector_->SetDocument(nullptr);
  inspector_ = inspector;    
  if(inspector_) inspector_->SetDocument(this);
  if(IsActive()) OnActivate ();
}

void EditorDocument::SetHierarchy(Hierarchy* hierarchy) {
  if(hierarchy_ == hierarchy) return;
  hierarchy_ = hierarchy;    
  if(IsActive()) OnActivate();
}

void EditorDocument::SetAnimationViewSource (Animator* animator) {
  if(animation_view_source_==animator) return;
  animation_view_source_ = animator;
  if(IsActive()) {
    Workspace* workspace = Workspace::GetWorkspace(this);
    workspace->SetAnimationViewAnimator(animation_view_source_);
  }
}

void EditorDocument::OnActivate (void) {
  Workspace* workspace = Workspace::GetWorkspace(this);
  if(nullptr == workspace) return;
  workspace->SetInspector(inspector_);
  workspace->SetHierarchy(hierarchy_);  
  workspace->SetAnimationViewAnimator(animation_view_source_);
}

void EditorDocument::OnDeactivate (void) {
  Workspace* workspace = Workspace::GetWorkspace(this);
  if(nullptr == workspace) return;
  workspace->SetHierarchy(nullptr);
  workspace->SetInspector(nullptr);
  workspace->SetAnimationViewAnimator(nullptr);
}

void EditorDocument::Select (Node* n) {
  if(nullptr == hierarchy_) return;
  hierarchy_->SetSelected(n);    
}

void EditorDocument::ExecuteAction (Action* action, bool commit_group) {
  noz_assert(action);

  // Give the animation view a chance to execute the action.
  AnimationView* av = AnimationView::GetAnimationView(this);
  if(av->ExecuteAction (action)) {
    if(commit_group) CommitUndoGroup();
    return;
  }

  // Erase any actions that were undone.
  while((noz_int32)undo_.size() > undo_action_) {
    delete undo_.back();
    undo_.pop_back();
  }

  action->Do();

  // Allow the animation view to respond to the animation being executed.
  av->OnAction(action,false);

  // If the previous undo group was comitted then add the new action
  // and mark as no longer comitted.
  if(undo_group_comitted_) {
    undo_.push_back(action);
    undo_action_++;
    undo_group_comitted_ = false;
  
  // Not committed and not already grouped
  } else if (nullptr == current_group_action_) {
    Action* last_action = undo_.back();
    if(last_action->CanMerge(action)) {
      last_action->Merge(action);
      delete action;
    } else {
      current_group_action_ = new GroupAction;
      current_group_action_->AddAction(last_action);
      current_group_action_->AddAction(action);
      undo_.pop_back();
      undo_.push_back(current_group_action_);
    }

  // Actions are already group so check if we can merge with last action in the group
  } else if(current_group_action_->GetLastAction()->CanMerge(action)) {
    current_group_action_->GetLastAction()->Merge(action);
    delete action;

  // Add the action to the end of the current group
  } else {
    current_group_action_->AddAction(action);
  }

  if(commit_group) {
    CommitUndoGroup();
  }
}

void EditorDocument::CommitUndoGroup (void) {
  current_group_action_ = nullptr;
  undo_group_comitted_ = true;
}

void EditorDocument::Undo (void) {
  CommitUndoGroup();

  if(undo_action_>0) {
    Action* action = undo_[--undo_action_];
    action->Undo();
    AnimationView::GetAnimationView(this)->OnAction(action,true);
  }
}

void EditorDocument::Redo (void) {
  if(undo_action_<(noz_int32)undo_.size()) {
    Action* action = undo_[undo_action_++];
    action->Do();
    AnimationView::GetAnimationView(this)->OnAction(action,false);
  }
}


EditorDocument* EditorDocument::GetActiveDocument(Node* node) {
  Workspace* workspace = Workspace::GetWorkspace(node);
  if(nullptr == workspace) return nullptr;
  return workspace->GetActiveDocument();
}

EditorDocument* EditorDocument::GetActiveDocument(Object* o) {
  if(nullptr == o) return nullptr;
  if(o->IsTypeOf(typeof(Node))) return GetActiveDocument((Node*)o);
  if(o->IsTypeOf(typeof(Component))) return GetActiveDocument(((Component*)o)->GetNode());
  return nullptr;
}
